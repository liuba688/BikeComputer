#ifndef __ILI9341_H
#define __ILI9341_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#define ILI9341_WIDTH   240U
#define ILI9341_HEIGHT  320U

#define ILI9341_RGB565(r, g, b)  ((uint16_t)((((uint16_t)(r) & 0xF8U) << 8) | (((uint16_t)(g) & 0xF8U) << 3) | (((uint16_t)(b) & 0xF8U) >> 3)))

#define ILI9341_COLOR_BLACK    0x0000U
#define ILI9341_COLOR_WHITE    0xFFFFU
#define ILI9341_COLOR_RED      0xF800U
#define ILI9341_COLOR_GREEN    0x07E0U
#define ILI9341_COLOR_BLUE     0x001FU
#define ILI9341_COLOR_CYAN     0x07FFU
#define ILI9341_COLOR_MAGENTA  0xF81FU
#define ILI9341_COLOR_YELLOW   0xFFE0U
#define ILI9341_COLOR_ORANGE   0xFD20U
#define ILI9341_COLOR_GRAY     0x8410U

void ILI9341_WriteCommand(uint8_t command);
void ILI9341_WriteData(uint8_t *data, uint16_t size);
void ILI9341_Reset(void);
void ILI9341_Init(void);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void ILI9341_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void ILI9341_FillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void ILI9341_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t background, uint8_t size);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t background, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* __ILI9341_H */
