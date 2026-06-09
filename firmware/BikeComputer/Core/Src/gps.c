#include "gps.h"
#include "bike_data.h"

extern UART_HandleTypeDef huart1;

#define GPS_KNOTS_TO_KMH  1.852f
#define GPS_FIELD_MAX     18U

static uint8_t gps_rx_byte = 0U;
static char gps_sentence_buffer[GPS_NMEA_SENTENCE_MAX];
static char gps_latest_sentence[GPS_NMEA_SENTENCE_MAX];
static char gps_pending_sentence[GPS_NMEA_SENTENCE_MAX];
static uint16_t gps_sentence_index = 0U;
static uint8_t gps_sentence_overflow = 0U;
static volatile uint8_t gps_sentence_ready = 0U;
static volatile uint8_t gps_parse_pending = 0U;

static void GPS_ClearSentenceBuffer(void)
{
  gps_sentence_index = 0U;
  gps_sentence_overflow = 0U;
  gps_sentence_buffer[0] = '\0';
}

static void GPS_CopyString(char *destination, const char *source, uint16_t destination_size)
{
  uint16_t index = 0U;

  if (destination_size == 0U)
  {
    return;
  }

  while ((source[index] != '\0') && (index < (uint16_t)(destination_size - 1U)))
  {
    destination[index] = source[index];
    index++;
  }

  destination[index] = '\0';
}

static uint8_t GPS_StringEquals(const char *left, const char *right)
{
  while ((*left != '\0') && (*right != '\0'))
  {
    if (*left != *right)
    {
      return 0U;
    }

    left++;
    right++;
  }

  return (*left == *right) ? 1U : 0U;
}

static uint8_t GPS_IsDigit(char value)
{
  return ((value >= '0') && (value <= '9')) ? 1U : 0U;
}

static float GPS_ParseFloat(const char *text)
{
  float value = 0.0f;
  float decimal_place = 0.1f;
  uint8_t negative = 0U;
  uint8_t after_decimal = 0U;

  if (*text == '-')
  {
    negative = 1U;
    text++;
  }

  while ((*text != '\0') && (*text != '*'))
  {
    if (*text == '.')
    {
      after_decimal = 1U;
    }
    else if (GPS_IsDigit(*text) != 0U)
    {
      if (after_decimal == 0U)
      {
        value = (value * 10.0f) + (float)(*text - '0');
      }
      else
      {
        value += ((float)(*text - '0') * decimal_place);
        decimal_place *= 0.1f;
      }
    }
    else
    {
      break;
    }

    text++;
  }

  return (negative != 0U) ? -value : value;
}

static uint8_t GPS_ParseUInt8(const char *text)
{
  uint16_t value = 0U;

  while (GPS_IsDigit(*text) != 0U)
  {
    value = (uint16_t)((value * 10U) + (uint16_t)(*text - '0'));
    if (value > 255U)
    {
      return 255U;
    }
    text++;
  }

  return (uint8_t)value;
}

static uint8_t GPS_GetField(const char *sentence, uint8_t field_index, char *field, uint16_t field_size)
{
  uint8_t current_field = 0U;
  uint16_t index = 0U;

  if (field_size == 0U)
  {
    return 0U;
  }

  field[0] = '\0';

  while ((*sentence != '\0') && (*sentence != '*'))
  {
    if (*sentence == ',')
    {
      if (current_field == field_index)
      {
        field[index] = '\0';
        return 1U;
      }

      current_field++;
      index = 0U;
      sentence++;
      continue;
    }

    if (current_field == field_index)
    {
      if (index < (uint16_t)(field_size - 1U))
      {
        field[index] = *sentence;
        index++;
      }
    }

    sentence++;
  }

  if (current_field == field_index)
  {
    field[index] = '\0';
    return 1U;
  }

  return 0U;
}

static void GPS_CopyUtcTime(const char *text)
{
  uint8_t index = 0U;

  while ((text[index] != '\0') && (text[index] != '*') && (index < (BIKE_DATA_UTC_TIME_MAX - 1U)))
  {
    BikeData.utcTime[index] = text[index];
    index++;
  }

  BikeData.utcTime[index] = '\0';
}

