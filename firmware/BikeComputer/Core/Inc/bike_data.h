#ifndef __BIKE_DATA_H
#define __BIKE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  float speed;
  uint16_t power;
  uint16_t cadence;
  uint16_t heartRate;
  float altitude;
  float distance;
  float temperature;
  float pitch;
  float roll;
  uint8_t battery;
} BikeData_t;

extern BikeData_t BikeData;

void BikeData_InitDemo(void);
void BikeData_UpdateDemo(void);

#ifdef __cplusplus
}
#endif

#endif /* __BIKE_DATA_H */
