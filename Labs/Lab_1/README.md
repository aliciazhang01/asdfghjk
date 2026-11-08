# Lab 1: LED
Blink the status LED four ways: blocking delay, non-blocking loop, timer interrupt, and state machine.

**Files you edit:** `blinky.c`, `morse.c`, `lab_1.c`

## Hardware
| signal | pin | notes |
| --- | --- | --- |
| Status LED | PB13 | push-pull output, high = on |

CubeMX names PB13 `Status_LED` in `Core/Inc/main.h`:
```c
HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_SET);   // on
HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_RESET); // off
HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);                // flip
```

The CPU and every timer run at **8 MHz** from the external crystal (HSE). `HAL_GetTick()` returns milliseconds since reset, counted by the SysTick interrupt.

## Part 1: blocking blink
`blink_blocking()` in `blinky.c` is already written. Read it, then run part 1.

✅ The LED blinks once a second (500 ms on, 500 ms off).

✅ Add `lab_1_loop_count` to the Watch panel. Pause, wait 10 s, pause again. It goes up about once a second.

## Part 2: non-blocking blink
Check the time every loop and toggle only when it's due:

```
every loop:
    if (now - last_toggle >= period):
        toggle the LED
        last_toggle = now
```

Write `blink_nonblocking()` in `blinky.c`.

✅ Same blink as part 1.

✅ `lab_1_loop_count` now goes up by a huge amount every second.

## Part 3: timer interrupt
A hardware timer (TIM1) does the timing. When its counter rolls over, it raises an interrupt and the CPU runs `HAL_TIM_PeriodElapsedCallback`.

```
TIM1 clock (8 MHz) ──► ÷ (Prescaler + 1) ──► counter ──► counts 0 … Period, then rolls over ──► interrupt
```

1. in `blink_timer_start()`, pick `Prescaler` and `Period` so the interrupt fires `toggle_hz` times a second. Both are 16-bit (max 65535).
2. in `HAL_TIM_PeriodElapsedCallback`, toggle the LED

`lab_1.c` calls `blink_timer_start(2)`, two toggles a second = 1 Hz blink.

✅ 1 Hz blink. The main loop doesn't touch the LED at all.

✅ Change it to `blink_timer_start(10)`. The LED blinks 5 times a second.

✅ Put a breakpoint in the callback and show the **Call Stack**.

## Part 4: Morse code with a state machine
Blink "NFR" in Morse code (`-. ..-. .-.`). The timing rules, in units of `MORSE_UNIT_MS`:

| | units |
| --- | --- |
| dot | 1 on |
| dash | 3 on |
| gap inside a letter | 1 off |
| gap between letters | 3 off |
| gap before repeating | 7 off |

A **finite state machine** (FSM) keeps this manageable: the LED is always in one **state** (`On` or `Off`), each state has an enter action, and a **transition** says when to move to the next state.

```
              time up
     ┌──────────────────────┐
     ▼                      │
 ┌────────┐  time up  ┌──────────┐
 │   On   │ ────────► │   Off    │
 │ LED on │           │ LED off  │
 │ 1 or 3 │           │ 1, 3 or 7│
 └────────┘           └──────────┘
```

`morse.c` uses the [c-fsm](../../c-fsm/README.md) library. `morse_start()` creates the FSM and its states, and `morse_update()` calls `fsm_run()` to check transitions and run one step. Shared state data lives in `morse_context_t`.

Fill in the TODOs in `morse.c`:
1. `on_enter_on`: start timing, pick dot or dash length, LED on
2. `on_enter_off`: move to the next symbol, pick the gap length, LED off
3. `state_time_up`: return true once the state has lasted long enough
4. `morse_start`: add the two transitions

✅ The LED blinks `-. ..-. .-.` and repeats.

✅ Change the message to your initials.

## Extensions
- Add a third state, `Pause`, that the FSM enters after playing the message 3 times and leaves after 5 s.
- Write a function that turns plain text (`"NFR"`) into the dot-dash string.
- Dim the LED with PWM. Check in CubeMX which timer channel PB13 can connect to, if any.
