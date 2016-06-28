TESTS := ary_init.c ary_push.c
SOURCES := ../ary.c

CFLAGS += -std=c99 -pedantic -Wall -Wextra -g -DDEBUG -fstrict-aliasing
LDFLAGS +=
LDLIBS +=
CC := gcc

# includes
CFLAGS += -I..
CFLAGS += -I../../tap.h

# defines
CFLAGS += -D_ISOC99_SOURCE
CFLAGS += -D_POSIX_C_SOURCE=200809L

include ../../tap.h/mkfile
