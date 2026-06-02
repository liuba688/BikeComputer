#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define MPU6050_I2C_ADDRESS      (0x68U << 1)
#define MPU6050_WHO_AM_I_REG     0x75U
#define MPU6050_WHO_AM_I_VALUE   0x68U

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t z;
} MPU6050_RawVector_t;

HAL_StatusTypeDef MPU6050_ReadWhoAmI(uint8_t *who_am_i);
uint8_t MPU6050_IsFound(void);
HAL_StatusTypeDef MPU6050_Init(void);
HAL_StatusTypeDef MPU6050_ReadRawAccel(MPU6050_RawVector_t *accel);
HAL_StatusTypeDef MPU6050_ReadRawGyro(MPU6050_RawVector_t *gyro);
HAL_StatusTypeDef MPU6050_ReadTemperature(float *temperature_c);
HAL_StatusTypeDef MPU6050_UpdateBikeData(void);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
