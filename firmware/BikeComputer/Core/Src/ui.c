#include "ui.h"
#include "ili9341.h"
#include "bike_data.h"
#include "gps.h"
#include "ui_font.h"

#define UI_FONT_WIDTH_WITH_SPACING  6U
#define UI_FONT_HEIGHT              7U
#define UI_LABEL_FONT_SIZE          1U
#define UI_UNIT_FONT_SIZE           1U
#define UI_VALUE_FONT_MEDIUM        2U
#define UI_VALUE_FONT_LARGE         3U
#define UI_VALUE_FONT_EXTRA_LARGE   4U
#define UI_MAX_WIDGETS              8U
#define UI_VALUE_TEXT_MAX           128U
#define UI_LABEL_HEIGHT             14U
#define UI_SCREEN_BACKGROUND        ILI9341_COLOR_WHITE
#define UI_WIDGET_BACKGROUND        ILI9341_COLOR_WHITE
#define UI_TEXT_COLOR               ILI9341_COLOR_BLACK
#define UI_BORDER_COLOR             ILI9341_RGB565(190U, 196U, 202U)
#define UI_SELECTED_BACKGROUND      ILI9341_COLOR_BLACK
#define UI_SELECTED_TEXT            ILI9341_COLOR_WHITE
#define UI_CARD_MARGIN              0U
#define UI_CARD_PADDING             6U

static const Page_t *current_rendered_page = 0;
static char last_widget_values[UI_MAX_WIDGETS][UI_VALUE_TEXT_MAX];
static uint8_t last_widget_values_valid[UI_MAX_WIDGETS];
static uint8_t current_page_index = 0U;
static UI_Mode_t ui_mode = PAGE_MODE;
static uint8_t menu_cursor = 0U;
static uint8_t menu_dirty = 1U;
static uint8_t system_info_visible = 0U;

static const char *menu_items[] = {
  "Start",
  "Sensors",
  "Display",
  "System"
};

#define UI_MENU_ITEM_COUNT  ((uint8_t)(sizeof(menu_items) / sizeof(menu_items[0])))

static void UI_AppendChar(char *buffer, uint8_t buffer_size, uint8_t *index, char value)
{
  if (*index < (uint8_t)(buffer_size - 1U))
  {
    buffer[*index] = value;
    (*index)++;
    buffer[*index] = '\0';
  }
}

static void UI_AppendText(char *buffer, uint8_t buffer_size, uint8_t *index, const char *text)
{
  while (*text != '\0')
  {
    UI_AppendChar(buffer, buffer_size, index, *text);
    text++;
  }
}

static void UI_AppendUnsigned(char *buffer, uint8_t buffer_size, uint8_t *index, uint32_t value)
{
  char digits[10];
  uint8_t digit_count = 0U;

  if (value == 0U)
  {
    UI_AppendChar(buffer, buffer_size, index, '0');
    return;
  }

  while ((value > 0U) && (digit_count < sizeof(digits)))
  {
    digits[digit_count] = (char)('0' + (value % 10U));
    value /= 10U;
    digit_count++;
  }

  while (digit_count > 0U)
  {
    digit_count--;
    UI_AppendChar(buffer, buffer_size, index, digits[digit_count]);
  }
}

static void UI_AppendFloat1(char *buffer, uint8_t buffer_size, uint8_t *index, float value)
{
  uint32_t scaled;

  if (value < 0.0f)
  {
    UI_AppendChar(buffer, buffer_size, index, '-');
    value = -value;
  }

  scaled = (uint32_t)((value * 10.0f) + 0.5f);
  UI_AppendUnsigned(buffer, buffer_size, index, scaled / 10U);
  UI_AppendChar(buffer, buffer_size, index, '.');
  UI_AppendUnsigned(buffer, buffer_size, index, scaled % 10U);
}

