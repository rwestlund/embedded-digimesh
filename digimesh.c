/* Copyright (c) 2015 Randy Westlund, All rights resrved.
 * This code is under the BSD-2-Clause license
 */

#include <stdint.h> /* integer type definitions */
#include <string.h> /* memcpy */
#include "digimesh.h"

/* Should be called once for each incoming byte. Will return an xbee_packet
 * pointer when a packet has been completed  */
struct xbee_packet *xbee_add_byte(uint8_t c) {
    /* Two buffers for receiving packets. We receive into one, then swap
     * pointers and receive into the other. This lets the application handle a
     * packet while we read the next, without using memcpy. Obviously the
     * application must read it quickly */
    static struct xbee_packet p1, p2;
    /* start out using the first Rx buffer */
    static struct xbee_packet *p = &p1;
    /* holds the payload length of the packet we're receiving */
    static uint16_t len;

    /* oob error -- drop the packet before we overflow */
    if(p->len >= XBEE_BUFFER_SIZE) {
        p->len = 0; len = 0;
        /* log the err */
        xbee_comm_err_count++;
    }

    /* if no data in buffer, next byte must be START */
    if(!p->len && c != XBEE_START) { 
        /* log the err */
        xbee_comm_err_count++;
        return NULL;
    }

    /* read in next byte */
    p->buf[p->len++] = c;

    /* if we just got the payload length */
    if(p->len == 3) {
        len = ((uint16_t)p->buf[1])<<8 | p->buf[2];
        return NULL;
    }
    /* if we've finished the packet */
    if (len && (p->len == len + 4)) {
        /* if the packet is valid */
        if(!calc_checksum(p)) {
            len = 0;
            /* swap which buffer we're using and return a pointer to the one we
             * just filled. This gives us double-buffering without the memcpy */
            if (p == &p1) { p = &p2; return &p1; }
            else { p = &p1; return &p2; }
        }
        /* drop the bad packet */
        else {
            p->len = 0; len = 0;
            /* log the err */
            xbee_comm_err_count++;
        }
    }
    return NULL;
}


/* Build packet for an AT command. Pass a pointer to an empty xbee_packet, a
 * pointer to the buffer to use for the payload, and the length of the payload.
 * Example arg: "NITEST" set NI to "TEST" */
void xbee_build_command_packet(struct xbee_packet *p, const uint8_t *data, uint16_t bytes)
{
  p->len = bytes + 6;
  p->buf[0] = XBEE_START;
  p->buf[1] = (uint8_t)((p->len-4)>>8);
  p->buf[2] = (uint8_t)(p->len-4);
  p->buf[3] = FRAME_AT_COMMAND;
  p->buf[4] = get_frame_id();
  memcpy(p->buf+5, data, bytes);
  p->buf[p->len-1] = calc_checksum(p);
}


/* This function takes a buffer and sends it to an address.
 * An empty addr means broadcast.
 */
struct xbee_packet *xbee_tx_data(struct xbee_packet *p, uint64_t addr,
const uint8_t *data, uint16_t bytes)
{
  p->len = bytes + 14;
  if(!addr) addr = XBEE_BROADCAST_ADDRESS;
  p->buf[0] = XBEE_START;
  p->buf[1] = (uint8_t)((p->len-4)>>8); /* len upper */
  p->buf[2] = (uint8_t)((p->len-4)); /* len lower */
  p->buf[3] = FRAME_TRANSMIT_REQUEST;
  p->buf[4] = get_frame_id();
  p->buf[5] = addr>>56; /* addr high */
  p->buf[6] = addr>>48;
  p->buf[7] = addr>>40;
  p->buf[8] = addr>>32;
  p->buf[9] = addr>>24;
  p->buf[10] = addr>>16;
  p->buf[11] = addr>>8;
  p->buf[12] = addr; /* addr low */
  p->buf[13] = 0xff; /* reserved */
  p->buf[14] = 0xfe; /* reserved */
  p->buf[15] = 0x00; /* max hops */
  p->buf[16] = 0x00; /* Tx options */
  memcpy((uint8_t*)p->buf+17, data, bytes); /* insert data payoad */
  p->buf[p->len-1] = calc_checksum(p);
  return p;
}

/* Calculate and return checksum from a message buffer */
uint8_t calc_checksum(const struct xbee_packet *p) {
    uint16_t i;
    uint8_t chksum = 0;
    for(i = 3; i < p->len-4; i++) chksum += p->buf[i];
    return 0xff - chksum;
}

/* Return consecutive frame IDs, skipping zero */
uint8_t get_frame_id() {
    static uint8_t frame_id;
    /* don't use ID 0, it means no ACK */
    if(frame_id == 255) frame_id = 0;
    return ++frame_id;
}
