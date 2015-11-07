P := libary.a
SOURCES := ary.c

CFLAGS += -std=c99 -pedantic -Wall -Wextra -g -DDEBUG -fstrict-aliasing
LDFLAGS +=
LDLIBS +=
CC := gcc

# defines
CFLAGS += -D_ISOC99_SOURCE
CFLAGS += -D_POSIX_C_SOURCE=200809L

#include ../mkfile
$(P): $(SOURCES:.c=.o)
	$(AR) rcs $@ $^
