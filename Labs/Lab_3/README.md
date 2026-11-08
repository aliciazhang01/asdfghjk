# Lab 3: ADC
Configure the MCP3561R 24-bit ADC over SPI, read the force sensor, measure noise vs. speed, and turn the sensor into a button.

**Files you edit:** `mcp3561.c`, `lab_3.c`

## Hardware
| signal | pin | notes |
| --- | --- | --- |
| SPI1 SCK | PA5 | 1 MHz (8 MHz ÷ 8) |
| SPI1 MISO | PA6 | ADC → MCU (the chip's SDO) |
| SPI1 MOSI | PA7 | MCU → ADC (the chip's SDI) |
| SPI1 NSS | PA4 | chip select, driven by the SPI peripheral |
| ADC_IRQ | PB4 | input, low when a new result is ready |

The ADC is U3. CH0 is the `FSR_SENSE` net (force-sensitive resistor circuit, see the schematic).

## Background: SPI
SPI has a clock, a data line in each direction, and a chip select (CS) per device. The controller pulls CS low, then each clock pulse **sends one bit and receives one bit**. To read a byte you have to send one (usually zeros).

```
CS    ‾‾\________________________________________/‾‾
SCK      _/‾\_/‾\_/‾\_ … _/‾\_/‾\_ … _/‾\_/‾\_
MOSI     [ command byte  ][ data out (or zeros) … ]
MISO     [ STATUS byte   ][ data in              … ]
```

`HAL_SPI_TransmitReceive(&hspi1, tx, rx, n, timeout)` sends `n` bytes from `tx` and fills `rx` with `n` bytes. The SPI peripheral holds CS (PA4) low for the whole call.

## Background: MCP3561R
See the "Serial Interface" and "Internal Registers" sections of the datasheet.

Every transfer starts with a **command byte**:
```
 bit:   7   6 │ 5   4   3   2 │ 1   0
      device  │ register or   │ command
      address │ fast command  │ type
```
The macro `MCP3561_COMMAND(reg, type)` in `mcp3561.h` builds it. While the command byte goes out, the chip sends back its **STATUS byte**, and bits 5:3 echo the device address, which confirms the chip is there.

A **delta-sigma** ADC averages many fast, rough samples into one result. The number averaged is the **oversampling ratio (OSR)**: higher OSR means less noise and fewer results per second.

Results are **24-bit two's complement**: `0x7FFFFF` is just under +VREF, `0x800000` is −VREF. VREF is the chip's internal 2.4 V reference.

## Part 1: talk to the chip
Write `mcp3561_transfer()` in `mcp3561.c`, which the rest of the driver uses. Part 1 sends the "full reset" fast command and saves the STATUS byte.

✅ `lab_3_status` in the Watch panel: bits 5:3 are `010` (device address 01, plus a copy of A0 inverted).

## Part 2: configure it
`mcp3561_init()` (given) resets the chip, checks its ID, writes `CONFIG0` through `MUX` in one burst, and reads them back to check they stuck. You fill in the 6 config bytes from the datasheet's register descriptions:

| register | settings |
| --- | --- |
| CONFIG0 | internal voltage reference, not in shutdown, internal clock (no clock output), no current source on the inputs, ADC mode = conversion |
| CONFIG1 | prescaler = MCLK ÷ 1, oversampling ratio = the `osr` argument |
| CONFIG2 | given: boost 1×, gain 1×, auto-zero off |
| CONFIG3 | continuous conversions, 24-bit output format, CRC off, digital offset/gain calibration off |
| IRQ | IRQ pin outputs the IRQ (not MDAT), inactive state = logic high (push-pull), fast commands enabled, conversion-start interrupt off |
| MUX | VIN+ = CH0, VIN− = AGND |

Write each byte as binary first (`0b…`), then convert.

✅ Part 2 prints `ADC: configured` (or step through `mcp3561_init()` and see it return 0).

## Part 3: read it
1. `mcp3561_data_ready()`: read the ADC_IRQ pin. It's **active low**.
2. `mcp3561_read()`: build the 24-bit result into an `int32_t`, **sign-extend** it (if bit 23 is set, the number is negative), then convert to millivolts

✅ Watch `lab_3_adc`. Press the force sensor, `millivolts` changes. Let go, it goes back.

✅ Write down `millivolts` with nothing pressing and with a firm press. You'll need these in part 5.

✅ `lab_3_samples_per_sec` shows the conversion rate.

## Part 4: noise vs. speed
Part 4 reads 64 samples at three OSR settings. Fill in the statistics in `measure_noise()`: the mean, and the standard deviation in **µV**:

```
mean = Σx / N
std  = √( Σx²/N − mean² )
```

Don't touch the sensor while it runs.

✅ Fill in from `lab_3_noise`:
| OSR | std (µV) | ms per sample |
| --- | --- | --- |
| 32 | | |
| 256 | | |
| 4096 | | |

## Part 5: force sensor as a button
`force_button()` averages a few startup readings as the **baseline** (don't touch the sensor at startup), then tracks how far each reading moves from it. Write the press / release logic with **hysteresis**: a press needs `delta > PRESS_DELTA_MV`, but a release needs `delta < RELEASE_DELTA_MV`, which is smaller.

Pick `PRESS_DELTA_MV` and `RELEASE_DELTA_MV` from your part 3 numbers.

✅ LED is on while you press. `lab_3_press_count` goes up by exactly 1 per press, even for slow or light presses.

## Extensions
- **Interrupt instead of polling:** in CubeMX, change PB4 to `GPIO_EXTI4` (falling edge) and enable its interrupt. Read the ADC in `HAL_GPIO_EXTI_Falling_Callback` and keep a running average there.
- **Force to resistance:** using the schematic, turn the voltage into the sensor's resistance. Then use the FSR's datasheet curve to estimate force in newtons.
- **Scan channels:** change `MUX` to read the other inputs and the chip's internal temperature sensor (see the MUX table).
- **Offset calibration:** measure the ADC's own offset with `MUX` set to read AGND against AGND, and subtract it from every reading.