static void UI_AppendFloat5(char *buffer, uint8_t buffer_size, uint8_t *index, float value)
{
  uint32_t scaled;
  uint32_t fraction;
  uint32_t divisor = 10000U;

  if (value < 0.0f)
  {
    UI_AppendChar(buffer, buffer_size, index, '-');
    value = -value;
  }

  scaled = (uint32_t)((value * 100000.0f) + 0.5f);
  UI_AppendUnsigned(buffer, buffer_size, index, scaled / 100000U);
  UI_AppendChar(buffer, buffer_size, index, '.');
  fraction = scaled % 100000U;

  while ((divisor > 1U) && (fraction < divisor))
  {
    UI_AppendChar(buffer, buffer_size, index, '0');
    divisor /= 10U;
  }

  UI_AppendUnsigned(buffer, buffer_size, index, fraction);
}

static void UI_AppendTwoDigits(char *buffer, uint8_t buffer_size, uint8_t *index, uint32_t value)
{
  UI_AppendChar(buffer, buffer_size, index, (char)('0' + ((value / 10U) % 10U)));
  UI_AppendChar(buffer, buffer_size, index, (char)('0' + (value % 10U)));
}

static void UI_AppendRideTime(char *buffer, uint8_t buffer_size, uint8_t *index, uint32_t seconds)
{
  uint32_t hours = seconds / 3600U;
  uint32_t minutes = (seconds / 60U) % 60U;
  uint32_t remaining_seconds = seconds % 60U;

  UI_AppendUnsigned(buffer, buffer_size, index, hours);
  UI_AppendChar(buffer, buffer_size, index, ':');
  UI_AppendTwoDigits(buffer, buffer_size, index, minutes);
  UI_AppendChar(buffer, buffer_size, index, ':');
  UI_AppendTwoDigits(buffer, buffer_size, index, remaining_seconds);
}

static void UI_GetFieldUnit(FieldType_t field, char *buffer, uint8_t buffer_size)
{
  uint8_t index = 0U;

  if (buffer_size == 0U)
  {
    return;
  }

  buffer[0] = '\0';

  switch (field)
  {
    case FIELD_SPEED:
      UI_AppendText(buffer, buffer_size, &index, "km/h");
      break;
    case FIELD_POWER:
      UI_AppendText(buffer, buffer_size, &index, "W");
      break;
    case FIELD_HEARTRATE:
      UI_AppendText(buffer, buffer_size, &index, "bpm");
      break;
    case FIELD_CADENCE:
      UI_AppendText(buffer, buffer_size, &index, "rpm");
      break;
    case FIELD_ALTITUDE:
      UI_AppendText(buffer, buffer_size, &index, "m");
      break;
    case FIELD_DISTANCE:
      UI_AppendText(buffer, buffer_size, &index, "km");
      break;
    case FIELD_MAX_SPEED:
      UI_AppendText(buffer, buffer_size, &index, "km/h");
      break;
    case FIELD_RIDE_TIME:
      break;
    case FIELD_TEMPERATURE:
    case FIELD_AMBIENT_TEMP:
      UI_AppendText(buffer, buffer_size, &index, "C");
      break;
    case FIELD_PRESSURE:
      UI_AppendText(buffer, buffer_size, &index, "hPa");
      break;
    case FIELD_BATTERY:
      UI_AppendText(buffer, buffer_size, &index, "%");
      break;
    case FIELD_GPS_SPEED:
      UI_AppendText(buffer, buffer_size, &index, "km/h");
      break;
    case FIELD_GPS_ALTITUDE:
      UI_AppendText(buffer, buffer_size, &index, "m");
      break;
    case FIELD_GPS_RAW:
      break;
    default:
      break;
  }
}

