#include "lab_3.h"
#include "main.h"
#include "mcp3561.h"
#include <math.h>
#include <stdio.h>

// Which part of the lab runs. Change this, rebuild, and flash.
#define LAB_3_PART 1

#define NOISE_SAMPLES 64
#define READY_TIMEOUT_MS 100

// Part 5: how far from the baseline counts as pressed / released. Pick these
// from what you measured in part 3. Press must be bigger than release so a
// reading near the edge doesn't flicker (hysteresis).
#define PRESS_DELTA_MV 200.0f
#define RELEASE_DELTA_MV 100.0f

uint8_t lab_3_status;
mcp3561_reading_t lab_3_adc;
uint32_t lab_3_samples_per_sec;
lab_3_noise_t lab_3_noise[3] = {
    {.osr = MCP3561_OSR_32},
    {.osr = MCP3561_OSR_256},
    {.osr = MCP3561_OSR_4096},
};
float lab_3_baseline_mv;
uint32_t lab_3_press_count;

static void halt(const char *message) {
  printf("ADC: %s\r\n", message);
  while (1) {
  }
}

// Waits for the IRQ pin to say a new result is ready, then reads it
static int read_next(mcp3561_reading_t *reading) {
  uint32_t start = HAL_GetTick();

  while (!mcp3561_data_ready()) {
    if (HAL_GetTick() - start > READY_TIMEOUT_MS) {
      return -1;
    }
  }
  return mcp3561_read(reading);
}

// ---------------------------------------------------------------- part 1

static void say_hello(void) {
  int ok = mcp3561_transfer(
      MCP3561_COMMAND(MCP3561_FAST_RESET, MCP3561_FAST_COMMAND), NULL, NULL, 0,
      &lab_3_status);

  printf("ADC: STATUS 0x%02x, %s\r\n", lab_3_status,
         ok == 0 ? "address matches" : "no answer");
}

// ---------------------------------------------------------------- part 3

static void stream(void) {
  uint32_t window_start = HAL_GetTick();
  uint32_t count = 0;

  while (1) {
    if (read_next(&lab_3_adc) != 0) {
      halt("no new conversion, is the IRQ pin set up?");
    }
    count++;

    // Once a second, save how many conversions came in and print one
    if (HAL_GetTick() - window_start >= 1000) {
      lab_3_samples_per_sec = count;
      count = 0;
      window_start = HAL_GetTick();
      printf("ADC: %ld uV (code %ld), %lu samples/s\r\n",
             (long)(lab_3_adc.millivolts * 1000), (long)lab_3_adc.code,
             (unsigned long)lab_3_samples_per_sec);
    }
  }
}

// ---------------------------------------------------------------- part 4

static void measure_noise(lab_3_noise_t *result) {
  float sum = 0, sum_sq = 0;
  uint32_t start;

  if (mcp3561_init(result->osr) != 0) {
    halt("init failed");
  }
  read_next(&lab_3_adc); // throw away the first result after a config change

  start = HAL_GetTick();
  for (int i = 0; i < NOISE_SAMPLES; i++) {
    if (read_next(&lab_3_adc) != 0) {
      halt("no new conversion");
    }
    // TODO (part 4): add this sample to sum and its square to sum_sq
  }

  // TODO (part 4): fill in result->mean_mv and result->std_uv (in
  // microvolts!). std = sqrt(E[x^2] - E[x]^2), where E[] is the average.
  (void)sum;
  (void)sum_sq;
  result->sample_ms = (float)(HAL_GetTick() - start) / NOISE_SAMPLES;
}

// ---------------------------------------------------------------- part 5

static void force_button(void) {
  float sum = 0;
  int pressed = 0;

  // Average a short burst with nothing touching the sensor
  for (int i = 0; i < 32; i++) {
    read_next(&lab_3_adc);
    sum += lab_3_adc.millivolts;
  }
  lab_3_baseline_mv = sum / 32;

  while (1) {
    if (read_next(&lab_3_adc) != 0) {
      continue;
    }
    float delta = fabsf(lab_3_adc.millivolts - lab_3_baseline_mv);

    // TODO (part 5): if not pressed and delta goes over PRESS_DELTA_MV, it's a
    // new press (count it). If pressed and delta drops under
    // RELEASE_DELTA_MV, it's released.
    (void)delta;

    HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin,
                      pressed ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
}

// ---------------------------------------------------------------- main

void lab_3(void) {
  printf("lab 3 part %d\r\n", LAB_3_PART);

  switch (LAB_3_PART) {
  case 1: // is anyone there?
    say_hello();
    while (1) {
    }

  case 2: // configure it
    if (mcp3561_init(MCP3561_OSR_256) != 0) {
      halt("init failed");
    }
    printf("ADC: configured\r\n");
    while (1) {
    }

  case 3: // read it
    if (mcp3561_init(MCP3561_OSR_256) != 0) {
      halt("init failed");
    }
    stream();

  case 4: // noise vs. speed
    for (int i = 0; i < 3; i++) {
      measure_noise(&lab_3_noise[i]);
      printf("ADC: OSR setting %d: mean %ld uV, std %ld uV, %ld us/sample\r\n",
             lab_3_noise[i].osr, (long)(lab_3_noise[i].mean_mv * 1000),
             (long)lab_3_noise[i].std_uv,
             (long)(lab_3_noise[i].sample_ms * 1000));
    }
    while (1) {
    }

  case 5: // force sensor as a button
    if (mcp3561_init(MCP3561_OSR_256) != 0) {
      halt("init failed");
    }
    force_button();
  }
}
