CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -O2 -D_GNU_SOURCE
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

BEACON_SRCS = src/main.c src/hci.c src/beacon.c
BEACON_OBJS = $(BEACON_SRCS:.c=.o)

KEYGEN_SRCS = src/keygen.c src/beacon.c
KEYGEN_OBJS = $(KEYGEN_SRCS:.c=.o)

TARGETS = findmy-beacon findmy-keygen

.PHONY: all clean install uninstall

all: $(TARGETS)

findmy-beacon: $(BEACON_OBJS)
	$(CC) $(CFLAGS) -o $@ $(BEACON_OBJS)

findmy-keygen: $(KEYGEN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(KEYGEN_OBJS) -lcrypto

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGETS)

install: $(TARGETS)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 findmy-beacon $(DESTDIR)$(BINDIR)/findmy-beacon
	install -m 755 findmy-keygen $(DESTDIR)$(BINDIR)/findmy-keygen

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/findmy-beacon $(DESTDIR)$(BINDIR)/findmy-keygen