static void UI_GetFieldValue(FieldType_t field, char *buffer, uint8_t buffer_size)
{
  uint8_t index = 0U;

  if (buffer_size == 0U)
  {
    return;
  }

  buffer[0] = '\0';

  switch (field)
  {
    case FIELD_SPEED:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.speed);
      break;
    case FIELD_POWER:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.power);
      break;
    case FIELD_HEARTRATE:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.heartRate);
      break;
    case FIELD_CADENCE:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.cadence);
      break;
    case FIELD_ALTITUDE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.altitude);
      break;
    case FIELD_DISTANCE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.distance);
      break;
    case FIELD_MAX_SPEED:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.maxSpeed);
      break;
    case FIELD_RIDE_TIME:
      UI_AppendRideTime(buffer, buffer_size, &index, BikeData.rideTime);
      break;
    case FIELD_TEMPERATURE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.imuTemperature);
      break;
    case FIELD_AMBIENT_TEMP:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.ambientTemperature);
      break;
    case FIELD_PRESSURE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.pressure);
      break;
    case FIELD_PITCH:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.pitch);
      break;
    case FIELD_ROLL:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.roll);
      break;
    case FIELD_BATTERY:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.battery);
      break;
    case FIELD_GPS_FIX:
      UI_AppendText(buffer, buffer_size, &index, (BikeData.gpsFix != 0U) ? "YES" : "NO");
      break;
    case FIELD_GPS_SATELLITES:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.satelliteCount);
      break;
    case FIELD_GPS_SPEED:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.gpsSpeed);
      break;
    case FIELD_GPS_LATITUDE:
      UI_AppendFloat5(buffer, buffer_size, &index, BikeData.latitude);
      break;
    case FIELD_GPS_LONGITUDE:
      UI_AppendFloat5(buffer, buffer_size, &index, BikeData.longitude);
      break;
    case FIELD_GPS_ALTITUDE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.gpsAltitude);
      break;
    case FIELD_GPS_UTC:
      UI_AppendText(buffer, buffer_size, &index, BikeData.utcTime);
      break;
    case FIELD_GPS_RAW:
      GPS_GetLatestSentence(buffer, buffer_size);
      if (buffer[0] == '\0')
      {
        UI_AppendText(buffer, buffer_size, &index, "Waiting for NMEA...");
      }
      break;
    default:
      UI_AppendText(buffer, buffer_size, &index, "--");
      break;
  }
}

static const char *UI_GetFieldLabel(FieldType_t field)
{
  switch (field)
  {
    case FIELD_SPEED:
      return "Speed";
    case FIELD_POWER:
      return "Power";
    case FIELD_HEARTRATE:
      return "Heart Rate";
    case FIELD_CADENCE:
      return "Cadence";
    case FIELD_ALTITUDE:
      return "Altitude";
    case FIELD_DISTANCE:
      return "Distance";
    case FIELD_MAX_SPEED:
      return "Max Speed";
    case FIELD_RIDE_TIME:
      return "Ride Time";
    case FIELD_TEMPERATURE:
      return "IMU Temp";
    case FIELD_AMBIENT_TEMP:
      return "Temperature";
    case FIELD_PRESSURE:
      return "Pressure";
    case FIELD_PITCH:
      return "Pitch";
    case FIELD_ROLL:
      return "Roll";
    case FIELD_BATTERY:
      return "Battery";
    case FIELD_GPS_FIX:
      return "Fix";
    case FIELD_GPS_SATELLITES:
      return "Satellites";
    case FIELD_GPS_SPEED:
      return "GPS Speed";
    case FIELD_GPS_LATITUDE:
      return "Latitude";
    case FIELD_GPS_LONGITUDE:
      return "Longitude";
    case FIELD_GPS_ALTITUDE:
      return "GPS Altitude";
    case FIELD_GPS_UTC:
      return "UTC Time";
    case FIELD_GPS_RAW:
      return "GPS RAW";
    default:
      return "";
  }
}

static uint16_t UI_GetFieldColor(FieldType_t field)
{
  (void)field;
  return UI_TEXT_COLOR;
}

static uint16_t UI_GetWidgetBackground(const Widget_t *widget)
{
  return (widget->style != 0) ? widget->style->background_color : UI_WIDGET_BACKGROUND;
}

static uint16_t UI_GetWidgetValueColor(const Widget_t *widget)
{
  return (widget->style != 0) ? widget->style->value_color : UI_GetFieldColor(widget->field);
}

