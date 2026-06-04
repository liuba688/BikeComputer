#include "mpu6050.h"
#include "bike_data.h"

extern I2C_HandleTypeDef hi2c1;

#define MPU6050_I2C_TIMEOUT_MS  100U
#define MPU6050_PWR_MGMT_1_REG  0x6BU
#define MPU6050_ACCEL_XOUT_H    0x3BU
#define MPU6050_TEMP_OUT_H      0x41U
#define MPU6050_GYRO_XOUT_H     0x43U
#define MPU6050_ACCEL_SCALE     16384.0f
#define MPU6050_RAD_TO_DEG      57.29578f

HAL_StatusTypeDef MPU6050_ReadWhoAmI(uint8_t *who_am_i)
{
  if (who_am_i == 0)
  {
    return HAL_ERROR;
  }

  return HAL_I2C_Mem_Read(&hi2c1,
                          MPU6050_I2C_ADDRESS,
                          MPU6050_WHO_AM_I_REG,
                          I2C_MEMADD_SIZE_8BIT,
                          who_am_i,
                          1U,
                          MPU6050_I2C_TIMEOUT_MS);
}

uint8_t MPU6050_IsFound(void)
{
  uint8_t who_am_i = 0U;

  if (MPU6050_ReadWhoAmI(&who_am_i) != HAL_OK)
  {
    return 0U;
  }

  return (who_am_i == MPU6050_WHO_AM_I_VALUE) ? 1U : 0U;
}

HAL_StatusTypeDef MPU6050_Init(void)
{
  uint8_t data = 0U;

  if (MPU6050_IsFound() == 0U)
  {
    return HAL_ERROR;
  }

  return HAL_I2C_Mem_Write(&hi2c1,
                           MPU6050_I2C_ADDRESS,
                           MPU6050_PWR_MGMT_1_REG,
                           I2C_MEMADD_SIZE_8BIT,
                           &data,
                           1U,
                           MPU6050_I2C_TIMEOUT_MS);
}

static int16_t MPU6050_ToInt16(uint8_t high_byte, uint8_t low_byte)
{
  return (int16_t)((uint16_t)((uint16_t)high_byte << 8) | low_byte);
}

static float MPU6050_AbsFloat(float value)
{
  return (value < 0.0f) ? -value : value;
}

static float MPU6050_SqrtApprox(float value)
{
  float result;

  if (value <= 0.0f)
  {
    return 0.0f;
  }

  result = value;
  for (uint8_t i = 0U; i < 6U; i++)
  {
    result = 0.5f * (result + (value / result));
  }

  return result;
}

static float MPU6050_AtanApprox(float value)
{
  float abs_value = MPU6050_AbsFloat(value);
  float angle;

  if (abs_value <= 1.0f)
  {
    angle = value / (1.0f + (0.28f * value * value));
  }
  else
  {
    angle = 1.5707963f - (abs_value / ((abs_value * abs_value) + 0.28f));
    if (value < 0.0f)
    {
      angle = -angle;
    }
  }

  return angle;
}

static float MPU6050_Atan2Approx(float y, float x)
{
  if (x > 0.0f)
  {
    return MPU6050_AtanApprox(y / x);
  }

  if (x < 0.0f)
  {
    return (y >= 0.0f) ? (MPU6050_AtanApprox(y / x) + 3.1415927f) : (MPU6050_AtanApprox(y / x) - 3.1415927f);
  }

  if (y > 0.0f)
  {
    return 1.5707963f;
  }

  if (y < 0.0f)
  {
    return -1.5707963f;
  }

  return 0.0f;
}

HAL_StatusTypeDef MPU6050_ReadRawAccel(MPU6050_RawVector_t *accel)
{
  uint8_t data[6];
  HAL_StatusTypeDef status;

  if (accel == 0)
  {
    return HAL_ERROR;
  }

  status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDRESS, MPU6050_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, data, sizeof(data), MPU6050_I2C_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  accel->x = MPU6050_ToInt16(data[0], data[1]);
  accel->y = MPU6050_ToInt16(data[2], data[3]);
  accel->z = MPU6050_ToInt16(data[4], data[5]);
  return HAL_OK;
}

HAL_StatusTypeDef MPU6050_ReadRawGyro(MPU6050_RawVector_t *gyro)
{
  uint8_t data[6];
  HAL_StatusTypeDef status;

  if (gyro == 0)
  {
    return HAL_ERROR;
  }

  status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDRESS, MPU6050_GYRO_XOUT_H, I2C_MEMADD_SIZE_8BIT, data, sizeof(data), MPU6050_I2C_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  gyro->x = MPU6050_ToInt16(data[0], data[1]);
  gyro->y = MPU6050_ToInt16(data[2], data[3]);
  gyro->z = MPU6050_ToInt16(data[4], data[5]);
  return HAL_OK;
}

HAL_StatusTypeDef MPU6050_ReadTemperature(float *temperature_c)
{
  uint8_t data[2];
  int16_t raw_temperature;
  HAL_StatusTypeDef status;

  if (temperature_c == 0)
  {
    return HAL_ERROR;
  }

  status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDRESS, MPU6050_TEMP_OUT_H, I2C_MEMADD_SIZE_8BIT, data, sizeof(data), MPU6050_I2C_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  raw_temperature = MPU6050_ToInt16(data[0], data[1]);
  *temperature_c = ((float)raw_temperature / 340.0f) + 36.53f;
  return HAL_OK;
}

HAL_StatusTypeDef MPU6050_UpdateBikeData(void)
{
  MPU6050_RawVector_t accel;
  MPU6050_RawVector_t gyro;
  float accelX_g;
  float accelY_g;
  float accelZ_g;
  float temperature_c;
  float yz_magnitude;
  HAL_StatusTypeDef status;

  status = MPU6050_ReadRawAccel(&accel);
  if (status != HAL_OK)
  {
    return status;
  }

  accelX_g = (float)accel.x / MPU6050_ACCEL_SCALE;
  accelY_g = (float)accel.y / MPU6050_ACCEL_SCALE;
  accelZ_g = (float)accel.z / MPU6050_ACCEL_SCALE;
  yz_magnitude = MPU6050_SqrtApprox((accelY_g * accelY_g) + (accelZ_g * accelZ_g));

  BikeData.pitch = MPU6050_Atan2Approx(-accelX_g, yz_magnitude) * MPU6050_RAD_TO_DEG;
  BikeData.roll = MPU6050_Atan2Approx(accelY_g, accelZ_g) * MPU6050_RAD_TO_DEG;

  if (MPU6050_ReadRawGyro(&gyro) == HAL_OK)
  {
    (void)gyro;
  }

  if (MPU6050_ReadTemperature(&temperature_c) == HAL_OK)
  {
    BikeData.imuTemperature = temperature_c;
  }

  return status;
}