static float GPS_ConvertCoordinate(const char *coordinate, const char *hemisphere)
{
  float raw = GPS_ParseFloat(coordinate);
  uint16_t degrees = (uint16_t)(raw / 100.0f);
  float minutes = raw - ((float)degrees * 100.0f);
  float decimal_degrees = (float)degrees + (minutes / 60.0f);

  if ((hemisphere[0] == 'S') || (hemisphere[0] == 'W'))
  {
    decimal_degrees = -decimal_degrees;
  }

  return decimal_degrees;
}

static void GPS_UpdateLatLon(uint8_t lat_field, uint8_t ns_field, uint8_t lon_field, uint8_t ew_field, const char *sentence)
{
  char latitude[GPS_FIELD_MAX];
  char ns[GPS_FIELD_MAX];
  char longitude[GPS_FIELD_MAX];
  char ew[GPS_FIELD_MAX];

  if ((GPS_GetField(sentence, lat_field, latitude, sizeof(latitude)) != 0U) &&
      (GPS_GetField(sentence, ns_field, ns, sizeof(ns)) != 0U) &&
      (GPS_GetField(sentence, lon_field, longitude, sizeof(longitude)) != 0U) &&
      (GPS_GetField(sentence, ew_field, ew, sizeof(ew)) != 0U) &&
      (latitude[0] != '\0') &&
      (longitude[0] != '\0'))
  {
    BikeData.latitude = GPS_ConvertCoordinate(latitude, ns);
    BikeData.longitude = GPS_ConvertCoordinate(longitude, ew);
  }
}

static void GPS_ParseGGA(const char *sentence)
{
  char field[GPS_FIELD_MAX];
  uint8_t has_fix = 0U;

  if (GPS_GetField(sentence, 1U, field, sizeof(field)) != 0U)
  {
    GPS_CopyUtcTime(field);
  }

  if (GPS_GetField(sentence, 6U, field, sizeof(field)) != 0U)
  {
    BikeData.gpsFix = (field[0] != '0') && (field[0] != '\0') ? 1U : 0U;
    has_fix = BikeData.gpsFix;
  }

  if (GPS_GetField(sentence, 7U, field, sizeof(field)) != 0U)
  {
    BikeData.satelliteCount = GPS_ParseUInt8(field);
  }

  if (has_fix != 0U)
  {
    GPS_UpdateLatLon(2U, 3U, 4U, 5U, sentence);
  }

  if ((has_fix != 0U) && (GPS_GetField(sentence, 9U, field, sizeof(field)) != 0U))
  {
    BikeData.gpsAltitude = GPS_ParseFloat(field);
  }
}

static void GPS_ParseRMC(const char *sentence)
{
  char field[GPS_FIELD_MAX];

  if (GPS_GetField(sentence, 1U, field, sizeof(field)) != 0U)
  {
    GPS_CopyUtcTime(field);
  }

  if (GPS_GetField(sentence, 2U, field, sizeof(field)) != 0U)
  {
    BikeData.gpsFix = (field[0] == 'A') ? 1U : 0U;
  }

  if (BikeData.gpsFix != 0U)
  {
    GPS_UpdateLatLon(3U, 4U, 5U, 6U, sentence);
  }

  if ((BikeData.gpsFix != 0U) && (GPS_GetField(sentence, 7U, field, sizeof(field)) != 0U))
  {
    BikeData.gpsSpeed = GPS_ParseFloat(field) * GPS_KNOTS_TO_KMH;
    BikeData.speed = BikeData.gpsSpeed;
  }
  else if (BikeData.gpsFix == 0U)
  {
    BikeData.gpsSpeed = 0.0f;
    BikeData.speed = 0.0f;
  }
}

