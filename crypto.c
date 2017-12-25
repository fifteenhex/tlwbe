#include <openssl/cmac.h>
#include <openssl/rand.h>
#include <string.h>
#include "crypto.h"
#include "lorawan.h"

uint32_t crypto_mic_2(void* key, size_t keylen, void* data1, size_t data1len,
		void* data2, size_t data2len) {
	CMAC_CTX *ctx = CMAC_CTX_new();
	CMAC_Init(ctx, key, keylen, EVP_aes_128_cbc(), NULL);
	CMAC_Update(ctx, data1, data1len);
	if (data2 != NULL)
		CMAC_Update(ctx, data2, data2len);
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

uint32_t crypto_mic(void* key, size_t keylen, void* data, size_t datalen) {
	return crypto_mic_2(key, keylen, data, datalen, NULL, 0);
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

#define SKEYPAD (SESSIONKEYLEN - (1 + APPNONCELEN + NETIDLEN + DEVNONCELEN))

static void crypto_calculatesessionkeys_key(const uint8_t* key,
		const uint8_t* appnonce, const uint8_t* netid, const uint8_t* devnonce,
		uint8_t* skey, uint8_t kb) {

	uint8_t pad[SKEYPAD] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx);
	EVP_EncryptInit(ctx, EVP_aes_128_ecb(), key, NULL);
	EVP_CIPHER_CTX_set_padding(ctx, 0);

	int outlen;
	EVP_EncryptUpdate(ctx, skey, &outlen, &kb, sizeof(kb));
	skey += outlen;
	EVP_EncryptUpdate(ctx, skey, &outlen, appnonce, APPNONCELEN);
	skey += outlen;
	EVP_EncryptUpdate(ctx, skey, &outlen, netid, NETIDLEN);
	skey += outlen;
	EVP_EncryptUpdate(ctx, skey, &outlen, devnonce, DEVNONCELEN);
	skey += outlen;
	EVP_EncryptUpdate(ctx, skey, &outlen, pad, sizeof(pad));
	skey += outlen;
	EVP_EncryptFinal(ctx, skey, &outlen);
	skey += outlen;
	EVP_CIPHER_CTX_free(ctx);

}

void crypto_calculatesessionkeys(const uint8_t* key, const uint8_t* appnonce,
		const uint8_t* netid, const uint8_t* devnonce, uint8_t* networkkey,
		uint8_t* appkey) {

	const uint8_t nwkbyte = 0x01;
	const uint8_t appbyte = 0x02;

	crypto_calculatesessionkeys_key(key, appnonce, netid, devnonce, networkkey,
			nwkbyte);
	crypto_calculatesessionkeys_key(key, appnonce, netid, devnonce, appkey,
			appbyte);
}

void crypto_randbytes(void* buff, size_t len) {
	RAND_bytes(buff, len);
}

void crypto_fillinblock(uint8_t* block, uint8_t firstbyte, uint8_t dir,
		uint32_t devaddr, uint32_t fcnt, uint8_t lastbyte) {
	// 5 - dir
	// 6 - devaddr
	// 10 - fcnt
	// 15 - len
	memset(block, 0, BLOCKLEN);
	block[0] = firstbyte;
	block[5] = dir;
	memcpy(block + 6, &devaddr, sizeof(devaddr));
	memcpy(block + 10, &fcnt, sizeof(fcnt));
	block[15] = lastbyte;
}