static uint16_t UI_GetWidgetLabelColor(const Widget_t *widget)
{
  return (widget->style != 0) ? widget->style->label_color : UI_TEXT_COLOR;
}

static uint16_t UI_GetWidgetBorderColor(const Widget_t *widget)
{
  return (widget->style != 0) ? widget->style->border_color : UI_BORDER_COLOR;
}

static void UI_DrawWidgetSeparators(const Widget_t *widget, uint16_t card_x, uint16_t card_y, uint16_t card_width, uint16_t card_height)
{
  uint16_t right = (uint16_t)(card_x + card_width);
  uint16_t bottom = (uint16_t)(card_y + card_height);
  uint16_t separator_color = UI_GetWidgetBorderColor(widget);

  if ((right > 0U) && (right < ILI9341_WIDTH))
  {
    ILI9341_DrawLine((int16_t)(right - 1U), (int16_t)card_y, (int16_t)(right - 1U), (int16_t)(bottom - 1U), separator_color);
  }

  if ((bottom > 0U) && (bottom < ILI9341_HEIGHT))
  {
    ILI9341_DrawLine((int16_t)card_x, (int16_t)(bottom - 1U), (int16_t)(right - 1U), (int16_t)(bottom - 1U), separator_color);
  }
}

static uint8_t UI_GetFontSize(const Widget_t *widget)
{
  if (widget->height >= 120U)
  {
    return 6U;
  }

  if (widget->height >= 92U)
  {
    return UI_VALUE_FONT_EXTRA_LARGE;
  }

  if (widget->height >= 70U)
  {
    return UI_VALUE_FONT_LARGE;
  }

  if (widget->height >= 48U)
  {
    return UI_VALUE_FONT_LARGE;
  }

  return UI_VALUE_FONT_MEDIUM;
}

static uint8_t UI_FieldUsesExtraLargeNumeric(FieldType_t field)
{
  return ((field == FIELD_SPEED) ||
          (field == FIELD_POWER) ||
          (field == FIELD_HEARTRATE)) ? 1U : 0U;
}

static uint8_t UI_FieldUsesNumericFont(FieldType_t field)
{
  switch (field)
  {
    case FIELD_SPEED:
    case FIELD_POWER:
    case FIELD_HEARTRATE:
    case FIELD_CADENCE:
    case FIELD_ALTITUDE:
    case FIELD_DISTANCE:
    case FIELD_MAX_SPEED:
    case FIELD_RIDE_TIME:
    case FIELD_TEMPERATURE:
    case FIELD_AMBIENT_TEMP:
    case FIELD_PRESSURE:
    case FIELD_PITCH:
    case FIELD_ROLL:
    case FIELD_BATTERY:
    case FIELD_GPS_SATELLITES:
    case FIELD_GPS_SPEED:
    case FIELD_GPS_LATITUDE:
    case FIELD_GPS_LONGITUDE:
    case FIELD_GPS_ALTITUDE:
    case FIELD_GPS_UTC:
      return 1U;
    default:
      return 0U;
  }
}

static const Font_t *UI_GetTextValueFont(uint16_t width, uint16_t height, const char *text)
{
  uint16_t text_width;
  uint16_t text_height;

  if (UIFont_FontCanDraw(&UIFont_MediumNumericAA, text) != 0U)
  {
    text_width = UIFont_GetStringWidthAA(&UIFont_MediumNumericAA, text);
    text_height = UIFont_GetStringHeightAA(&UIFont_MediumNumericAA);

    if ((text_width <= width) && (text_height <= height))
    {
      return &UIFont_MediumNumericAA;
    }
  }

  if (UIFont_FontCanDraw(&UIFont_SmallNumericAA, text) != 0U)
  {
    text_width = UIFont_GetStringWidthAA(&UIFont_SmallNumericAA, text);
    text_height = UIFont_GetStringHeightAA(&UIFont_SmallNumericAA);

    if ((text_width <= width) && (text_height <= height))
    {
      return &UIFont_SmallNumericAA;
    }
  }

  if (UIFont_FontCanDraw(&UIFont_LabelAA, text) != 0U)
  {
    return &UIFont_LabelAA;
  }

  return 0;
}

