
DESTDIR ?=
PREFIX = /usr/local

CC ?= gcc
CFLAGS += -std=gnu99 -Wall -Wextra -Wshadow -Werror
LDFLAGS += -Wl,--no-undefined

all: ttymidi ttymidi.so

ttymidi: src/ttymidi.c
	$(CC) $< $(CFLAGS) $(shell pkg-config --cflags --libs jack) $(LDFLAGS) -lpthread -o $@

ttymidi.so: src/ttymidi.c
	$(CC) $< $(CFLAGS) $(shell pkg-config --cflags --libs jack) $(LDFLAGS) -fPIC -lpthread -shared -o $@

install: ttymidi
	install -m 755 ttymidi $(DESTDIR)$(PREFIX)/bin

clean:
	rm -f ttymidi

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/ttymidi
