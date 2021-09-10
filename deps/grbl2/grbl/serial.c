/*
  serial.c - Low level functions for sending and recieving bytes via the serial port
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"
#include <drivers/uart.h>
#include <logging/log.h>
#include <sys/ring_buffer.h>

LOG_MODULE_REGISTER (serial);

const struct device *uart = NULL;

uint8_t rxRingBufferBlock[RX_BUFFER_SIZE]; // Power of 2 is more efficient here (according to the docs)
struct ring_buf rxRingBuf;

uint8_t txRingBufferBlock[TX_BUFFER_SIZE];
struct ring_buf txRingBuf;

// Returns the number of bytes available in the RX serial buffer.
uint32_t serial_get_rx_buffer_available () { return ring_buf_capacity_get (&rxRingBuf) - ring_buf_size_get (&rxRingBuf); }

// Returns the number of bytes used in the RX serial buffer.
// NOTE: Deprecated. Not used unless classic status reports are enabled in config.h.
uint32_t serial_get_rx_buffer_count () { return ring_buf_size_get (&rxRingBuf); }

// Returns the number of bytes used in the TX serial buffer.
// NOTE: Not used except for debugging and ensuring no TX bottlenecks.
uint32_t serial_get_tx_buffer_count () { return ring_buf_size_get (&txRingBuf); }

static void uartInterruptHandler (const struct device *dev, void *user_data);

void serial_init()
{
   // Before error checking (which returns from this function and would leave ring buffers uninitialized).
        ring_buf_init (&rxRingBuf, sizeof (rxRingBufferBlock), rxRingBufferBlock);
        ring_buf_init (&txRingBuf, sizeof (txRingBufferBlock), txRingBufferBlock);

        // This usart has to be initialized and set-up in src/mcu-peripherals.cc
        uart = device_get_binding (DT_LABEL (DT_ALIAS (grbluart)));

        if (!uart) {
                LOG_ERR ("Problem configuring uart");
                return;
        }

        // IRQ based API.
        uart_irq_callback_set (uart, uartInterruptHandler);

        /* Enable rx interrupts */
        uart_irq_rx_enable (uart);
}


// Writes one byte to the TX serial buffer. Called by main program.
void serial_write(uint8_t data) {

  while (ring_buf_put (&txRingBuf, &data, 1) != 1) {
    // TODO: Restructure st_prep_buffer() calls to be executed here during a long print.
    if (sys_rt_exec_state & EXEC_RESET) { return; } // Only check for abort to avoid an endless loop.
  }

  uart_irq_tx_enable (uart);
}

/*
// Data Register Empty Interrupt handler
ISR(SERIAL_UDRE)
{
  uint8_t tail = serial_tx_buffer_tail; // Temporary serial_tx_buffer_tail (to optimize for volatile)

  // Send a byte from the buffer
  UDR0 = serial_tx_buffer[tail];

  // Update tail position
  tail++;
  if (tail == TX_RING_BUFFER) { tail = 0; }

  serial_tx_buffer_tail = tail;

  // Turn off Data Register Empty Interrupt to stop tx-streaming if this concludes the transfer
  if (tail == serial_tx_buffer_head) { UCSR0B &= ~(1 << UDRIE0); }
}
*/

// Fetches the first byte in the serial read buffer. Called by main program.
uint8_t serial_read()
{
        uint8_t data;

        if (ring_buf_get (&rxRingBuf, &data, 1) != 1) {
    return SERIAL_NO_DATA;
        }

    return data;
}