static uint8_t UI_GetNumericFontScale(const Widget_t *widget)
{
  if (UI_FieldUsesExtraLargeNumeric(widget->field) != 0U)
  {
    if (widget->height >= 120U)
    {
      return 6U;
    }

    if (widget->height >= 92U)
    {
      return 4U;
    }

    if (widget->height >= 70U)
    {
      return 3U;
    }

    return 2U;
  }

  if (widget->height >= 92U)
  {
    return 3U;
  }

  if (widget->height >= 64U)
  {
    return 3U;
  }

  if (widget->height >= 48U)
  {
    return 2U;
  }

  return 1U;
}

static uint16_t UI_StringLength(const char *text)
{
  uint16_t length = 0U;

  while (text[length] != '\0')
  {
    length++;
  }

  return length;
}

static uint8_t UI_StringsEqual(const char *left, const char *right)
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

static void UI_CopyString(char *destination, const char *source, uint8_t destination_size)
{
  uint8_t index = 0U;

  if (destination_size == 0U)
  {
    return;
  }

  while ((source[index] != '\0') && (index < (uint8_t)(destination_size - 1U)))
  {
    destination[index] = source[index];
    index++;
  }

  destination[index] = '\0';
}

static void UI_CopyUpperString(char *destination, const char *source, uint8_t destination_size)
{
  uint8_t index = 0U;
  char value;

  if (destination_size == 0U)
  {
    return;
  }

  while ((source[index] != '\0') && (index < (uint8_t)(destination_size - 1U)))
  {
    value = source[index];
    if ((value >= 'a') && (value <= 'z'))
    {
      value = (char)(value - ('a' - 'A'));
    }

    destination[index] = value;
    index++;
  }

  destination[index] = '\0';
}

static uint16_t UI_GetCenteredTextX(uint16_t x, uint16_t width, const char *text, uint8_t font_size)
{
  uint16_t text_width = (uint16_t)(UI_StringLength(text) * UI_FONT_WIDTH_WITH_SPACING * font_size);

  if (width <= text_width)
  {
    return x;
  }

  return (uint16_t)(x + ((width - text_width) / 2U));
}

static uint16_t UI_GetCenteredTextXAA(uint16_t x, uint16_t width, const Font_t *font, const char *text, uint8_t fallback_font_size)
{
  uint16_t text_width = UIFont_GetStringWidthAA(font, text);

  if (text_width == 0U)
  {
    return UI_GetCenteredTextX(x, width, text, fallback_font_size);
  }

  if (width <= text_width)
  {
    return x;
  }

  return (uint16_t)(x + ((width - text_width) / 2U));
}

static void UI_DrawWrappedText(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char *text, uint16_t color, uint16_t background, uint8_t font_size)
{
  uint16_t char_width = (uint16_t)(UI_FONT_WIDTH_WITH_SPACING * font_size);
  uint16_t char_height = (uint16_t)(UI_FONT_HEIGHT * font_size);
  uint16_t max_columns;
  uint16_t column = 0U;
  uint16_t cursor_y = y;

  if ((char_width == 0U) || (char_height == 0U) || (width < char_width))
  {
    return;
  }

  max_columns = width / char_width;

  while ((*text != '\0') && ((uint16_t)(cursor_y + char_height) <= (uint16_t)(y + height)))
  {
    if ((*text == '\n') || (column >= max_columns))
    {
      column = 0U;
      cursor_y = (uint16_t)(cursor_y + char_height + 2U);
      if (*text == '\n')
      {
        text++;
      }
      continue;
    }

    if (*text != '\r')
    {
      ILI9341_DrawChar((uint16_t)(x + (column * char_width)), cursor_y, *text, color, background, font_size);
      column++;
    }

    text++;
  }
}

