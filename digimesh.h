/* Copyright (c) 2015 Randy Westlund, All rights resrved.
 * This code is under the BSD-2-Clause license
 */

#ifndef DIGIMESH_H
#define DIGIMESH_H

#include <stdint.h>

#define XBEE_BUFFER_SIZE 100

/* digimesh constants */
#define XBEE_START 0x7e
#define XBEE_BROADCAST_ADDRESS 0x000000000000ffff

/* frame types */
#define XBEE_FRAME_TRANSMIT_REQUEST 0x10
#define XBEE_FRAME_TRANSMIT_STATUS 0x8b
#define XBEE_FRAME_AT_COMMAND 0x08
#define XBEE_FRAME_AT_COMMAND_RESPONSE 0x88
#define XBEE_FRAME_RECEIVE_PACKET 0x90
#define XBEE_FRAME_MODEM_STATUS 0x8a

/* used by the application to hold a packet ready for Tx */
struct xbee_packet {
    uint8_t buf[XBEE_BUFFER_SIZE];
    /* length of the whole buffer, not just the payload */
    uint16_t len;
    /* if set, frame_id will be set to 0, disabling ACKs */
    uint8_t disable_ack;
};

/* receive functions */
struct xbee_packet *xbee_add_byte(uint8_t c);

/* transmit functions */
uint8_t xbee_build_command_packet(struct xbee_packet *p, const uint8_t *data,
                                uint16_t bytes);
uint8_t xbee_build_data_packet(struct xbee_packet *p, uint64_t addr,
                                const uint8_t *data, uint16_t bytes);

/* utility functions */
uint8_t xbee_calc_checksum(const struct xbee_packet *p);

/* Incremented for every checksum or overflow error, saturates at 255. May be
 * used by the application in what ever way it wants */
extern volatile uint8_t xbee_comm_err_count;

#endif /* DIGIMESH_H */
