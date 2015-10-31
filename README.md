# embedded-digimesh

A minimal C library that implements a portion of the XBee Digimesh protocol.

## Description

This library provides convenience functions for building and receiving packets.
As it is designed for bare metal embedded applications, it does not do anything
fancy.  The application can extend it with whatever subset of the DigiMesh
protocol is needed.  I use it for my AVR/Arduino projects.

It can certainly be used on the desktop, but you may want something more
featureful for that application.

**Note:** The XBees must be in API mode (AP=1).

## License

This library is under the BSD-2-Clause license.

## Installation

Include `digimesh.h`, build `digimesh.c`, and link with `digimesh.o`.

## Testing

This library uses CUnit for tests.  After installing the CUnit libraries, run:
```
make
./obj/test
```
The makefile is written in BSD make, but if you don't have it it's easy to
build by hand:
```
cc -Wall -Werror -g -I. -I/usr/local/include -std=c99 -fstack-protector \
digimesh.c test.c -o obj/test -L/usr/local/lib -lcunit
```

## Usage

You'll have to RTFS and to get everything, but here are some examples.

### AT commands

There is one general `xbee_build_command_packet` function for building AT
commands.  To set the Node Identifier to `001`:

```
uint8_t cmd[] = "NI001";
struct xbee_packet p;
uint8_t frame_id = xbee_build_command_packet(&p, cmd, 5);
/* now send p.buf through your UART and wait for a response to frame_id */
```

### Sending a Message

```
uint8_t *my_payload;
/* fill the buffer with your app-specific payload, then: */
uint64_t addr = TARGET_ADDRESS;
struct xbee_packet p;
uint8_t frame_id = xbee_build_data_packet(&p, addr, my_payload, payload_len);
/* now send p.buf through your UART and wait for a response to frame_id */
```

### Receiving Messages

In your UART interrupt handler where you get one byte at a time, feed the byte
to the library.  It will return `NULL` if the packet is not finished, or an
`xbee_packet` pointer if it is.

```
/* as a global variable */
volatile struct xbee_packet *rx_packet;

/* then, in your interrupt handler: */
uint8_t c; /* filled with the byte you just received */
rx_packet = xbee_add_byte(c);

/* then in your main loop: */
if (rx_packet) {
  /* parse it however you like */
}
```

This library uses a double-buffered approach to receiving packets.  Bytes are
received into the first buffer until the packet is full.  At that point, a
pointer to the first buffer is returned and future bytes go into the second
buffer.  When the second buffer has a packet, they swap again.

The reason for this is to give the application time to parse the packet without
dynamically allocating memory.  Naturally, the application must parse the
packet quickly, or `memcpy` it somewhere else.

## Future Work

- Do some type parsing of received packets for the application.
- Use the `frame_id` to track ACKs and NACKs.
