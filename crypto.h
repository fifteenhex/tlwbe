#pragma once

#include <stdbool.h>

uint32_t crypto_mic_2(void* key, size_t keylen, void* data1, size_t data1len,
		void* data2, size_t data2len);
uint32_t crypto_mic(void* key, size_t keylen, void* data, size_t datalen);
void crypto_encryptfordevice(const char* key, void* data, size_t datalen,
		void* dataout);
void crypto_randbytes(void* buff, size_t len);
void crypto_calculatesessionkeys(const uint8_t* key, const uint8_t* appnonce,
		const uint8_t* netid, const uint8_t* devnonce, uint8_t* networkkey,
		uint8_t* appkey);
