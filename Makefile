PKGCONFIG ?= pkg-config
CFLAGS = --std=gnu99 -ggdb
GLIB = `$(PKGCONFIG) --cflags --libs glib-2.0 gio-2.0`
JSON = `$(PKGCONFIG) --cflags --libs json-glib-1.0`
LIBCRYPTO = `$(PKGCONFIG) --cflags --libs libcrypto` 
SQLITE = `$(PKGCONFIG) --cflags --libs sqlite3`
MOSQUITTO = -lmosquitto

all: tlwbe

tlwbe: tlwbe.c \
	crypto.c \
	join.c \
	database.c \
	control.c \
	utils.c \
	downlink.c \
	uplink.c \
	pktfwdbr.c \
	packet.c
	$(CC) $(GLIB) $(JSON) $(LIBCRYPTO) $(SQLITE) $(MOSQUITTO) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -rf tlwbe
