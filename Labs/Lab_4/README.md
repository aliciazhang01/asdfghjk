# Lab 4: CAN
CAN connects every controller on the car. Set the bus speed, test in loopback, join a real bus, then stream IMU and ADC readings while taking LED commands from another node.

**Files you edit:** `lab_4.c`, `lab_4.h` (your node ID)
**Given, don't edit:** `can_lib.c` / `can_lib.h`
**Needs:** labs 2 and 3 finished (part 4 uses your drivers)

## Hardware
| signal | pin | notes |
| --- | --- | --- |
| FDCAN1 RX | PB8 | from the transceiver |
| FDCAN1 TX | PB9 | to the transceiver |

The FDCAN peripheral outputs logic levels. A **transceiver** converts them to the differential CANH / CANL pair on the connector. Find both in the schematic, and check whether the board has a 120 Ω termination resistor.

For parts 3–5 you need one of these:
- a second education board (a partner), or
- a USB-CAN adapter set to **classic CAN, 500 kbit/s**, plus its software (PCAN-View, SavvyCAN, or `candump` / `cansend` from can-utils on Linux)

Wire CANH to CANH, CANL to CANL, and connect the grounds. A CAN bus needs **120 Ω at each end** (about 60 Ω measured between CANH and CANL with the power off).

## Background: CAN
- **Differential, shared bus.** Every node sees every frame. A bit is **dominant** (0, CANH pulled up / CANL pulled down) or **recessive** (1, both lines left alone). If anyone sends dominant, the bus is dominant.
- **No addresses, only IDs.** A frame has an 11-bit **ID** saying *what* the data is (e.g. "front wheel speeds"), not who it's for. Each node decides which IDs it cares about.
- **Arbitration.** If two nodes start at once, each sends its ID bit by bit and listens. A node sending recessive that hears dominant has lost, and it stops and tries again later. **Lower ID = higher priority.**
- **ACK.** Every node that receives a frame correctly pulls the ACK bit dominant. If nobody does, the sender counts it as an error.
- **Error counters.** Each node has a transmit (TEC) and receive (REC) error counter. Errors add to them (8 for a transmit error), successful frames take 1 off. Over 127: **error passive** (the node gets quieter about errors). TEC over 255: **bus-off** (it stops transmitting completely).

A classic CAN data frame, simplified:
```
| SOF | ID (11) | RTR | IDE | r0 | DLC (4) | data (0–8 bytes) | CRC (15) | ACK | EOF |
```

### Bit timing
Each bit is cut into **time quanta** (tq). The node resyncs on the sync segment and samples the bus at the end of segment 1:
```
|sync|<───────── TSEG1 ─────────>|<─ TSEG2 ─>|
  1 tq          13 tq            ▲    2 tq
                            sample point
```
```
tq       = prescaler / f_clock
bit time = (1 + TSEG1 + TSEG2) × tq
bitrate  = f_clock / (prescaler × (1 + TSEG1 + TSEG2))
```
FDCAN1 is clocked from the 8 MHz crystal.

## Part 1: bit timing
CubeMX left FDCAN1 at prescaler 16, TSEG1 13, TSEG2 2.

1. work out the bitrate CubeMX set, by hand
2. in `can_setup()`, compute the prescaler for `bitrate`. Return −1 if no whole number works.

✅ Watch `hfdcan1.Init.NominalPrescaler`: it's the value you expect for 500 kbit/s.

## Part 2: talk to yourself
In **internal loopback** mode the FDCAN peripheral receives its own frames without using the pins, so no wiring is needed.

In `heartbeat()`, build an 8-byte frame (counter in byte 0, `HAL_GetTick()` in bytes 4–7, little endian) and send it with `HAL_FDCAN_AddMessageToTxFifoQ`. The receive side (given) reads the frame back and compares it.

✅ Watch `lab_4_bus`: `sent` and `received` go up together, `mismatched` stays 0, and both error counters stay 0.

## Part 3: a real bus
Wire your board to your partner's or to the USB-CAN adapter.

**First, alone.** Disconnect the CAN cable (or turn the adapter off) and run part 3.

✅ `lab_4_bus.tx_errors` climbs and stops at 128. `error_passive` = 1, `bus_off` stays 0.

**Then connected.** Plug the bus in and set the adapter to 500 kbit/s. (With a partner board, both run part 3.)

✅ The adapter shows ID `0x123` ten times a second, byte 0 counting up.

✅ `tx_errors` goes back down to 0, one per frame.

✅ With a partner, `lab_4_bus.received` goes up: you're getting their heartbeats. With an adapter, send a frame from the PC and see `received` go up.

## Part 4: telemetry and commands
Set `NODE_ID` in `lab_4.h` (get a unique one from your lead).

| ID | direction | bytes | contents |
| --- | --- | --- | --- |
| `0x300 + NODE_ID` | sent, every 100 ms | 0–1 | force sensor, mV (int16) |
| | | 2–3, 4–5, 6–7 | accel X, Y, Z, mg (int16) |
| `0x200 + NODE_ID` | received | 0 | LED mode: 0 off, 1 on, 2 blink |
| | | 1–2 | blink period, ms (uint16) |

All multi-byte values are **little endian**.

`telemetry()` uses `can_lib`'s message table: each TX message has a function that packs its bytes, and each RX message has one that unpacks them. The library sets up hardware **filters** from the RX table, so other frames never reach your code.

1. `construct_telemetry()`: pack the table above with `put_le16`
2. `parse_led_command()`: unpack the LED command

✅ The adapter shows `0x300 + NODE_ID` at 10 Hz. Decoded by hand, the values follow board tilt and sensor presses.

✅ Send `0x200 + NODE_ID` with data `02 F4 01`. The LED blinks with a 500 ms period. Try `00` and `01`.

✅ Send `0x200 + (some other node ID)`. `lab_4_bus.received` doesn't change: the filter dropped it.

✅ Write a [DBC file](https://www.csselectronics.com/pages/can-dbc-file-database-intro) for your two messages and load it into the adapter software so it decodes them for you. A starting point:
```
BO_ 769 Telemetry_1: 8 Node1
 SG_ Force_mV : 0|16@1- (1,0) [-2400|2400] "mV" Vector__XXX
 SG_ AccX_mg : 16|16@1- (1,0) [-8000|8000] "mg" Vector__XXX
```
(`769` = `0x301`, `0|16@1-` = start bit 0, 16 bits, little endian, signed.)

## Part 5: react to your partner
Pair with another board. Set `PARTNER_ID` to their `NODE_ID` (and they set theirs to yours).

1. in `telemetry()`, register an RX message for `0x300 + PARTNER_ID` with `parse_partner_telemetry` as its handler
2. in `parse_partner_telemetry()`, save their Z acceleration

✅ Flip your partner's board upside down: **your** LED turns on.

## Extensions
- **Interrupt-driven receive:** enable the FDCAN1 interrupt in CubeMX, call `HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0)`, and handle frames in `HAL_FDCAN_RxFifo0Callback` instead of polling.
- **Two rates:** add a gyro message sent every 10 ms (`FAST`) and move telemetry to `SLOW` (every 100 ms) using `can_lib`'s `speed` field.
- **Heartbeat timeout:** if your partner's telemetry hasn't arrived for 500 ms, blink the LED fast as a warning.