static void UI_ClearValueCache(void)
{
  for (uint8_t i = 0U; i < UI_MAX_WIDGETS; i++)
  {
    last_widget_values[i][0] = '\0';
    last_widget_values_valid[i] = 0U;
  }
}

static void UI_RenderWidgetLabel(const Widget_t *widget)
{
  char label[16];
  char unit[8];
  uint16_t card_x = (uint16_t)(widget->x + UI_CARD_MARGIN);
  uint16_t card_y = (uint16_t)(widget->y + UI_CARD_MARGIN);
  uint16_t card_width = (widget->width > (UI_CARD_MARGIN * 2U)) ? (uint16_t)(widget->width - (UI_CARD_MARGIN * 2U)) : widget->width;
  uint16_t card_height = (widget->height > (UI_CARD_MARGIN * 2U)) ? (uint16_t)(widget->height - (UI_CARD_MARGIN * 2U)) : widget->height;
  uint16_t label_x = (uint16_t)(card_x + UI_CARD_PADDING);
  uint16_t unit_x;
  uint16_t background = UI_GetWidgetBackground(widget);
  uint16_t label_color = UI_GetWidgetLabelColor(widget);

  UI_CopyUpperString(label, UI_GetFieldLabel(widget->field), sizeof(label));
  ILI9341_FillRectangle(card_x, card_y, card_width, card_height, background);
  UI_DrawWidgetSeparators(widget, card_x, card_y, card_width, card_height);
  UIFont_DrawBoldString(label_x, (uint16_t)(card_y + UI_CARD_PADDING), label, label_color, background, UI_LABEL_FONT_SIZE);

  UI_GetFieldUnit(widget->field, unit, sizeof(unit));
  if (unit[0] != '\0')
  {
    unit_x = UI_GetCenteredTextXAA(card_x, card_width, &UIFont_LabelAA, unit, UI_UNIT_FONT_SIZE);
    UIFont_DrawBoldString(unit_x, (uint16_t)(card_y + card_height - 15U), unit, label_color, background, UI_UNIT_FONT_SIZE);
  }
}

