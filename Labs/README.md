# Labs
Four labs, one per piece of hardware on the board. Do them in order.

| lab | hardware | interface | topics |
| --- | --- | --- | --- |
| [Lab 1](Lab_1/README.md) | status LED (PB13) | GPIO, TIM1 | GPIO, blocking vs. non-blocking code, timer interrupts, state machines |
| [Lab 2](Lab_2/README.md) | BMI323 IMU (U4) | I2C1 | I2C transactions, reading a datasheet register map, sensor units, tilt from gravity |
| [Lab 3](Lab_3/README.md) | MCP3561R ADC (U3) + force sensor | SPI1 | SPI, configuring a chip from its datasheet, two's complement, noise vs. speed |
| [Lab 4](Lab_4/README.md) | CAN transceiver | FDCAN1 | CAN frames, bit timing, error handling, filters, sending sensor data on a bus |

Plan on about 2 hours per lab.

## Before you start
- finish the setup in the [top-level README](../README.md) (clone with submodules, build, flash)
- have the datasheets open, links are at the bottom of the top-level README
- for lab 4 parts 3 to 5 you also need a second board or a USB-CAN adapter, ask your lead (the simulator bench provides both)

## How a lab works
Each lab folder has:
- `README.md`: the handout
- `lab_N.c`: the entry point. `#define LAB_N_PART` picks which part runs
- driver files with `TODO (part X)` comments where you write code

To run a lab:
1. in `Core/Src/main.c`, uncomment the lab's call (only one at a time):
   ```c
     lab_1();
     // lab_2();
     // lab_3();
     // lab_4();
   ```
2. set `LAB_N_PART` in `lab_N.c` to the part you're on
3. build and flash with `F5` (on the simulator: build and upload the `.elf` to the part's bench, see below)

Everything compiles before you write anything. Only edit files with a `TODO` unless the handout says otherwise.

## Seeing what your code does
The board has no screen, so check your work with the debugger.

**Watch panel.** Each lab has global variables named `lab_N_...` that hold its results.
1. start debugging with `F5`, then continue (`F5` again)
2. pause with `F6`
3. in the **Run and Debug** sidebar, open **Watch**, click **+** and type the variable name (e.g. `lab_2_imu`)
4. continue, wait, pause again to see new values

Right-click a value to change its format (e.g. hex).

**Breakpoints.** Click left of a line number. Execution stops there and you can hover over variables. The **Call Stack** panel shows how you got there.

**printf (optional).** `printf` output goes out the SWO pin through the ST-LINK (see `__io_putchar` in `Core/Src/main.c`). One way to read it:
1. flash your code, then stop debugging
2. open stm32cube programmer, connect, and open the **SWV** tab
3. set the system clock to **8 MHz** and press **Start**

`printf` can't print floats (`%f`), so the labs print integers.

## Simulator
The same `Labs/` code runs in the NFR27 CTF simulator. Its starter project (`starter.zip` on the CTF site) is this repo with `Core/` generated for an STM32H743, since the simulator can't run the U535. Code written there works on the real board.

- build the same way, then upload `build/Debug/nfr27_ctf.elf` on the bench for the part you're on
- the bench shows the LED, sensors, CAN bus, `printf` output and checks. It reads your `lab_N_...` variables, so don't rename them
- **Reference run** on the bench shows what a passing board does
- the simulator doesn't model CAN bit timing, ACKs or error counters

## Checkoff
Show your lead each checkpoint (marked ✅ in the handouts).

| | part 1 | part 2 | part 3 | part 4 | part 5 |
| --- | --- | --- | --- | --- | --- |
| Lab 1 | ☐ | ☐ | ☐ | ☐ | |
| Lab 2 | ☐ | ☐ | ☐ | ☐ | |
| Lab 3 | ☐ | ☐ | ☐ | ☐ | ☐ |
| Lab 4 | ☐ | ☐ | ☐ | ☐ | ☐ |
