#ifndef LAB_3
#define LAB_3

#include "mcp3561.h"
#include <stdint.h>

// Noise measurement for one oversampling ratio (part 4)
typedef struct {
  mcp3561_osr_t osr;
  float mean_mv;
  float std_uv;     // standard deviation in microvolts
  float sample_ms;  // time per conversion
} lab_3_noise_t;

// Things to add to the Watch panel
extern uint8_t lab_3_status;            // STATUS byte from the reset command
extern mcp3561_reading_t lab_3_adc;     // latest reading
extern uint32_t lab_3_samples_per_sec;  // measured conversion rate
extern lab_3_noise_t lab_3_noise[3];    // part 4 results
extern float lab_3_baseline_mv;         // FSR reading with nothing pressing
extern uint32_t lab_3_press_count;      // presses detected

// Main lab_3 loop
void lab_3(void);

#endif