static void UI_RenderWidgetValue(const Widget_t *widget, uint8_t widget_index, uint8_t force_redraw)
{
  char text[UI_VALUE_TEXT_MAX];
  uint8_t font_size = UI_GetFontSize(widget);
  uint8_t numeric_scale = UI_GetNumericFontScale(widget);
  uint8_t use_numeric_font = 0U;
  const Font_t *text_value_font = 0;
  uint16_t card_x = (uint16_t)(widget->x + UI_CARD_MARGIN);
  uint16_t card_y = (uint16_t)(widget->y + UI_CARD_MARGIN);
  uint16_t card_width = (widget->width > (UI_CARD_MARGIN * 2U)) ? (uint16_t)(widget->width - (UI_CARD_MARGIN * 2U)) : widget->width;
  uint16_t card_height = (widget->height > (UI_CARD_MARGIN * 2U)) ? (uint16_t)(widget->height - (UI_CARD_MARGIN * 2U)) : widget->height;
  uint16_t value_y = (uint16_t)(card_y + UI_LABEL_HEIGHT + UI_CARD_PADDING);
  uint16_t value_height = (card_height > 38U) ? (uint16_t)(card_height - 38U) : card_height;
  uint16_t value_width = (card_width > (UI_CARD_PADDING * 2U)) ? (uint16_t)(card_width - (UI_CARD_PADDING * 2U)) : card_width;
  uint16_t text_height = (uint16_t)(UI_FONT_HEIGHT * font_size);
  uint16_t text_width;
  uint16_t text_x;
  uint16_t text_y = value_y;
  uint16_t background = UI_GetWidgetBackground(widget);
  uint16_t value_color = UI_GetWidgetValueColor(widget);

  UI_GetFieldValue(widget->field, text, sizeof(text));

  if ((force_redraw == 0U) &&
      (widget_index < UI_MAX_WIDGETS) &&
      (last_widget_values_valid[widget_index] != 0U) &&
      (UI_StringsEqual(last_widget_values[widget_index], text) != 0U))
  {
    return;
  }

  use_numeric_font = ((UI_FieldUsesNumericFont(widget->field) != 0U) &&
                      (UIFont_IsNumericText(text) != 0U)) ? 1U : 0U;

  if (use_numeric_font != 0U)
  {
    text_width = UIFont_GetNumericTextWidth(text, numeric_scale);
    text_height = UIFont_GetNumericTextHeight(numeric_scale);

    while ((numeric_scale > 1U) &&
           ((text_width > value_width) || (text_height > value_height)))
    {
      numeric_scale--;
      text_width = UIFont_GetNumericTextWidth(text, numeric_scale);
      text_height = UIFont_GetNumericTextHeight(numeric_scale);
    }

    text_x = (card_width > text_width) ? (uint16_t)(card_x + ((card_width - text_width) / 2U)) : card_x;
  }
  else
  {
    text_value_font = UI_GetTextValueFont(value_width, value_height, text);

    if (text_value_font != 0)
    {
      text_width = UIFont_GetStringWidthAA(text_value_font, text);
      text_height = UIFont_GetStringHeightAA(text_value_font);
      text_x = (card_width > text_width) ? (uint16_t)(card_x + ((card_width - text_width) / 2U)) : card_x;
    }
    else
    {
      text_x = UI_GetCenteredTextX(card_x, card_width, text, font_size);

      while ((font_size > 1U) &&
             ((uint16_t)(UI_StringLength(text) * UI_FONT_WIDTH_WITH_SPACING * font_size) > value_width))
      {
        font_size--;
        text_height = (uint16_t)(UI_FONT_HEIGHT * font_size);
        text_x = UI_GetCenteredTextX(card_x, card_width, text, font_size);
      }
    }
  }

  if (value_height > text_height)
  {
    text_y = (uint16_t)(value_y + ((value_height - text_height) / 2U));
  }

  ILI9341_FillRectangle(card_x, value_y, card_width, value_height, background);
  UI_DrawWidgetSeparators(widget, card_x, card_y, card_width, card_height);
  if (widget->field == FIELD_GPS_RAW)
  {
    UI_DrawWrappedText((uint16_t)(card_x + UI_CARD_PADDING),
                       value_y,
                       (uint16_t)(card_width - (UI_CARD_PADDING * 2U)),
                       value_height,
                       text,
                       value_color,
                       background,
                       1U);
  }
  else if (use_numeric_font != 0U)
  {
    UIFont_DrawNumericString(text_x, text_y, text, value_color, background, numeric_scale);
  }
  else if (text_value_font != 0)
  {
    UIFont_DrawStringAA(text_x, text_y, text_value_font, text, value_color, background);
  }
  else
  {
    ILI9341_DrawString(text_x, text_y, text, value_color, background, font_size);
  }

  if (widget_index < UI_MAX_WIDGETS)
  {
    UI_CopyString(last_widget_values[widget_index], text, UI_VALUE_TEXT_MAX);
    last_widget_values_valid[widget_index] = 1U;
  }
}

void RenderPage(const Page_t *page)
{
  uint8_t page_changed;
  uint8_t widget_count;

  if ((page == 0) || (page->widgets == 0))
  {
    return;
  }

  page_changed = (page != current_rendered_page) ? 1U : 0U;
  widget_count = (page->widget_count > UI_MAX_WIDGETS) ? UI_MAX_WIDGETS : page->widget_count;

  if (page_changed != 0U)
  {
    current_rendered_page = page;
    UI_ClearValueCache();
    ILI9341_FillScreen(UI_SCREEN_BACKGROUND);

    for (uint8_t i = 0U; i < widget_count; i++)
    {
      UI_RenderWidgetLabel(&page->widgets[i]);
    }
  }

  for (uint8_t i = 0U; i < widget_count; i++)
  {
    UI_RenderWidgetValue(&page->widgets[i], i, page_changed);
  }
}

