#include "ui.h"
#include "ili9341.h"

#define PAGE1_BLUE_BACKGROUND  ILI9341_RGB565(86U, 137U, 163U)
#define PAGE1_DIVIDER_COLOR    ILI9341_RGB565(190U, 196U, 202U)

static const WidgetStyle_t Page1SpeedStyle = {
  PAGE1_BLUE_BACKGROUND,
  ILI9341_COLOR_WHITE,
  ILI9341_COLOR_WHITE,
  PAGE1_DIVIDER_COLOR
};

static const WidgetStyle_t Page1PowerStyle = {
  ILI9341_COLOR_BLACK,
  ILI9341_COLOR_WHITE,
  ILI9341_COLOR_WHITE,
  PAGE1_DIVIDER_COLOR
};

static const WidgetStyle_t Page1DefaultStyle = {
  ILI9341_COLOR_WHITE,
  ILI9341_COLOR_BLACK,
  ILI9341_COLOR_BLACK,
  PAGE1_DIVIDER_COLOR
};

static const Widget_t DemoPage1Widgets[] = {
  {0U,   0U, 240U, 132U, FIELD_SPEED, &Page1SpeedStyle},
  {0U, 132U, 120U,  94U, FIELD_POWER, &Page1PowerStyle},
  {120U, 132U, 120U,  94U, FIELD_HEARTRATE, &Page1DefaultStyle},
  {0U, 226U, 120U,  94U, FIELD_CADENCE, &Page1DefaultStyle},
  {120U, 226U, 120U,  94U, FIELD_AMBIENT_TEMP, &Page1DefaultStyle}
};

static const Widget_t RideStatsPageWidgets[] = {
  {0U,   0U, 240U, 80U, FIELD_SPEED, 0},
  {0U,  80U, 240U, 80U, FIELD_DISTANCE, 0},
  {0U, 160U, 240U, 80U, FIELD_RIDE_TIME, 0},
  {0U, 240U, 240U, 80U, FIELD_MAX_SPEED, 0}
};

static const Widget_t DemoPage2Widgets[] = {
  {0U,  30U, 240U, 78U, FIELD_AMBIENT_TEMP, 0},
  {0U, 122U, 240U, 78U, FIELD_PRESSURE, 0},
  {0U, 214U, 240U, 72U, FIELD_ALTITUDE, 0}
};

static const Widget_t DemoPage3Widgets[] = {
  {0U,  30U, 240U, 78U, FIELD_PITCH, 0},
  {0U, 122U, 240U, 78U, FIELD_ROLL, 0},
  {0U, 214U, 240U, 72U, FIELD_TEMPERATURE, 0}
};

static const Widget_t GpsPageWidgets[] = {
  {0U,    0U, 120U,  64U, FIELD_GPS_FIX, 0},
  {120U,  0U, 120U,  64U, FIELD_GPS_SATELLITES, 0},
  {0U,   64U, 240U,  74U, FIELD_GPS_SPEED, 0},
  {0U,  138U, 240U,  91U, FIELD_GPS_LATITUDE, 0},
  {0U,  229U, 240U,  91U, FIELD_GPS_LONGITUDE, 0}
};

static const Widget_t GpsDebugPageWidgets[] = {
  {0U, 0U, 240U, 320U, FIELD_GPS_RAW, 0}
};

const Page_t DemoPages[] = {
  {DemoPage1Widgets, (uint8_t)(sizeof(DemoPage1Widgets) / sizeof(DemoPage1Widgets[0]))},
  {RideStatsPageWidgets, (uint8_t)(sizeof(RideStatsPageWidgets) / sizeof(RideStatsPageWidgets[0]))},
  {DemoPage2Widgets, (uint8_t)(sizeof(DemoPage2Widgets) / sizeof(DemoPage2Widgets[0]))},
  {DemoPage3Widgets, (uint8_t)(sizeof(DemoPage3Widgets) / sizeof(DemoPage3Widgets[0]))},
  {GpsPageWidgets, (uint8_t)(sizeof(GpsPageWidgets) / sizeof(GpsPageWidgets[0]))},
  {GpsDebugPageWidgets, (uint8_t)(sizeof(GpsDebugPageWidgets) / sizeof(GpsDebugPageWidgets[0]))}
};

const uint8_t DemoPageCount = (uint8_t)(sizeof(DemoPages) / sizeof(DemoPages[0]));
