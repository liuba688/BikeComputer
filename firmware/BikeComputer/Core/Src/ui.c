#include "ui.h"
#include "ili9341.h"
#include "bike_data.h"

#define UI_FONT_WIDTH_WITH_SPACING  6U
#define UI_FONT_HEIGHT              7U
#define UI_MAX_WIDGETS              8U
#define UI_VALUE_TEXT_MAX           24U
#define UI_LABEL_HEIGHT             14U

static const Page_t *current_rendered_page = 0;
static char last_widget_values[UI_MAX_WIDGETS][UI_VALUE_TEXT_MAX];
static uint8_t last_widget_values_valid[UI_MAX_WIDGETS];

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

static void UI_GetFieldText(FieldType_t field, char *buffer, uint8_t buffer_size)
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
      UI_AppendText(buffer, buffer_size, &index, " km/h");
      break;
    case FIELD_POWER:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.power);
      UI_AppendText(buffer, buffer_size, &index, " W");
      break;
    case FIELD_HEARTRATE:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.heartRate);
      UI_AppendText(buffer, buffer_size, &index, " bpm");
      break;
    case FIELD_CADENCE:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.cadence);
      UI_AppendText(buffer, buffer_size, &index, " rpm");
      break;
    case FIELD_ALTITUDE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.altitude);
      UI_AppendText(buffer, buffer_size, &index, " m");
      break;
    case FIELD_DISTANCE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.distance);
      UI_AppendText(buffer, buffer_size, &index, " km");
      break;
    case FIELD_TEMPERATURE:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.temperature);
      UI_AppendText(buffer, buffer_size, &index, " C");
      break;
    case FIELD_PITCH:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.pitch);
      UI_AppendText(buffer, buffer_size, &index, " pitch");
      break;
    case FIELD_ROLL:
      UI_AppendFloat1(buffer, buffer_size, &index, BikeData.roll);
      UI_AppendText(buffer, buffer_size, &index, " roll");
      break;
    case FIELD_BATTERY:
      UI_AppendUnsigned(buffer, buffer_size, &index, BikeData.battery);
      UI_AppendText(buffer, buffer_size, &index, "%");
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
    case FIELD_TEMPERATURE:
      return "Temperature";
    case FIELD_PITCH:
      return "Pitch";
    case FIELD_ROLL:
      return "Roll";
    case FIELD_BATTERY:
      return "Battery";
    default:
      return "";
  }
}

static uint16_t UI_GetFieldColor(FieldType_t field)
{
  switch (field)
  {
    case FIELD_POWER:
      return ILI9341_COLOR_YELLOW;
    case FIELD_HEARTRATE:
      return ILI9341_COLOR_RED;
    case FIELD_CADENCE:
      return ILI9341_COLOR_CYAN;
    case FIELD_ALTITUDE:
      return ILI9341_COLOR_GREEN;
    case FIELD_DISTANCE:
      return ILI9341_COLOR_ORANGE;
    case FIELD_TEMPERATURE:
      return ILI9341_COLOR_MAGENTA;
    case FIELD_PITCH:
      return ILI9341_COLOR_BLUE;
    case FIELD_ROLL:
      return ILI9341_COLOR_GRAY;
    case FIELD_BATTERY:
      return ILI9341_COLOR_GREEN;
    default:
      return ILI9341_COLOR_WHITE;
  }
}

static uint8_t UI_GetFontSize(const Widget_t *widget)
{
  if (widget->height >= 80U)
  {
    return 4U;
  }

  if (widget->height >= 56U)
  {
    return 3U;
  }

  return 2U;
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

static uint16_t UI_GetCenteredTextX(uint16_t x, uint16_t width, const char *text, uint8_t font_size)
{
  uint16_t text_width = (uint16_t)(UI_StringLength(text) * UI_FONT_WIDTH_WITH_SPACING * font_size);

  if (width <= text_width)
  {
    return x;
  }

  return (uint16_t)(x + ((width - text_width) / 2U));
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
  const char *label = UI_GetFieldLabel(widget->field);
  uint16_t label_x = UI_GetCenteredTextX(widget->x, widget->width, label, 1U);

  ILI9341_DrawString(label_x, widget->y, label, ILI9341_COLOR_GRAY, ILI9341_COLOR_BLACK, 1U);
}

static void UI_RenderWidgetValue(const Widget_t *widget, uint8_t widget_index, uint8_t force_redraw)
{
  char text[24];
  uint8_t font_size = UI_GetFontSize(widget);
  uint16_t value_y = (uint16_t)(widget->y + UI_LABEL_HEIGHT);
  uint16_t value_height = (widget->height > UI_LABEL_HEIGHT) ? (uint16_t)(widget->height - UI_LABEL_HEIGHT) : widget->height;
  uint16_t text_height = (uint16_t)(UI_FONT_HEIGHT * font_size);
  uint16_t text_x;
  uint16_t text_y = value_y;

  UI_GetFieldText(widget->field, text, sizeof(text));

  if ((force_redraw == 0U) &&
      (widget_index < UI_MAX_WIDGETS) &&
      (last_widget_values_valid[widget_index] != 0U) &&
      (UI_StringsEqual(last_widget_values[widget_index], text) != 0U))
  {
    return;
  }

  text_x = UI_GetCenteredTextX(widget->x, widget->width, text, font_size);

  if (value_height > text_height)
  {
    text_y = (uint16_t)(value_y + ((value_height - text_height) / 2U));
  }

  ILI9341_FillRectangle(widget->x, value_y, widget->width, value_height, ILI9341_COLOR_BLACK);
  ILI9341_DrawString(text_x, text_y, text, UI_GetFieldColor(widget->field), ILI9341_COLOR_BLACK, font_size);

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
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);

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
