#include <openssl/cmac.h>

#include "crypto.h"

uint32_t crypto_mic(void* key, size_t keylen, void* data, size_t datalen) {
	CMAC_CTX *ctx = CMAC_CTX_new();
	CMAC_Init(ctx, key, keylen, EVP_aes_128_cbc(), NULL);
	CMAC_Update(ctx, data, datalen);
	uint8_t mac[16];
	size_t maclen;
	CMAC_Final(ctx, mac, &maclen);
	CMAC_CTX_free(ctx);

	printf("mac - size %d; ", (int) maclen);
	for (int i = maclen; i >= 0; i--) {
		printf("%02x", (int) mac[i]);
	}
	printf("\n");

	uint32_t mic;
	for (int i = 3; i >= 0; i--) {
		mic = mic << 8;
		mic |= mac[i];
	}
	return mic;
}
