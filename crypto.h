#pragma once

#include <stdbool.h>

uint32_t crypto_mic(void* key, size_t keylen, void* data, size_t datalen);
void crypto_encryptfordevice(const char* key, void* data, size_t datalen,
		void* dataout);
void crypto_randbytes(void* buff, size_t len);