static void uartInterruptHandler (const struct device *dev, void *user_data)
{
        ARG_UNUSED (user_data);

        while (uart_irq_update (dev) && uart_irq_is_pending (dev)) {

                if (uart_irq_rx_ready (dev)) {
                        int recvLen = 0;
                        uint8_t data = 0;

                        while ((recvLen = uart_fifo_read (dev, &data, 1)) == 1) {
  // Pick off realtime command characters directly from the serial stream. These characters are
  // not passed into the main buffer, but these set system state flag bits for realtime execution.
  switch (data) {
    case CMD_RESET:         mc_reset(); break; // Call motion control reset routine.
    case CMD_STATUS_REPORT: system_set_exec_state_flag(EXEC_STATUS_REPORT); break; // Set as true
    case CMD_CYCLE_START:   system_set_exec_state_flag(EXEC_CYCLE_START); break; // Set as true
    case CMD_FEED_HOLD:     system_set_exec_state_flag(EXEC_FEED_HOLD); break; // Set as true
    default :
      if (data > 0x7F) { // Real-time control characters are extended ACSII only.
        switch(data) {
          case CMD_SAFETY_DOOR:   system_set_exec_state_flag(EXEC_SAFETY_DOOR); break; // Set as true
          case CMD_JOG_CANCEL:   
            if (sys.state & STATE_JOG) { // Block all other states from invoking motion cancel.
              system_set_exec_state_flag(EXEC_MOTION_CANCEL); 
            }
            break; 
          #ifdef DEBUG
          case CMD_DEBUG_REPORT: {
            unsigned int l = irq_lock ();
            bit_true (sys_rt_exec_debug, EXEC_DEBUG_REPORT);
            irq_unlock (l);
          } break;
          #endif
          case CMD_FEED_OVR_RESET: system_set_exec_motion_override_flag(EXEC_FEED_OVR_RESET); break;
          case CMD_FEED_OVR_COARSE_PLUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_COARSE_PLUS); break;
          case CMD_FEED_OVR_COARSE_MINUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_COARSE_MINUS); break;
          case CMD_FEED_OVR_FINE_PLUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_FINE_PLUS); break;
          case CMD_FEED_OVR_FINE_MINUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_FINE_MINUS); break;
          case CMD_RAPID_OVR_RESET: system_set_exec_motion_override_flag(EXEC_RAPID_OVR_RESET); break;
          case CMD_RAPID_OVR_MEDIUM: system_set_exec_motion_override_flag(EXEC_RAPID_OVR_MEDIUM); break;
          case CMD_RAPID_OVR_LOW: system_set_exec_motion_override_flag(EXEC_RAPID_OVR_LOW); break;
          case CMD_SPINDLE_OVR_RESET: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_RESET); break;
          case CMD_SPINDLE_OVR_COARSE_PLUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_COARSE_PLUS); break;
          case CMD_SPINDLE_OVR_COARSE_MINUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_COARSE_MINUS); break;
          case CMD_SPINDLE_OVR_FINE_PLUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_FINE_PLUS); break;
          case CMD_SPINDLE_OVR_FINE_MINUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_FINE_MINUS); break;
          case CMD_SPINDLE_OVR_STOP: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_STOP); break;
          case CMD_COOLANT_FLOOD_OVR_TOGGLE: system_set_exec_accessory_override_flag(EXEC_COOLANT_FLOOD_OVR_TOGGLE); break;
          #ifdef ENABLE_M7
            case CMD_COOLANT_MIST_OVR_TOGGLE: system_set_exec_accessory_override_flag(EXEC_COOLANT_MIST_OVR_TOGGLE); break;
          #endif
        }
        // Throw away any unfound extended-ASCII character by not passing it to the serial buffer.
                                        }
                                        else { // Write character to buffer
                                                int written = ring_buf_put (&rxRingBuf, &data, 1);

                                                if (written != 1) {
                                                        LOG_ERR ("GRBL UART RX byte dropped.");
        }
      }
  }
                        } // while
}

                if (uart_irq_tx_ready (dev)) {
                        uint8_t buffer[64];

                        uint32_t bytesReadLen = ring_buf_get (&txRingBuf, buffer, sizeof (buffer));

                        if (bytesReadLen == 0) {
                                uart_irq_tx_disable (dev);
                                continue;
}

                        int sendLen = uart_fifo_fill (dev, buffer, bytesReadLen);

                        if (sendLen < bytesReadLen) {
                                LOG_ERR ("Drop %d bytes", bytesReadLen - sendLen);
                        }
                }
        }
}

void serial_reset_read_buffer () { ring_buf_reset (&rxRingBuf); }
