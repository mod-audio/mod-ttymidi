
DESTDIR ?=
PREFIX = /usr/local

CC ?= gcc
CFLAGS += -std=gnu99 -Wall -Wextra -Wshadow -Werror

all: ttymidi

ttymidi: src/ttymidi.c
	$(CC) $< $(CFLAGS) $(shell pkg-config --cflags --libs jack) -lpthread -o $@

install: ttymidi
	install -m 755 ttymidi $(DESTDIR)$(PREFIX)/bin

clean:
	rm -f ttymidi

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/ttymidi
