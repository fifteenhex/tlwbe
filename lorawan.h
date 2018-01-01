#pragma once

#include <stdint.h>

#define ASCIILEN(n) ((n * 2) + 1)

#define EUILEN		8
#define EUIASCIILEN ((EUILEN * 2) + 1)
#define KEYLEN		16
#define KEYASCIILEN	((KEYLEN * 2) + 1)

#define MHDRLEN		1
#define MICLEN		4
#define APPNONCELEN	3
#define APPNONCEASCIILEN ASCIILEN(APPNONCELEN)
#define DEVNONCELEN 2
#define DEVNONCEASCIILEN	((DEVNONCELEN * 2) + 1)
#define DEVADDRLEN	4
#define DEVADDRASCIILEN ((DEVADDRLEN * 2) + 1)
#define NETIDLEN	3
#define SESSIONKEYLEN	16
#define BLOCKLEN 16

#define MHDR_MTYPE_SHIFT	5
#define MHDR_MTYPE_MASK		0b111
#define MHDR_MTYPE_JOINREQ	0b000
#define MHDR_MTYPE_JOINACK	0b001
#define MHDR_MTYPE_UNCNFUP	0b010
#define MHDR_MTYPE_UNCNFDN	0b011
#define MHDR_MTYPE_CNFUP	0b100
#define MHDR_MTYPE_CNFDN	0b101

#define LORAWAN_TYPE(t) ((t >> MHDR_MTYPE_SHIFT) & MHDR_MTYPE_MASK)

#define LORAWAN_PAYLOADWITHMIC(payloadsz) (payloadsz + MICLEN)
#define LORAWAN_PKTSZ(payloadsz) (MHDRLEN + LORAWAN_PAYLOADWITHMIC(payloadsz))

struct lorawan_joinreq {
	uint64_t appeui;
	uint64_t deveui;
	uint16_t devnonce;
}__attribute__((packed));

struct lorawan_joinaccept {
	uint8_t appnonce[APPNONCELEN];
	uint8_t netid[NETIDLEN];
	uint32_t devaddr;
	uint8_t dlsettings;
	uint8_t rxdelay;
	uint8_t cflist[16];
}__attribute__((packed));

#define LORAWAN_FHDR_FCTRL_FOPTLEN_MASK 0b1111

struct lorawan_fhdr {
	uint32_t devaddr;
	uint8_t fctrl;
	uint16_t fcnt;
}__attribute__((packed));
