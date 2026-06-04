#include "bmp280.h"
#include "bike_data.h"
#include <math.h>

extern I2C_HandleTypeDef hi2c1;

#define BMP280_I2C_TIMEOUT_MS  100U
#define BMP280_CALIB_REG       0x88U
#define BMP280_CTRL_MEAS_REG   0xF4U
#define BMP280_CONFIG_REG      0xF5U
#define BMP280_PRESS_MSB_REG   0xF7U
#define BMP280_SEA_LEVEL_HPA   1013.25f

typedef struct
{
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
} BMP280_CalibData_t;

static uint8_t bmp280_address = 0U;
static int32_t bmp280_t_fine = 0;
static BMP280_CalibData_t bmp280_calib;

static uint16_t BMP280_U16LE(const uint8_t *data)
{
  return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static int16_t BMP280_S16LE(const uint8_t *data)
{
  return (int16_t)BMP280_U16LE(data);
}

HAL_StatusTypeDef BMP280_ReadChipIdAt(uint8_t address, uint8_t *chip_id)
{
  if (chip_id == 0)
  {
    return HAL_ERROR;
  }

  return HAL_I2C_Mem_Read(&hi2c1,
                          (uint16_t)(address << 1),
                          BMP280_CHIP_ID_REG,
                          I2C_MEMADD_SIZE_8BIT,
                          chip_id,
                          1U,
                          BMP280_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef BMP280_DetectAddress(uint8_t *address)
{
  const uint8_t addresses[] = {BMP280_I2C_ADDRESS_0, BMP280_I2C_ADDRESS_1};
  uint8_t chip_id = 0U;

  for (uint8_t i = 0U; i < (uint8_t)(sizeof(addresses) / sizeof(addresses[0])); i++)
  {
    if ((BMP280_ReadChipIdAt(addresses[i], &chip_id) == HAL_OK) && (chip_id == BMP280_CHIP_ID_VALUE))
    {
      *address = addresses[i];
      return HAL_OK;
    }
  }

  return HAL_ERROR;
}

static HAL_StatusTypeDef BMP280_ReadCalibration(void)
{
  uint8_t data[24];
  HAL_StatusTypeDef status;

  status = HAL_I2C_Mem_Read(&hi2c1,
                            (uint16_t)(bmp280_address << 1),
                            BMP280_CALIB_REG,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            sizeof(data),
                            BMP280_I2C_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  bmp280_calib.dig_T1 = BMP280_U16LE(&data[0]);
  bmp280_calib.dig_T2 = BMP280_S16LE(&data[2]);
  bmp280_calib.dig_T3 = BMP280_S16LE(&data[4]);
  bmp280_calib.dig_P1 = BMP280_U16LE(&data[6]);
  bmp280_calib.dig_P2 = BMP280_S16LE(&data[8]);
  bmp280_calib.dig_P3 = BMP280_S16LE(&data[10]);
  bmp280_calib.dig_P4 = BMP280_S16LE(&data[12]);
  bmp280_calib.dig_P5 = BMP280_S16LE(&data[14]);
  bmp280_calib.dig_P6 = BMP280_S16LE(&data[16]);
  bmp280_calib.dig_P7 = BMP280_S16LE(&data[18]);
  bmp280_calib.dig_P8 = BMP280_S16LE(&data[20]);
  bmp280_calib.dig_P9 = BMP280_S16LE(&data[22]);

  return HAL_OK;
}

HAL_StatusTypeDef BMP280_Init(void)
{
  uint8_t ctrl_meas = 0x27U;
  uint8_t config = 0xA0U;
  HAL_StatusTypeDef status;

  status = BMP280_DetectAddress(&bmp280_address);
  if (status != HAL_OK)
  {
    return status;
  }

  status = BMP280_ReadCalibration();
  if (status != HAL_OK)
  {
    return status;
  }

  status = HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(bmp280_address << 1), BMP280_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &config, 1U, BMP280_I2C_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  return HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(bmp280_address << 1), BMP280_CTRL_MEAS_REG, I2C_MEMADD_SIZE_8BIT, &ctrl_meas, 1U, BMP280_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef BMP280_ReadRaw(int32_t *raw_temperature, int32_t *raw_pressure)
{
  uint8_t data[6];
  HAL_StatusTypeDef status;

  if ((raw_temperature == 0) || (raw_pressure == 0) || (bmp280_address == 0U))
  {
    return HAL_ERROR;
  }

  status = HAL_I2C_Mem_Read(&hi2c1,
                            (uint16_t)(bmp280_address << 1),
                            BMP280_PRESS_MSB_REG,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            sizeof(data),
                            BMP280_I2C_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  *raw_pressure = (int32_t)((((uint32_t)data[0]) << 12) | (((uint32_t)data[1]) << 4) | (((uint32_t)data[2]) >> 4));
  *raw_temperature = (int32_t)((((uint32_t)data[3]) << 12) | (((uint32_t)data[4]) << 4) | (((uint32_t)data[5]) >> 4));

  return HAL_OK;
}

static float BMP280_CompensateTemperature(int32_t adc_T)
{
  int32_t var1;
  int32_t var2;
  int32_t temperature;

  var1 = ((((adc_T >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) * ((int32_t)bmp280_calib.dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) * ((int32_t)bmp280_calib.dig_T3)) >> 14;
  bmp280_t_fine = var1 + var2;
  temperature = (bmp280_t_fine * 5 + 128) >> 8;

  return (float)temperature / 100.0f;
}

static float BMP280_CompensatePressure(int32_t adc_P)
{
  int64_t var1;
  int64_t var2;
  int64_t pressure;

  var1 = ((int64_t)bmp280_t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)bmp280_calib.dig_P6;
  var2 = var2 + ((var1 * (int64_t)bmp280_calib.dig_P5) << 17);
  var2 = var2 + (((int64_t)bmp280_calib.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)bmp280_calib.dig_P3) >> 8) + ((var1 * (int64_t)bmp280_calib.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280_calib.dig_P1) >> 33;

  if (var1 == 0)
  {
    return 0.0f;
  }

  pressure = 1048576 - adc_P;
  pressure = (((pressure << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)bmp280_calib.dig_P9) * (pressure >> 13) * (pressure >> 13)) >> 25;
  var2 = (((int64_t)bmp280_calib.dig_P8) * pressure) >> 19;
  pressure = ((pressure + var1 + var2) >> 8) + (((int64_t)bmp280_calib.dig_P7) << 4);

  return ((float)pressure / 256.0f) / 100.0f;
}

HAL_StatusTypeDef BMP280_ReadTemperature(float *temperature_c)
{
  int32_t raw_temperature;
  int32_t raw_pressure;
  HAL_StatusTypeDef status;

  if (temperature_c == 0)
  {
    return HAL_ERROR;
  }

  status = BMP280_ReadRaw(&raw_temperature, &raw_pressure);
  if (status != HAL_OK)
  {
    return status;
  }

  (void)raw_pressure;
  *temperature_c = BMP280_CompensateTemperature(raw_temperature);
  return HAL_OK;
}

HAL_StatusTypeDef BMP280_ReadPressure(float *pressure_hpa)
{
  int32_t raw_temperature;
  int32_t raw_pressure;
  HAL_StatusTypeDef status;

  if (pressure_hpa == 0)
  {
    return HAL_ERROR;
  }

  status = BMP280_ReadRaw(&raw_temperature, &raw_pressure);
  if (status != HAL_OK)
  {
    return status;
  }

  (void)BMP280_CompensateTemperature(raw_temperature);
  *pressure_hpa = BMP280_CompensatePressure(raw_pressure);
  return HAL_OK;
}

float BMP280_CalculateAltitude(float pressure_hpa)
{
  if (pressure_hpa <= 0.0f)
  {
    return 0.0f;
  }

  return 44330.0f * (1.0f - powf(pressure_hpa / BMP280_SEA_LEVEL_HPA, 0.1903f));
}

HAL_StatusTypeDef BMP280_UpdateBikeData(void)
{
  int32_t raw_temperature;
  int32_t raw_pressure;
  float temperature_c;
  float pressure_hpa;
  HAL_StatusTypeDef status;

  status = BMP280_ReadRaw(&raw_temperature, &raw_pressure);
  if (status != HAL_OK)
  {
    return status;
  }

  temperature_c = BMP280_CompensateTemperature(raw_temperature);
  pressure_hpa = BMP280_CompensatePressure(raw_pressure);

  BikeData.ambientTemperature = temperature_c;
  BikeData.pressure = pressure_hpa;
  BikeData.altitude = BMP280_CalculateAltitude(pressure_hpa);

  return HAL_OK;
}

uint8_t BMP280_IsFound(void)
{
  uint8_t address = 0U;

  return (BMP280_DetectAddress(&address) == HAL_OK) ? 1U : 0U;
}
