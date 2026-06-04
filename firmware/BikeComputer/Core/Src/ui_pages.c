#include "ui.h"

static const Widget_t DemoPage1Widgets[] = {
  {0U,  12U, 240U, 82U, FIELD_SPEED},
  {0U,  94U, 240U, 68U, FIELD_POWER},
  {0U, 162U, 240U, 68U, FIELD_HEARTRATE},
  {0U, 230U, 240U, 68U, FIELD_CADENCE}
};

static const Widget_t DemoPage2Widgets[] = {
  {0U,  30U, 240U, 78U, FIELD_AMBIENT_TEMP},
  {0U, 122U, 240U, 78U, FIELD_PRESSURE},
  {0U, 214U, 240U, 72U, FIELD_ALTITUDE}
};

static const Widget_t DemoPage3Widgets[] = {
  {0U,  30U, 240U, 78U, FIELD_PITCH},
  {0U, 122U, 240U, 78U, FIELD_ROLL},
  {0U, 214U, 240U, 72U, FIELD_TEMPERATURE}
};

const Page_t DemoPages[] = {
  {DemoPage1Widgets, (uint8_t)(sizeof(DemoPage1Widgets) / sizeof(DemoPage1Widgets[0]))},
  {DemoPage2Widgets, (uint8_t)(sizeof(DemoPage2Widgets) / sizeof(DemoPage2Widgets[0]))},
  {DemoPage3Widgets, (uint8_t)(sizeof(DemoPage3Widgets) / sizeof(DemoPage3Widgets[0]))}
};

const uint8_t DemoPageCount = (uint8_t)(sizeof(DemoPages) / sizeof(DemoPages[0]));
