#include <openssl/cmac.h>
#include <openssl/rand.h>
#include "crypto.h"

uint32_t crypto_mic(void* key, size_t keylen, void* data, size_t datalen) {
	CMAC_CTX *ctx = CMAC_CTX_new();
	CMAC_Init(ctx, key, keylen, EVP_aes_128_cbc(), NULL);
	CMAC_Update(ctx, data, datalen);

	uint8_t mac[16];
	size_t maclen;
	CMAC_Final(ctx, mac, &maclen);
	CMAC_CTX_free(ctx);

	uint32_t mic;
	for (int i = 3; i >= 0; i--) {
		mic = mic << 8;
		mic |= mac[i];
	}
	return mic;
}

void crypto_encryptfordevice(const char* key, void* data, size_t datalen,
		void* dataout) {
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx);
	EVP_DecryptInit(ctx, EVP_aes_128_ecb(), key, NULL);
	int outlen;
	EVP_DecryptUpdate(ctx, dataout, &outlen, data, datalen);
	EVP_DecryptFinal(ctx, dataout + outlen, &outlen);
	EVP_CIPHER_CTX_free(ctx);
}

void crypto_randbytes(void* buff, size_t len) {
	RAND_bytes(buff, len);
}
