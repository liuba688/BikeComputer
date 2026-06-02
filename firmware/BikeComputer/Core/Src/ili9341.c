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

  data = 0x55U; /* 16-bit RGB565 pixels */
  ILI9341_WriteCommand(ILI9341_COLMOD);
  ILI9341_WriteData(&data, 1U);
  HAL_Delay(10U);

  data = 0x48U;
  ILI9341_WriteCommand(ILI9341_MADCTL);
  ILI9341_WriteData(&data, 1U);

  ILI9341_WriteCommand(ILI9341_DISPON);
  HAL_Delay(100U);
}

void ILI9341_FillScreen(uint16_t color)
{
  uint8_t data[2];

  data[0] = (uint8_t)(color >> 8);
  data[1] = (uint8_t)(color & 0xFFU);

  ILI9341_SetAddressWindow(0U, 0U, ILI9341_WIDTH - 1U, ILI9341_HEIGHT - 1U);

  ILI9341_Select();
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);

  for (uint32_t i = 0U; i < (ILI9341_WIDTH * ILI9341_HEIGHT); i++)
  {
    HAL_SPI_Transmit(&hspi1, data, sizeof(data), ILI9341_SPI_TIMEOUT_MS);
  }

  ILI9341_Unselect();
}
