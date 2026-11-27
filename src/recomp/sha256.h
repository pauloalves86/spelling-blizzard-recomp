/* SHA-256 (FIPS 180-4). */
#ifndef SB_SHA256_H
#define SB_SHA256_H

#include <stddef.h>
#include <stdint.h>

void sha256(const void *data, size_t len, uint8_t out[32]);
/* Lowercase hex digest. */
void sha256_hex(const void *data, size_t len, char out[65]);

#endif
