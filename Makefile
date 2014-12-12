P = libmutarr.a
SOURCES = mutarr.c

CFLAGS = -std=c99 -pedantic -Wall -Wextra -g -DDEBUG
LDFLAGS =
LDLIBS =
CC = gcc

OBJECTS = $(SOURCES:.c=.o)
DOCFILE = $(addsuffix .html, $(basename $(P)))
DOCS = $(SOURCES:.c=.html)

# defines
CFLAGS += -D_ISOC99_SOURCE
CFLAGS += -D_POSIX_C_SOURCE=200809L

# build
all: $(P)

release: CFLAGS := -O2 -DNDEBUG $(filter-out -g -DDEBUG, $(CFLAGS))
release: clean all strip

fresh: clean all

$(P): $(OBJECTS)
ifneq (, $(filter .so, $(suffix $(P))))
	$(CC) -shared -fPIC $(LDFLAGS) -o $@ $^ $(LDLIBS)
else ifneq (, $(filter .dll, $(suffix $(P))))
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS) -Wl,--out-implib,$(addsuffix .a, $(P))
else ifneq (, $(filter .a, $(suffix $(P))))
	$(AR) rcs $@ $^
else
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endif

$(OBJECTS): %.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

strip:
	strip --strip-debug --strip-unneeded $(P)

clean:
	@$(RM) $(P) $(OBJECTS)

test:
	$(CC) $(LDFLAGS) -o test/test test/test.c $(SOURCES) $(LDLIBS)
	test/test

.PHONY: all release fresh strip clean test
