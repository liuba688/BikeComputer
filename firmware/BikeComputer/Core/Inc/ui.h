#ifndef __UI_H
#define __UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  FIELD_SPEED = 0,
  FIELD_POWER,
  FIELD_HEARTRATE,
  FIELD_CADENCE,
  FIELD_ALTITUDE,
  FIELD_DISTANCE,
  FIELD_TEMPERATURE,
  FIELD_PITCH,
  FIELD_ROLL,
  FIELD_BATTERY,
  FIELD_COUNT
} FieldType_t;

typedef struct
{
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  FieldType_t field;
} Widget_t;

typedef struct
{
  const Widget_t *widgets;
  uint8_t widget_count;
} Page_t;

void RenderPage(const Page_t *page);

extern const Page_t DemoPages[];
extern const uint8_t DemoPageCount;

#ifdef __cplusplus
}
#endif

#endif /* __UI_H */
