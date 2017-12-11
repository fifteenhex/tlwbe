#pragma once

#include <stdint.h>

#define MHDR_MTYPE_SHIFT	5
#define MHDR_MTYPE_MASK		0b111
#define MHDR_MTYPE_JOINREQ	0b000
#define MHDR_MTYPE_JOINACK	0b001
#define MHDR_MTYPE_UNCNFUP	0b010
#define MHDR_MTYPE_UNCNFDN	0b011
#define MHDR_MTYPE_CNFUP	0b100
#define MHDR_MTYPE_CNFDN	0b101

struct lorawan_joinreq {
	uint64_t appeui;
	uint64_t deveui;
	uint16_t devnonce;
}__attribute__((packed));
