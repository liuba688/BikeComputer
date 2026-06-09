#include "bike_data.h"

BikeData_t BikeData;

void BikeData_InitDemo(void)
{
  BikeData.speed = 0.0f;
  BikeData.power = 250U;
  BikeData.cadence = 92U;
  BikeData.heartRate = 158U;
  BikeData.altitude = 123.4f;
  BikeData.distance = 0.0f;
  BikeData.maxSpeed = 0.0f;
  BikeData.rideTime = 0U;
  BikeData.imuTemperature = 25.0f;
  BikeData.ambientTemperature = 25.0f;
  BikeData.pressure = 1008.2f;
  BikeData.pitch = 0.0f;
  BikeData.roll = 0.0f;
  BikeData.gpsFix = 0U;
  BikeData.latitude = 0.0f;
  BikeData.longitude = 0.0f;
  BikeData.gpsSpeed = 0.0f;
  BikeData.gpsAltitude = 0.0f;
  BikeData.satelliteCount = 0U;
  BikeData.utcTime[0] = '\0';
  BikeData.battery = 95U;
}

void BikeData_UpdateDemo(void)
{
  BikeData.speed += 0.1f;
  BikeData.power += 1U;
  BikeData.pitch += 0.5f;
  BikeData.roll -= 0.3f;

  if (BikeData.speed > 60.0f)
  {
    BikeData.speed = 0.0f;
  }

  if (BikeData.power > 999U)
  {
    BikeData.power = 0U;
  }

  if (BikeData.pitch > 45.0f)
  {
    BikeData.pitch = -45.0f;
  }

  if (BikeData.roll < -45.0f)
  {
    BikeData.roll = 45.0f;
  }
}
