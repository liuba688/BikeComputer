#ifndef __BUTTON_H
#define __BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum
{
  BUTTON_NEXT = 0,
  BUTTON_PREV,
  BUTTON_ENTER,
  BUTTON_COUNT
} ButtonId_t;

void Button_EXTI_Callback(uint16_t GPIO_Pin);
void Button_Update(void);
uint8_t Button_WasPressed(ButtonId_t id);
uint8_t Button_WasLongPressed(ButtonId_t id, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_H */