void UI_RenderCurrentPage(void)
{
  if (ui_mode == MENU_MODE)
  {
    if (menu_dirty == 0U)
    {
      return;
    }

    ILI9341_FillScreen(UI_SCREEN_BACKGROUND);
    ILI9341_DrawRectangle(8U, 8U, 224U, 304U, UI_BORDER_COLOR);
    ILI9341_DrawString(82U, 22U, "MENU", UI_TEXT_COLOR, UI_SCREEN_BACKGROUND, 3U);
    ILI9341_DrawLine(8, 62, 231, 62, UI_BORDER_COLOR);

    for (uint8_t i = 0U; i < UI_MENU_ITEM_COUNT; i++)
    {
      uint16_t y = (uint16_t)(92U + (i * 44U));
      uint16_t color = (i == menu_cursor) ? UI_SELECTED_TEXT : UI_TEXT_COLOR;
      uint16_t background = (i == menu_cursor) ? UI_SELECTED_BACKGROUND : UI_SCREEN_BACKGROUND;

      if (i == menu_cursor)
      {
        ILI9341_FillRectangle(24U, (uint16_t)(y - 6U), 192U, 30U, UI_SELECTED_BACKGROUND);
      }

      ILI9341_DrawString(40U, y, menu_items[i], color, background, 2U);
    }

    if (system_info_visible != 0U)
    {
      ILI9341_DrawString(24U, 278U, "BikeComputer v1.0", UI_TEXT_COLOR, UI_SCREEN_BACKGROUND, 2U);
    }

    menu_dirty = 0U;
    return;
  }

  if (DemoPageCount == 0U)
  {
    return;
  }

  if (current_page_index >= DemoPageCount)
  {
    current_page_index = 0U;
  }

  RenderPage(&DemoPages[current_page_index]);
}

UI_Mode_t UI_GetMode(void)
{
  return ui_mode;
}

void UI_NextPage(void)
{
  if (DemoPageCount == 0U)
  {
    return;
  }

  current_page_index++;
  if (current_page_index >= DemoPageCount)
  {
    current_page_index = 0U;
  }

  UI_ForceRedraw();
}

void UI_PreviousPage(void)
{
  if (DemoPageCount == 0U)
  {
    return;
  }

  if (current_page_index == 0U)
  {
    current_page_index = (uint8_t)(DemoPageCount - 1U);
  }
  else
  {
    current_page_index--;
  }

  UI_ForceRedraw();
}

void UI_EnterMenu(void)
{
  ui_mode = MENU_MODE;
  menu_cursor = 0U;
  system_info_visible = 0U;
  menu_dirty = 1U;
}

void UI_MenuNext(void)
{
  if (ui_mode != MENU_MODE)
  {
    return;
  }

  system_info_visible = 0U;
  menu_cursor++;
  if (menu_cursor >= UI_MENU_ITEM_COUNT)
  {
    menu_cursor = 0U;
  }

  menu_dirty = 1U;
}

void UI_MenuPrev(void)
{
  if (ui_mode != MENU_MODE)
  {
    return;
  }

  system_info_visible = 0U;
  if (menu_cursor == 0U)
  {
    menu_cursor = (uint8_t)(UI_MENU_ITEM_COUNT - 1U);
  }
  else
  {
    menu_cursor--;
  }

  menu_dirty = 1U;
}

void UI_MenuSelect(void)
{
  if (ui_mode != MENU_MODE)
  {
    return;
  }

  if (menu_cursor == 0U)
  {
    ui_mode = PAGE_MODE;
    system_info_visible = 0U;
    UI_ForceRedraw();
    return;
  }

  if (menu_cursor == 3U)
  {
    system_info_visible = 1U;
  }

  menu_dirty = 1U;
}

void UI_ForceRedraw(void)
{
  current_rendered_page = 0;
  UI_ClearValueCache();
  menu_dirty = 1U;
}
