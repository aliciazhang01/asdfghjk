
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

  // TODO (part 2): HAL_GetTick() returns milliseconds since reset. If at least
  // BLINK_DURATION_DEFAULT ms have passed since last_toggle, toggle the LED
  // and remember when you did. Otherwise do nothing and return.
  (void)last_toggle;
}

void blink_timer_start(uint32_t toggle_hz) {
  // Counter clock = TIM1_CLOCK_HZ / (Prescaler + 1)
  // Update rate   = counter clock / (Period + 1)
  // Both registers are 16 bits, so each must be 65535 or less.
  //
  // TODO (part 3): pick values so the update rate equals toggle_hz. The
  // values below are what CubeMX set up, what rate do they give?
  // Hint: a 1 kHz counter clock makes Period easy to pick in milliseconds.
  htim1.Init.Prescaler = 999;
  htim1.Init.Period = 7999;

  HAL_TIM_Base_Init(&htim1);
  HAL_TIM_Base_Start_IT(&htim1);
}

// Called from the TIM1 update interrupt every time the counter rolls over
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM1) {
    // TODO (part 3): toggle the LED
  }
}
