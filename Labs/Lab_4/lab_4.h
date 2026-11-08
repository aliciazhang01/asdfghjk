#ifndef LAB_4
#define LAB_4

#include <stdint.h>

// Your board's number on the shared bus. Every board on the bus needs a
// different one, ask your lead which to use.
#define NODE_ID 0x01
// The board you're paired with in part 5
#define PARTNER_ID 0x02

// Message IDs (11-bit standard IDs)
#define LAB_4_HEARTBEAT_ID 0x123          // parts 2 and 3
#define LAB_4_LED_CMD_BASE 0x200          // + node id, received
#define LAB_4_TELEMETRY_BASE 0x300        // + node id, sent

// Bus health, filled in by check_bus(). Add these to the Watch panel.
typedef struct {
  uint32_t tx_errors;     // transmit error counter (0-255)
  uint32_t rx_errors;     // receive error counter (0-127)
  uint32_t error_passive; // 1 once either counter passes 127
  uint32_t bus_off;       // 1 once tx_errors passes 255
  uint32_t sent;          // frames queued for sending
  uint32_t received;      // frames read out of the RX FIFO
  uint32_t mismatched;    // loopback frames that came back different
} lab_4_bus_t;

extern lab_4_bus_t lab_4_bus;

// Re-initializes FDCAN1 for the given bitrate and mode (FDCAN_MODE_NORMAL,
// FDCAN_MODE_INTERNAL_LOOPBACK, ...) with room for std_filters standard ID
// filters. Call before HAL_FDCAN_Start. Returns 0 on success.
int can_setup(uint32_t bitrate, uint32_t mode, uint32_t std_filters);

// Main lab_4 loop
void lab_4(void);

#endif
