#ifndef __BMP280_H
#define __BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define BMP280_I2C_ADDRESS_0   0x76U
#define BMP280_I2C_ADDRESS_1   0x77U
#define BMP280_CHIP_ID_REG     0xD0U
#define BMP280_CHIP_ID_VALUE   0x58U

HAL_StatusTypeDef BMP280_ReadChipIdAt(uint8_t address, uint8_t *chip_id);
HAL_StatusTypeDef BMP280_Init(void);
HAL_StatusTypeDef BMP280_ReadTemperature(float *temperature_c);
HAL_StatusTypeDef BMP280_ReadPressure(float *pressure_hpa);
float BMP280_CalculateAltitude(float pressure_hpa);
HAL_StatusTypeDef BMP280_UpdateBikeData(void);
uint8_t BMP280_IsFound(void);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */
