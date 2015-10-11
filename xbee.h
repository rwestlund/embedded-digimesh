/* Randy Westlund
 * XBee include file
 * 11/19/12
 */

#ifndef XBEE_H
#define XBEE_H

#define START 0x7E
#define XBEE_BUFFER_SIZE 100

/* COMMUNICATION FUNCTIONS */
void xbee_tx_command(const uint8_t *data, uint16_t bytes);
void xbee_tx_data(uint64_t addr, uint8_t current_frame_id, const uint8_t *data, uint16_t bytes);
void xbee_tx_frame(void);
//void xbee_enable_api_mode(void);
uint8_t get_frame_id(void);
uint8_t *xbee_add_byte(uint8_t c);

/* Two buffers for receiving packets. We receive into one, then swap pointers
 * and receive into the other. This lets the application handle a packet while
 * we read the next, without using memcpy */
static uint8_t rx_buffer[XBEE_BUFFER_SIZE];
static uint8_t rx_buffer2[XBEE_BUFFER_SIZE];

volatile uint8_t xbee_tx_buffer[XBEE_BUFFER_SIZE];
volatile uint8_t *xbee_tx_buffer_ptr;
volatile uint8_t *xbee_tx_buffer_oob;

#endif /* XBEE_H */
