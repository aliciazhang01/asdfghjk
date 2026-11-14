
#include "blinky.h"
#include "main.h"
#include <stdint.h>

extern TIM_HandleTypeDef htim1;

// TIM1 runs off APB2, which is 8 MHz on this board (see SystemClock_Config)
#define TIM1_CLOCK_HZ 8000000

void blink_blocking() {
  HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_SET);

  HAL_Delay(BLINK_DURATION_DEFAULT);

  HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_RESET);

  HAL_Delay(BLINK_DURATION_DEFAULT);
}

void blink_nonblocking() {
  // `static` keeps the value between calls
  static uint32_t last_toggle = 0;

  uint32_t now = HAL_GetTick();

  // Unsigned subtraction still gives the right answer when HAL_GetTick()
  // wraps around past 0xFFFFFFFF (after ~49 days), so compare the
  // difference, never "now >= last_toggle + period".
  if (now - last_toggle >= BLINK_DURATION_DEFAULT) {
    HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);
    last_toggle = now;
  }
}

void blink_timer_start(uint32_t toggle_hz) {
  // Counter clock = TIM1_CLOCK_HZ / (Prescaler + 1)
  // Update rate   = counter clock / (Period + 1)
  // Both registers are 16 bits, so each must be 65535 or less.
  //
  // CubeMX's values (Prescaler 999, Period 7999) give
  //   8 MHz / 1000 = 8 kHz counter, 8 kHz / 8000 = 1 update per second,
  // i.e. 1 toggle/s = a 0.5 Hz blink.
  //
  // Instead: Prescaler 7999 -> 8 MHz / 8000 = 1 kHz counter (1 count = 1 ms).
  // Then Period + 1 = number of ms between toggles = 1000 / toggle_hz.
  //   toggle_hz = 2  -> Period 499 (500 ms)
  //   toggle_hz = 10 -> Period 99  (100 ms)
  if (toggle_hz == 0) {
    toggle_hz = 1; // avoid dividing by zero
  } else if (toggle_hz > 1000) {
    toggle_hz = 1000; // a 1 kHz counter can't go faster than 1 ms per toggle
  }

  HAL_TIM_Base_Stop_IT(&htim1); // in case it was already running

  htim1.Init.Prescaler = (TIM1_CLOCK_HZ / 1000) - 1; // 7999
  htim1.Init.Period = (1000 / toggle_hz) - 1;

  HAL_TIM_Base_Init(&htim1);
  HAL_TIM_Base_Start_IT(&htim1);
}

// Called from the TIM1 update interrupt every time the counter rolls over
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM1) {
    HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);
  }
}
