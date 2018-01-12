PKGCONFIG ?= pkg-config
CFLAGS = --std=gnu99 -ggdb
GLIB = `$(PKGCONFIG) --cflags --libs glib-2.0 gio-2.0`
JSON = `$(PKGCONFIG) --cflags --libs json-glib-1.0`
LIBCRYPTO = `$(PKGCONFIG) --cflags --libs libcrypto` 
SQLITE = `$(PKGCONFIG) --cflags --libs sqlite3`
MOSQUITTO = -lmosquitto

all: tlwbe

tlwbe: tlwbe.c tlwbe.h \
	crypto.c crypto.h \
	join.c join.h \
	database.c database.h database_context.h \
	control.c control.h \
	utils.c utils.h \
	downlink.c downlink.h \
	uplink.c uplink.h \
	pktfwdbr.c pktfwdbr.h \
	packet.c packet.h \
	regional.c regional.h \
	flags.c flags.h \
	config.h lorawan.h
	$(CC) $(GLIB) $(JSON) $(LIBCRYPTO) $(SQLITE) $(MOSQUITTO) $(CFLAGS) $(filter %.c,$^) -o $@

.PHONY: clean

clean:
	rm -rf tlwbe