static void GPS_ParseGLL(const char *sentence)
{
  char field[GPS_FIELD_MAX];

  if (GPS_GetField(sentence, 5U, field, sizeof(field)) != 0U)
  {
    GPS_CopyUtcTime(field);
  }

  if (GPS_GetField(sentence, 6U, field, sizeof(field)) != 0U)
  {
    BikeData.gpsFix = (field[0] == 'A') ? 1U : 0U;
  }

  if (BikeData.gpsFix != 0U)
  {
    GPS_UpdateLatLon(1U, 2U, 3U, 4U, sentence);
  }
}

static void GPS_ParseSentence(const char *sentence)
{
  char type[GPS_FIELD_MAX];

  if (sentence[0] != '$')
  {
    return;
  }

  if (GPS_GetField(sentence, 0U, type, sizeof(type)) == 0U)
  {
    return;
  }

  if ((GPS_StringEquals(type, "$GNGGA") != 0U) || (GPS_StringEquals(type, "$GPGGA") != 0U))
  {
    GPS_ParseGGA(sentence);
  }
  else if ((GPS_StringEquals(type, "$GNRMC") != 0U) || (GPS_StringEquals(type, "$GPRMC") != 0U))
  {
    GPS_ParseRMC(sentence);
  }
  else if ((GPS_StringEquals(type, "$GNGLL") != 0U) || (GPS_StringEquals(type, "$GPGLL") != 0U))
  {
    GPS_ParseGLL(sentence);
  }
}

static void GPS_StartReceiveIT(void)
{
  (void)HAL_UART_Receive_IT(&huart1, &gps_rx_byte, 1U);
}

void GPS_Init(void)
{
  gps_latest_sentence[0] = '\0';
  gps_pending_sentence[0] = '\0';
  gps_sentence_ready = 0U;
  gps_parse_pending = 0U;
  GPS_ClearSentenceBuffer();
  GPS_StartReceiveIT();
}

void GPS_Update(void)
{
  char sentence[GPS_NMEA_SENTENCE_MAX];
  uint32_t primask;

  if (gps_parse_pending == 0U)
  {
    return;
  }

  primask = __get_PRIMASK();
  __disable_irq();
  GPS_CopyString(sentence, gps_pending_sentence, sizeof(sentence));
  gps_parse_pending = 0U;
  if (primask == 0U)
  {
    __enable_irq();
  }

  GPS_ParseSentence(sentence);
}

void GPS_GetLatestSentence(char *buffer, uint16_t buffer_size)
{
  uint32_t primask;

  if (buffer == 0)
  {
    return;
  }

  primask = __get_PRIMASK();
  __disable_irq();
  GPS_CopyString(buffer, gps_latest_sentence, buffer_size);
  if (primask == 0U)
  {
    __enable_irq();
  }
}

uint8_t GPS_HasSentence(void)
{
  return gps_sentence_ready;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART1)
  {
    return;
  }

  if (gps_rx_byte == '\n')
  {
    gps_sentence_buffer[gps_sentence_index] = '\0';
    if ((gps_sentence_overflow == 0U) && (gps_sentence_index > 0U))
    {
      GPS_CopyString(gps_latest_sentence, gps_sentence_buffer, GPS_NMEA_SENTENCE_MAX);
      GPS_CopyString(gps_pending_sentence, gps_sentence_buffer, GPS_NMEA_SENTENCE_MAX);
      gps_sentence_ready = 1U;
      gps_parse_pending = 1U;
    }
    GPS_ClearSentenceBuffer();
  }
  else if (gps_rx_byte != '\r')
  {
    if (gps_sentence_overflow != 0U)
    {
      /* Drop bytes until the next newline resets the sentence buffer. */
    }
    else if (gps_sentence_index < (GPS_NMEA_SENTENCE_MAX - 1U))
    {
      gps_sentence_buffer[gps_sentence_index] = (char)gps_rx_byte;
      gps_sentence_index++;
      gps_sentence_buffer[gps_sentence_index] = '\0';
    }
    else
    {
      gps_sentence_overflow = 1U;
    }
  }

  GPS_StartReceiveIT();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    GPS_StartReceiveIT();
  }
}
