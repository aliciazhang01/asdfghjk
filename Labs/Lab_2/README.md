# Lab 2: IMU
Read the BMI323 IMU (accelerometer + gyroscope) over I2C, convert to real units, and build a bubble level.

**Files you edit:** `bmi323.c`, `lab_2.c`
**Given, don't edit:** `i2c.c` / `i2c.h`

## Hardware
| signal | pin | notes |
| --- | --- | --- |
| I2C1 SCL | PB6 | clock |
| I2C1 SDA | PB7 | data |

The BMI323 (U4) answers at 7-bit address **0x68** or **0x69** depending on its SDO pin. Part 1 finds which.

## Background: I2C
I2C uses two shared, open-drain wires: chips can only pull them low, and pull-up resistors bring them high. The MCU is the **controller** and starts every transfer. Each chip has its own address.

A register read is two transfers joined by a **repeated START**:

```
 S | addr+W | A | reg | A | Sr | addr+R | A | data0 | A | data1 | A | … | dataN | N | P
   └─ "I want to talk to you, I'm writing" ┘    └─ "now I'm reading" ──────────────────┘

 S = START   Sr = repeated START   P = STOP
 A = ACK (receiver pulls SDA low)   N = NACK (controller says "that's enough")
 addr+W = (addr << 1) | 0   addr+R = (addr << 1) | 1
```

If no chip has that address, nobody ACKs. A bus scan relies on this.

`i2c.c` handles the bus:
```c
int i2c_probe(uint8_t addr);                                                    // 0 if someone ACKs
int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);        // 0 on success
int i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len); // 0 on success
```

## Background: BMI323 registers
See the datasheet's register map and I2C section. This chip differs from most in two ways:
- **Registers are 16 bits**, sent **low byte first** (little endian)
- **Every I2C read starts with 2 dummy bytes** before the real data

Reading 1 register means reading 4 bytes and dropping the first 2:
```
byte:   0      1      2         3
      dummy  dummy  reg LSB   reg MSB
```

A **burst** read continues through `reg`, `reg+1`, …, 2 bytes each.

## Part 1: bus scan
In `lab_2.c`, finish `scan_bus()`: probe every address from 0x08 to 0x77 and save the ones that answer.

✅ `lab_2_found` in the Watch panel shows the IMU's address.

## Part 2: read one register
1. in `bmi323.c`, write `bmi323_read_regs()`: read `2 + 2 * count` bytes, skip the dummy bytes, and combine each pair into a `uint16_t`
2. in `lab_2.c`, set `bmi323_addr` in `read_chip_id()` to the address from part 1

✅ `lab_2_chip_id` reads `0x0043` (or at least the low byte is `0x43`).

## Part 3: configure and read
Both sensors power up **off**. `bmi323_init()` (given) soft-resets the chip, writes `ACC_CONF` and `GYR_CONF`, then checks the error register.

1. write `bmi323_write_reg()` (opposite of part 2, low byte first)
2. fill in the `CONF_*` bit fields at the top of `bmi323.c` from the `ACC_CONF` / `GYR_CONF` register tables. You want:
   | setting | accelerometer | gyroscope |
   | --- | --- | --- |
   | output data rate | 100 Hz | 100 Hz |
   | range | ±8 g | ±2000 °/s |
   | mode | normal | normal |
3. work out the sensitivities (`ACC_LSB_PER_G`, `GYR_LSB_PER_DPS`) and the temperature conversion
4. finish `bmi323_read()`: fill every field of the reading

If `bmi323_init()` fails, check its `err` variable against the datasheet.

✅ Watch `lab_2_imu`. With the board flat on the table, `acc_g` is about `(0, 0, +1)`. Flip it over, Z goes to about −1.

✅ Fill in this table (round to 0.1 g):
| board position | acc X | acc Y | acc Z |
| --- | --- | --- | --- |
| flat, chips up | | | |
| flat, chips down | | | |
| standing on its left edge | | | |
| standing on its right edge | | | |
| standing on its front edge | | | |
| standing on its back edge | | | |

Mark on the board (or a photo of it) which way +X, +Y and +Z point.

✅ Sitting still, `gyr_dps` is close to 0. Spin the board on the table, Z jumps.

✅ `temp_c` is a bit above room temperature.

## Part 4: bubble level
At rest, the only acceleration is gravity, so its direction gives the tilt.

