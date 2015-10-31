# Copyright (c) 2015 Randy Westlund, All rights resrved.
# This code is under the BSD-2-Clause license

# Used to build the test code, written in BSD make

CFLAGS = -Wall -Werror -g -I. -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lcunit
MAKEOBJDIR = obj

PROG = test
SRCS = digimesh.c test.c
MAN =

.include <bsd.prog.mk>
