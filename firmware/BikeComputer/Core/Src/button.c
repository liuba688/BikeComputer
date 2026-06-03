#include "button.h"
#include "main.h"

#define BUTTON_RELEASE_STABLE_MS  100U

typedef enum
{
  BUTTON_RELEASED = 0,
  BUTTON_PRESSED_LOCKED
} ButtonState_t;

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  volatile uint8_t pressed_event;
  volatile uint8_t long_pressed_event;
  volatile ButtonState_t state;
  uint32_t press_start_tick;
  uint32_t release_high_start_tick;
  uint8_t release_high_timing;
  uint8_t long_event_sent;
} ButtonRuntime_t;

static ButtonRuntime_t buttons[BUTTON_COUNT] = {
  {BTN_NEXT_GPIO_Port, BTN_NEXT_Pin, 0U, 0U, BUTTON_RELEASED, 0U, 0U, 0U, 0U},
  {BTN_PREV_GPIO_Port, BTN_PREV_Pin, 0U, 0U, BUTTON_RELEASED, 0U, 0U, 0U, 0U},
  {BTN_ENTER_GPIO_Port, BTN_ENTER_Pin, 0U, 0U, BUTTON_RELEASED, 0U, 0U, 0U, 0U}
};

static ButtonRuntime_t *Button_FindByPin(uint16_t GPIO_Pin)
{
  for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
  {
    if (buttons[i].pin == GPIO_Pin)
    {
      return &buttons[i];
    }
  }

  return 0;
}

void Button_EXTI_Callback(uint16_t GPIO_Pin)
{
  ButtonRuntime_t *button = Button_FindByPin(GPIO_Pin);

  if (button == 0)
  {
    return;
  }

  if (button->state != BUTTON_RELEASED)
  {
    return;
  }

  if (HAL_GPIO_ReadPin(button->port, button->pin) != GPIO_PIN_RESET)
  {
    return;
  }

  button->pressed_event = 1U;
  button->state = BUTTON_PRESSED_LOCKED;
  button->press_start_tick = HAL_GetTick();
  button->release_high_timing = 0U;
  button->long_event_sent = 0U;
}

void Button_Update(void)
{
  uint32_t now;

  for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
  {
    if (buttons[i].state != BUTTON_PRESSED_LOCKED)
    {
      continue;
    }

    now = HAL_GetTick();

    if (HAL_GPIO_ReadPin(buttons[i].port, buttons[i].pin) != GPIO_PIN_SET)
    {
      buttons[i].release_high_timing = 0U;
      continue;
    }

    if (buttons[i].release_high_timing == 0U)
    {
      buttons[i].release_high_start_tick = now;
      buttons[i].release_high_timing = 1U;
      continue;
    }

    if ((now - buttons[i].release_high_start_tick) >= BUTTON_RELEASE_STABLE_MS)
    {
      buttons[i].state = BUTTON_RELEASED;
      buttons[i].release_high_timing = 0U;
    }
  }
}

uint8_t Button_WasPressed(ButtonId_t id)
{
  uint8_t was_pressed = 0U;

  if (id >= BUTTON_COUNT)
  {
    return 0U;
  }

  __disable_irq();
  if (buttons[id].pressed_event != 0U)
  {
    buttons[id].pressed_event = 0U;
    was_pressed = 1U;
  }
  __enable_irq();

  return was_pressed;
}

uint8_t Button_WasLongPressed(ButtonId_t id, uint32_t duration_ms)
{
  uint8_t was_long_pressed = 0U;

  if (id >= BUTTON_COUNT)
  {
    return 0U;
  }

  if ((buttons[id].state == BUTTON_PRESSED_LOCKED) &&
      (HAL_GPIO_ReadPin(buttons[id].port, buttons[id].pin) == GPIO_PIN_RESET) &&
      (buttons[id].long_event_sent == 0U) &&
      ((HAL_GetTick() - buttons[id].press_start_tick) >= duration_ms))
  {
    __disable_irq();
    buttons[id].long_pressed_event = 1U;
    buttons[id].pressed_event = 0U;
    buttons[id].long_event_sent = 1U;
    __enable_irq();
  }

  __disable_irq();
  if (buttons[id].long_pressed_event != 0U)
  {
    buttons[id].long_pressed_event = 0U;
    buttons[id].pressed_event = 0U;
    was_long_pressed = 1U;
  }
  __enable_irq();

  return was_long_pressed;
}
