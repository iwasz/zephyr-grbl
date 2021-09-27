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

K_MUTEX_DEFINE(rxUartMutex);
RING_BUF_ITEM_DECLARE_SIZE(rxRingBuf, RX_BUFFER_SIZE);

// uint8_t rxRingBufferBlock[RX_BUFFER_SIZE]; // Power of 2 is more efficient here (according to the docs)
// struct ring_buf rxRingBuf; // This ringBuffer is used both for the serial port and the sd card

K_MUTEX_DEFINE(lineUartMutex);
RING_BUF_ITEM_DECLARE_SIZE(lineRingBuf, TX_BUFFER_SIZE_WORDS);
// uint32_t lineRingBufferBlock[TX_BUFFER_SIZE_WORDS];
// struct ring_buf lineRingBuf; // Ring buffer for line based responses from the GRBL used for interactions with the SD casrd subsystem.


// uint8_t txRingBufferBlock[TX_BUFFER_SIZE_BYTES];
// struct ring_buf txRingBuf; // Character based buffer for GRBL responses. This will be sent out via UART.
RING_BUF_ITEM_DECLARE_SIZE(txRingBuf, TX_BUFFER_SIZE_BYTES);

// Returns the number of bytes available in the RX serial buffer.
uint32_t serial_get_rx_buffer_available () { return ring_buf_capacity_get (&rxRingBuf) - ring_buf_size_get (&rxRingBuf); }

// Returns the number of bytes used in the RX serial buffer.
// NOTE: Deprecated. Not used unless classic status reports are enabled in config.h.
uint32_t serial_get_rx_buffer_count () { return ring_buf_size_get (&rxRingBuf); }

// Returns the number of bytes used in the TX serial buffer.
// NOTE: Not used except for debugging and ensuring no TX bottlenecks.
// uint32_t serial_get_tx_buffer_count () { return ring_buf_size_get (&txRingBuf); }

static void uartInterruptHandler (const struct device *dev, void *user_data);

void serial_init()
{
   // Before error checking (which returns from this function and would leave ring buffers uninitialized).
        // ring_buf_init (&rxRingBuf, sizeof (rxRingBufferBlock), rxRingBufferBlock);
        // ring_buf_init (&lineRingBuf, sizeof (lineRingBufferBlock) / 4, lineRingBufferBlock);
        // ring_buf_init (&txRingBuf, sizeof (txRingBufferBlock), txRingBufferBlock);

        // This usart has to be initialized and set-up in src/mcu-peripherals.cc
        uart = device_get_binding (DT_LABEL (DT_ALIAS (grbluart)));
        
        // SD card functions check the uart variable to see if serial.c subsys. is ready. 
        // So in this line they would see it is (if uart != NULL).

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

  // First add a byte to the UART buffer. This gets sent to the PC. 
  while (ring_buf_put (&txRingBuf, &data, 1) != 1) {
    // TODO: Restructure st_prep_buffer() calls to be executed here during a long print.
    if (sys_rt_exec_state & EXEC_RESET) { return; } // Only check for abort to avoid an endless loop.
  }


  // And now another buffer that stores the same information (GRBL responses) but line by line. 
  // This is to make a cooperation with SD thread easy.
  // Double buffered.
  static uint8_t l2buffer[LINE_BUFFER_SIZE] = {'\0'};
  static uint8_t len = 0; 
  static bool cr = false; // \r
  bool lf = false;


  if (data == '\r') {
    cr = true;
  }
  else if (cr && data == '\n') {
    lf = true;
    cr = false;
  }

  l2buffer[len++] = data;

  // Adds only line by line.
  if (lf) { 
    k_mutex_lock(&lineUartMutex, K_FOREVER);

    // This data structure preserves the line length (and number of lines).
    if (ring_buf_item_put (&lineRingBuf, 0, len, (uint32_t *)l2buffer, len/4 + len%4) != 0) {
      LOG_ERR ("lineRingBuf bytes skipped");
    }

    k_mutex_unlock(&lineUartMutex);
    // uart_irq_tx_enable (uart);
    len = 0;
    l2buffer[0] = '\0';
    cr = false;
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

  k_mutex_lock(&rxUartMutex, K_FOREVER); // Isn't it too frequently?
  uint32_t bytesRead = ring_buf_get (&rxRingBuf, &data, 1);
  k_mutex_unlock(&rxUartMutex);

  if (bytesRead != 1){
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
                        uint8_t buffer[LINE_BUFFER_SIZE];

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

/****************************************************************************/
/* Access to the RX and TX buffers from the outside (for the SD card        */
/* implementation).                                                         */
/****************************************************************************/

void serial_reset_read_buffer () { ring_buf_reset (&rxRingBuf); }
void serial_reset_transmit_buffer () { ring_buf_reset (&lineRingBuf); }


bool serial_is_initialized () { return uart != NULL; }

/**
 * Disables both RX and TX IRQs.
 */
void serial_disable_irqs ()
{
  if (uart == NULL) {
    return;
  }

  uart_irq_rx_disable(uart);
  // uart_irq_tx_disable(uart);
}

/**
 * Enables both RX and TX IRQs.
 */
void serial_enable_irqs ()
{
  if (uart == NULL) {
    return;
  }

  uart_irq_rx_enable(uart);
  // uart_irq_tx_enable(uart);
}

static uint32_t serial_buffer_appendln_impl (const char *str, bool crlf)
{  
  k_mutex_lock(&rxUartMutex, K_FOREVER);
  uint32_t written = ring_buf_put (&rxRingBuf, (const uint8_t *)str, strlen (str));
  
  if (crlf){
    ring_buf_put (&rxRingBuf, (const uint8_t *)"\r\n", 2);
  }
  
  k_mutex_unlock(&rxUartMutex);
  return written;
}

/**
 * Appends a string (a line[s]) to the RX buffer as if they has been sent 
 * through UART. Do not call from the ISR, call from the user code. Can be called
 * only if UART IRQs are off!
 */
uint32_t serial_buffer_append (const char *str) {  return serial_buffer_appendln_impl (str, false); }

/**
* As the serial_buffer_append, but adds \r\n at the end.
*/
uint32_t serial_buffer_appendln (const char *str) { return serial_buffer_appendln_impl (str, true); }

/**
 * Returns line size in bytes (can be 0). Or -1 if the buffer 
 * is too small (bufSize).
 */
int serial_get_tx_line (uint32_t *buffer, uint8_t bufSize)
{
  uint16_t type = 0;
  uint8_t lineLen = 0;

  k_mutex_lock(&lineUartMutex, K_FOREVER);
  int ret = ring_buf_item_get(&lineRingBuf, &type, &lineLen, buffer, &bufSize);
  k_mutex_unlock(&lineUartMutex);

  if (ret == -EAGAIN) { // Ring buffer empty
    return 0;
  }

  if (ret < 0) {
    return ret;
  }

  return lineLen;
}