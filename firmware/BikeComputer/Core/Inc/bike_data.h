#ifndef __BIKE_DATA_H
#define __BIKE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BIKE_DATA_UTC_TIME_MAX  12U

typedef struct
{
  float speed;
  uint16_t power;
  uint16_t cadence;
  uint16_t heartRate;
  float altitude;
  float distance;
  float maxSpeed;
  uint32_t rideTime;
  float imuTemperature;
  float ambientTemperature;
  float pressure;
  float pitch;
  float roll;
  uint8_t gpsFix;
  float latitude;
  float longitude;
  float gpsSpeed;
  float gpsAltitude;
  uint8_t satelliteCount;
  char utcTime[BIKE_DATA_UTC_TIME_MAX];
  uint8_t battery;
} BikeData_t;

extern BikeData_t BikeData;

void BikeData_InitDemo(void);
void BikeData_UpdateDemo(void);

#ifdef __cplusplus
}
#endif

#endif /* __BIKE_DATA_H */
