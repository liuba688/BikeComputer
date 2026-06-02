#include "ili9341.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

#define ILI9341_SWRESET  0x01U
#define ILI9341_SLPOUT   0x11U
#define ILI9341_DISPON   0x29U
#define ILI9341_CASET    0x2AU
#define ILI9341_PASET    0x2BU
#define ILI9341_RAMWR    0x2CU
#define ILI9341_MADCTL   0x36U
#define ILI9341_COLMOD   0x3AU

#define ILI9341_SPI_TIMEOUT_MS  100U
#define ILI9341_FONT_WIDTH      5U
#define ILI9341_FONT_HEIGHT     7U

static const uint8_t Font5x7[96][5] = {
  {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
  {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
  {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
  {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
  {0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},{0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},
  {0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},
  {0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
  {0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},
  {0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
  {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
  {0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},
  {0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},
  {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},{0x7F,0x10,0x28,0x44,0x00},
  {0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},
  {0x7C,0x14,0x14,0x14,0x08},{0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
  {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},
  {0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},
  {0x00,0x00,0x7F,0x00,0x00},{0x00,0x41,0x36,0x08,0x00},{0x10,0x08,0x08,0x10,0x08},{0x00,0x00,0x00,0x00,0x00}
};

static void ILI9341_Select(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

static void ILI9341_Unselect(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

static void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  uint8_t data[4];

  data[0] = (uint8_t)(x0 >> 8);
  data[1] = (uint8_t)(x0 & 0xFFU);
  data[2] = (uint8_t)(x1 >> 8);
  data[3] = (uint8_t)(x1 & 0xFFU);
  ILI9341_WriteCommand(ILI9341_CASET);
  ILI9341_WriteData(data, sizeof(data));

  data[0] = (uint8_t)(y0 >> 8);
  data[1] = (uint8_t)(y0 & 0xFFU);
  data[2] = (uint8_t)(y1 >> 8);
  data[3] = (uint8_t)(y1 & 0xFFU);
  ILI9341_WriteCommand(ILI9341_PASET);
  ILI9341_WriteData(data, sizeof(data));

  ILI9341_WriteCommand(ILI9341_RAMWR);
}

static void ILI9341_WriteColorPixels(uint16_t color, uint32_t count)
{
  uint8_t buffer[64];

  for (uint16_t i = 0U; i < sizeof(buffer); i += 2U)
  {
    buffer[i] = (uint8_t)(color >> 8);
    buffer[i + 1U] = (uint8_t)(color & 0xFFU);
  }

  ILI9341_Select();
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);

  while (count > 0U)
  {
    uint16_t pixels = (count > (sizeof(buffer) / 2U)) ? (sizeof(buffer) / 2U) : (uint16_t)count;
    HAL_SPI_Transmit(&hspi1, buffer, (uint16_t)(pixels * 2U), ILI9341_SPI_TIMEOUT_MS);
    count -= pixels;
  }

  ILI9341_Unselect();
}

void ILI9341_WriteCommand(uint8_t command)
{
  ILI9341_Select();
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, &command, 1U, ILI9341_SPI_TIMEOUT_MS);
  ILI9341_Unselect();
}

void ILI9341_WriteData(uint8_t *data, uint16_t size)
{
  ILI9341_Select();
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_SPI_Transmit(&hspi1, data, size, ILI9341_SPI_TIMEOUT_MS);
  ILI9341_Unselect();
}

void ILI9341_Reset(void)
{
  ILI9341_Unselect();
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(120U);
}

void ILI9341_Init(void)
{
  uint8_t data;

  ILI9341_Reset();

  ILI9341_WriteCommand(ILI9341_SWRESET);
  HAL_Delay(150U);

  ILI9341_WriteCommand(ILI9341_SLPOUT);
  HAL_Delay(120U);

  data = 0x55U;
  ILI9341_WriteCommand(ILI9341_COLMOD);
  ILI9341_WriteData(&data, 1U);
  HAL_Delay(10U);

  data = 0x48U;
  ILI9341_WriteCommand(ILI9341_MADCTL);
  ILI9341_WriteData(&data, 1U);

  ILI9341_WriteCommand(ILI9341_DISPON);
  HAL_Delay(100U);
}

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  uint8_t data[2];

  if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
  {
    return;
  }

  data[0] = (uint8_t)(color >> 8);
  data[1] = (uint8_t)(color & 0xFFU);

  ILI9341_SetAddressWindow(x, y, x, y);
  ILI9341_WriteData(data, sizeof(data));
}

void ILI9341_FillScreen(uint16_t color)
{
  ILI9341_FillRectangle(0U, 0U, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = dx + dy;

  while (1)
  {
    if ((x0 >= 0) && (y0 >= 0))
    {
      ILI9341_DrawPixel((uint16_t)x0, (uint16_t)y0, color);
    }

    if ((x0 == x1) && (y0 == y1))
    {
      break;
    }

    int16_t e2 = (int16_t)(2 * err);
    if (e2 >= dy)
    {
      err = (int16_t)(err + dy);
      x0 = (int16_t)(x0 + sx);
    }
    if (e2 <= dx)
    {
      err = (int16_t)(err + dx);
      y0 = (int16_t)(y0 + sy);
    }
  }
}

void ILI9341_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  if ((width == 0U) || (height == 0U))
  {
    return;
  }

  ILI9341_DrawLine((int16_t)x, (int16_t)y, (int16_t)(x + width - 1U), (int16_t)y, color);
  ILI9341_DrawLine((int16_t)x, (int16_t)(y + height - 1U), (int16_t)(x + width - 1U), (int16_t)(y + height - 1U), color);
  ILI9341_DrawLine((int16_t)x, (int16_t)y, (int16_t)x, (int16_t)(y + height - 1U), color);
  ILI9341_DrawLine((int16_t)(x + width - 1U), (int16_t)y, (int16_t)(x + width - 1U), (int16_t)(y + height - 1U), color);
}

void ILI9341_FillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  if ((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT) || (width == 0U) || (height == 0U))
  {
    return;
  }

  if ((x + width) > ILI9341_WIDTH)
  {
    width = ILI9341_WIDTH - x;
  }

  if ((y + height) > ILI9341_HEIGHT)
  {
    height = ILI9341_HEIGHT - y;
  }

  ILI9341_SetAddressWindow(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
  ILI9341_WriteColorPixels(color, (uint32_t)width * (uint32_t)height);
}

void ILI9341_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t background, uint8_t size)
{
  uint8_t c;

  if ((size == 0U) || (x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
  {
    return;
  }

  c = ((ch < ' ') || (ch > '~')) ? 0U : (uint8_t)(ch - ' ');

  for (uint8_t col = 0U; col < ILI9341_FONT_WIDTH; col++)
  {
    uint8_t line = Font5x7[c][col];
    for (uint8_t row = 0U; row < ILI9341_FONT_HEIGHT; row++)
    {
      uint16_t draw_color = ((line & 0x01U) != 0U) ? color : background;
      if (size == 1U)
      {
        ILI9341_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), draw_color);
      }
      else
      {
        ILI9341_FillRectangle((uint16_t)(x + (col * size)), (uint16_t)(y + (row * size)), size, size, draw_color);
      }
      line >>= 1;
    }
  }

  if (background != color)
  {
    if (size == 1U)
    {
      ILI9341_FillRectangle((uint16_t)(x + ILI9341_FONT_WIDTH), y, 1U, ILI9341_FONT_HEIGHT, background);
    }
    else
    {
      ILI9341_FillRectangle((uint16_t)(x + (ILI9341_FONT_WIDTH * size)), y, size, (uint16_t)(ILI9341_FONT_HEIGHT * size), background);
    }
  }
}

void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t background, uint8_t size)
{
  uint16_t cursor_x = x;
  uint16_t cursor_y = y;
  uint16_t char_width;
  uint16_t char_height;

  if ((str == 0) || (size == 0U))
  {
    return;
  }

  char_width = (uint16_t)((ILI9341_FONT_WIDTH + 1U) * size);
  char_height = (uint16_t)(ILI9341_FONT_HEIGHT * size);

  while (*str != '\0')
  {
    if (*str == '\n')
    {
      cursor_x = x;
      cursor_y = (uint16_t)(cursor_y + char_height + size);
    }
    else if (*str == '\r')
    {
      cursor_x = x;
    }
    else
    {
      if ((cursor_x + char_width) > ILI9341_WIDTH)
      {
        cursor_x = x;
        cursor_y = (uint16_t)(cursor_y + char_height + size);
      }

      if ((cursor_y + char_height) > ILI9341_HEIGHT)
      {
        break;
      }

      ILI9341_DrawChar(cursor_x, cursor_y, *str, color, background, size);
      cursor_x = (uint16_t)(cursor_x + char_width);
    }

    str++;
  }
}
