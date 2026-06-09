#include "ride_stats.h"
#include "bike_data.h"
#include "stm32f4xx_hal.h"
#include <math.h>

#define RIDE_STATS_EARTH_RADIUS_M      6371000.0f
#define RIDE_STATS_DEG_TO_RAD          0.01745329252f
#define RIDE_STATS_MIN_DISTANCE_M      3.0f
#define RIDE_STATS_MOVING_SPEED_KMH    0.5f

static uint8_t ride_stats_has_last_position = 0U;
static float ride_stats_last_latitude = 0.0f;
static float ride_stats_last_longitude = 0.0f;
static uint32_t ride_stats_last_tick = 0U;
static uint32_t ride_stats_time_accumulator_ms = 0U;

static float RideStats_DistanceMeters(float lat1, float lon1, float lat2, float lon2)
{
  float lat1_rad = lat1 * RIDE_STATS_DEG_TO_RAD;
  float lat2_rad = lat2 * RIDE_STATS_DEG_TO_RAD;
  float dlat = (lat2 - lat1) * RIDE_STATS_DEG_TO_RAD;
  float dlon = (lon2 - lon1) * RIDE_STATS_DEG_TO_RAD;
  float mean_lat = (lat1_rad + lat2_rad) * 0.5f;
  float x = dlon * cosf(mean_lat);
  float y = dlat;

  return sqrtf((x * x) + (y * y)) * RIDE_STATS_EARTH_RADIUS_M;
}

void RideStats_Init(void)
{
  ride_stats_has_last_position = 0U;
  ride_stats_last_latitude = 0.0f;
  ride_stats_last_longitude = 0.0f;
  ride_stats_last_tick = HAL_GetTick();
  ride_stats_time_accumulator_ms = 0U;

  BikeData.distance = 0.0f;
  BikeData.maxSpeed = 0.0f;
  BikeData.rideTime = 0U;
  BikeData.gpsSpeed = 0.0f;
  BikeData.speed = 0.0f;
}

void RideStats_Update(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t delta_ms = now - ride_stats_last_tick;

  ride_stats_last_tick = now;

  if (BikeData.gpsFix == 0U)
  {
    ride_stats_has_last_position = 0U;
    return;
  }

  if ((BikeData.latitude == 0.0f) && (BikeData.longitude == 0.0f))
  {
    return;
  }

  if (BikeData.gpsSpeed > BikeData.maxSpeed)
  {
    BikeData.maxSpeed = BikeData.gpsSpeed;
  }

  if (BikeData.gpsSpeed > RIDE_STATS_MOVING_SPEED_KMH)
  {
    ride_stats_time_accumulator_ms += delta_ms;
    while (ride_stats_time_accumulator_ms >= 1000U)
    {
      BikeData.rideTime++;
      ride_stats_time_accumulator_ms -= 1000U;
    }
  }

  if (ride_stats_has_last_position == 0U)
  {
    ride_stats_last_latitude = BikeData.latitude;
    ride_stats_last_longitude = BikeData.longitude;
    ride_stats_has_last_position = 1U;
    return;
  }

  {
    float distance_m = RideStats_DistanceMeters(ride_stats_last_latitude,
                                                ride_stats_last_longitude,
                                                BikeData.latitude,
                                                BikeData.longitude);

    if (distance_m >= RIDE_STATS_MIN_DISTANCE_M)
    {
      BikeData.distance += distance_m / 1000.0f;
      ride_stats_last_latitude = BikeData.latitude;
      ride_stats_last_longitude = BikeData.longitude;
    }
  }
}
