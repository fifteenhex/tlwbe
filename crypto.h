#pragma once

uint32_t crypto_mic(void* key, size_t keylen, void* data, size_t datalen);

void crypto_randbytes(void* buff, size_t len);
