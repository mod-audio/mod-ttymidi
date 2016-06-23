
DESTDIR ?=
PREFIX = /usr/local

CC ?= gcc
CFLAGS += -std=gnu99 -Wall -Wextra -Wshadow -Werror -fvisibility=hidden
LDFLAGS += -Wl,--no-undefined

all: ttymidi ttymidi.so

ttymidi: src/ttymidi.c src/mod-semaphore.h
	$(CC) $< $(CFLAGS) $(shell pkg-config --cflags --libs jack) $(LDFLAGS) -lpthread -o $@

ttymidi.so: src/ttymidi.c src/mod-semaphore.h
	$(CC) $< $(CFLAGS) $(shell pkg-config --cflags --libs jack) $(LDFLAGS) -fPIC -lpthread -shared -o $@

install: ttymidi ttymidi.so
	install -m 755 ttymidi    $(DESTDIR)$(PREFIX)/bin/
	install -m 755 ttymidi.so $(DESTDIR)$(shell pkg-config --variable=libdir jack)/jack/

clean:
	rm -f ttymidi ttymidi.so

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/ttymidi
	rm $(DESTDIR)$(shell pkg-config --variable=libdir jack)/jack/ttymidi.so