1. in `compute_tilt()`, calculate `lab_2_roll` (rotation about X) and `lab_2_pitch` (rotation about Y) in degrees. Both should be 0 when flat.
2. in `show_level()`, turn the LED on solid when the board is within `LEVEL_DEG` of flat, otherwise blink it, faster the more it tilts. Don't use `HAL_Delay`.

<details>
<summary>Hint: tilt formulas</summary>

```
roll  = atan2(ay, az)
pitch = atan2(-ax, sqrt(ay² + az²))
```
`atan2f` returns radians, multiply by `RAD_TO_DEG`.
</details>

✅ Lay the board flat: LED solid. Tilt it slowly: it starts blinking and speeds up.

## Extensions
- **Gyro calibration:** at startup, average 100 gyro samples while the board sits still and subtract that offset from every reading afterwards.
- **Complementary filter:** track angle by integrating the gyro (`angle += rate * dt`), and every step pull it slightly toward the accelerometer's angle: `angle = 0.98 * (angle + rate * dt) + 0.02 * accel_angle`. Compare it to part 4 while shaking.
- **Tap detection:** blink the LED once when the board is tapped. Look for a short spike in the acceleration size.
- Read the datasheet's FIFO section and read a batch of samples at once instead of one at a time.

## Running the completed parts

The implementation defaults to `LAB_2_PART 4` (bubble level). The application
calls `lab_2()` in the `USER CODE BEGIN 2` section of `Core/Src/main.c`.
Change `LAB_2_PART` at the top of `lab_2.c` to 1, 2, 3, or 4, then rebuild
and flash after each change. Parts 1 and 2 finish their check and wait forever;
parts 3 and 4 run continuously. No optional extensions are enabled.

- **Part 1:** scan 0x08 through 0x77. The Watch array stores up to eight
  responding addresses without overflowing. Expect the BMI323 at 0x68 or 0x69.
- **Part 2:** scan again, then verify the chip ID at a detected BMI323 address.
  No manually guessed address is required. Watch `lab_2_chip_id` and
  `bmi323_addr`; the chip-ID low byte must be 0x43.
- **Part 3:** print acceleration in mg, angular rate in mdps, and temperature
  in millidegrees Celsius every 100 ms. Divide printed values by 1000 to get
  g, degrees/second, and degrees Celsius. The Watch structure also exposes
  the raw words and converted floating-point values.
- **Part 4:** watch `lab_2_roll` and `lab_2_pitch`. The LED stays on when both
  angles are within 3 degrees of flat; otherwise it toggles at intervals
  between 50 and 500 ms, with shorter intervals at larger tilts. The LED
  routine uses elapsed tick time and never calls `HAL_Delay`.

### Configuration and calculations

Both sensors use ODR code 0x8 (100 Hz) and mode code 0x4 (normal).
The accelerometer range code is 0x2 and gyro range code is 0x4, yielding
`ACC_CONF = 0x4028` and `GYR_CONF = 0x4048`.

- Acceleration: `32768 / 8 = 4096` counts/g; `acc_g = signed_raw / 4096`.
- Angular rate: `32768 / 2000 = 16.384` counts/(degree/second).
- Temperature: `temp_c = signed_raw / 512 + 23`.
- Roll: `atan2f(ay, az) * 180 / pi`.
- Pitch: `atan2f(-ax, sqrtf(ay*ay + az*az)) * 180 / pi`.

Reference: [Bosch BMI323 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi323-ds000.pdf),
ACC_CONF, GYR_CONF, and TEMP_DATA register descriptions.

### Hardware observations still to record

The blank measurement table above is intentionally left for actual board
readings, rounded to 0.1 g. No hardware measurements have been made by the
software tests. Use part 3, hold the board still in each of the six positions,
and record `lab_2_imu.acc_g[0..2]` (or divide the printed mg values by 1000).

Chips-up and chips-down should be approximately (0, 0, +1) and (0, 0, -1)
under the README's mounting convention. On each edge, one horizontal axis
should approach +1 or -1 g. Which edge corresponds to X or Y depends on the
sensor's mounting and the chosen definition of the board's front; determine
that from the measurements rather than guessing.

To label an axis, orient the board so its reading is about +1 g: that positive
axis points upward in that pose. Transfer the arrow to the board or its photo.
Also check near-zero stationary gyro values, a changing Z gyro reading when
spinning flat, plausible die temperature, and the flat/tilted LED behavior.
