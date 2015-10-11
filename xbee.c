/* Randy Westlund
 * 11/20/12
 */

//#include <avr/io.h> /* port definitions */
#include <stdint.h> /* integer type definitions */
//#include <avr/interrupt.h> /* interrupt macros */
#include <string.h> /* memcpy */
#include "xbee.h"
//#include "usart.h" /* USART functions */
/* link with usart.c */

/* Should be called once for each incoming byte */
uint8_t *xbee_add_byte(c) {
    /* start out using the first Rx buffer */
    static uint8_t *buf = rx_buffer;
    static uint8_t *buf_ptr = rx_buffer;
    static uint8_t *buf_oob = rx_buffer + XBEE_BUFFER_SIZE;

    /* holds the length of the packet we're receiving */
    static uint16_t len;

    /* oob error */
    if(buf_ptr >= buf_oob) {
        /* ERROR! */ //TODO: handle this
    }

    /* if no data in buffer, next byte should be START */
    if(buf_ptr == buf) {
        if(c != START) return NULL; /* bad data */
    }

    *buf_ptr++ = c; /* read in next byte */

    /* do we have the length field? */
    if(buf_ptr == buf + 3) {
        len = ((uint16_t)buf[1])<<8 | buf[2];
        return NULL;
    }
    /* if we've finished the packet */
    if (len && (buf_ptr - buf == len + 4)) {
        uint8_t chksum = 0;
        uint16_t i;
        /* calculate checksum */
        for(i = 3; i <= len - 1; i++) chksum += buf[i];
        /* if the packet is valid */
        if(chksum == 0xFF) {
            len = 0;
            /* swap which buffer we're using and return a pointer to the one we
             * just filled. This gives us double-buffering without the memcpy */
            if (buf == rx_buffer) {
                buf = rx_buffer2;
                buf_ptr = rx_buffer2;
                buf_oob = rx_buffer2 + XBEE_BUFFER_SIZE;
                return rx_buffer;
            }
            else {
                buf = rx_buffer;
                buf_ptr = rx_buffer;
                buf_oob = rx_buffer + XBEE_BUFFER_SIZE;
                return rx_buffer2;
            }
        }
        /* drop the bad packet */
        else {
            buf_ptr = buf;
            len = 0;
            return NULL;
        }
    }
}


/* This function send an AT command. Example arg: "AP1" set AP to 1.
 */
void xbee_tx_command(const uint8_t *data, uint16_t bytes)
{
  uint16_t len = bytes + 2;

  /* we could be in an interrupt, so if the buffer is busy, we simply fail */
  if(xbee_tx_buffer_ptr) return; //TODO: can this be more graceful?

  xbee_tx_buffer[0] = len>>8;
  xbee_tx_buffer[1] = len;
  xbee_tx_buffer[2] = 0x08; /* frame type, AT command */
  xbee_tx_buffer[3] = get_frame_id();
  memcpy((uint8_t*)xbee_tx_buffer+4, data, bytes);
  xbee_tx_frame();
}


/* This function takes a string and sends it to address, type 0x10.
 */
void xbee_tx_data(uint64_t addr, uint8_t current_frame_id, const uint8_t *data, uint16_t bytes)
{
  uint16_t len = bytes + 14;
  if(addr == 0) addr = 0xFFFF; /* broadcast */

  /* we could be in an interrupt, so if the buffer is busy, we simply fail */
  if(xbee_tx_buffer_ptr) return; //TODO: can this be more graceful?

  xbee_tx_buffer[0] = len>>8; /* len upper */
  xbee_tx_buffer[1] = len; /* len lower */
  xbee_tx_buffer[2] = 0x10; /* frame type */
  xbee_tx_buffer[3] = current_frame_id; /* frame ID */
  xbee_tx_buffer[4] = addr>>56; /* addr high */
  xbee_tx_buffer[5] = addr>>48;
  xbee_tx_buffer[6] = addr>>40;
  xbee_tx_buffer[7] = addr>>32;
  xbee_tx_buffer[8] = addr>>24;
  xbee_tx_buffer[9] = addr>>16;
  xbee_tx_buffer[10] = addr>>8;
  xbee_tx_buffer[11] = addr; /* addr low */
  xbee_tx_buffer[12] = 0xFF; /* reserved */
  xbee_tx_buffer[13] = 0xFE; /* reserved */
  xbee_tx_buffer[14] = 0x00; /* max hops */
  xbee_tx_buffer[15] = 0x00; /* Tx options */
  memcpy((uint8_t*)xbee_tx_buffer+16, data, bytes); /* insert data payoad */
  xbee_tx_frame();
}


/* This function prepends the START delimiter, appends the checksum,
 * and transmits the frame.
 */
void xbee_tx_frame(void)
{
  uint16_t len = (((uint16_t)xbee_tx_buffer[0])<<8) | xbee_tx_buffer[1]; /* packet len */
  uint16_t num = len + 3; /* total num of bytes to Tx, not counting START */
  uint8_t chksum = 0;

  uint16_t i;
  for(i = 0; i < len; i++) /* generate checksum */
  {
    chksum += xbee_tx_buffer[i+2];
  }
  xbee_tx_buffer[num-1] = 0xFF - chksum; /* store final checksum */

  xbee_tx_buffer_ptr = xbee_tx_buffer;
  xbee_tx_buffer_oob = xbee_tx_buffer + num; /* end of buffer */
  usart_putc(START); /* start the transmission */
  UCSR0B |= _BV(UDRIE0); /* ensable UDRE interrupts to finish transmission */
}


/* This function enables API mode for the given xbee.
 */
//void enable_api_mode(void) //TODO: this sends "+++~", doesn't work
//{
//  delay(1000);
//  Serial1.print("+++");
//  delay(1100);
//  Serial1.print("ATAP1\r"); /* API no-escape mode */
//  delay(100);
//  Serial1.print("ATWR\r"); /* write to mem */
//  delay(100);
//  Serial1.print("ATCN\r"); /* exit AT mode */
//}


/* Return consecutive frame IDs, skipping 0 */
uint8_t get_frame_id() {
    static uint8_t frame_id;
    /* don't use ID 0, it means no ACK */
    if(frame_id == 255) frame_id = 0;
    return ++frame_id;
}
