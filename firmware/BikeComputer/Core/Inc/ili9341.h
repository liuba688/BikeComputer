#ifndef __ILI9341_H
#define __ILI9341_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#define ILI9341_WIDTH   240U
#define ILI9341_HEIGHT  320U

#define ILI9341_COLOR_RED  0xF800U

void ILI9341_WriteCommand(uint8_t command);
void ILI9341_WriteData(uint8_t *data, uint16_t size);
void ILI9341_Reset(void);
void ILI9341_Init(void);
void ILI9341_FillScreen(uint16_t color);

#ifdef __cplusplus
}
#endif

#endif /* __ILI9341_H */
