
/**
 * Canonical implementation of Init/Update/Finalize for SHA-3 byte input.
 * Based on code from https://github.com/brainhub/SHA3IUF/
 *
 * This work is released into the public domain with CC0 1.0.
 *
 * Copyright (c) 2015. Andrey Jivsov <crypto@brainhub.org>
 * Copyright (c) 2023 Tino Reichardt <milky-7zip@mcmilk.de>
 */

#ifndef SHA3_H
#define SHA3_H

#include <stdint.h>

#define SHA3_256_DIGEST_LENGTH 32
#define SHA3_384_DIGEST_LENGTH 48
#define SHA3_512_DIGEST_LENGTH 64

/* 'Words' here refers to uint64_t */
#define SHA3_KECCAK_SPONGE_WORDS (200 / sizeof(uint64_t))
typedef struct sha3_context_ {
	/* the portion of the input message that we didn't consume yet */
	uint64_t saved;
	uint64_t byteIndex;	/* 0..7--the next byte after the set one */
	uint64_t wordIndex;	/* 0..24--the next word to integrate input */
	uint64_t capacityWords;	/* the double size of the hash output in words */

	/* Keccak's state */
	union {
		uint64_t s[SHA3_KECCAK_SPONGE_WORDS];
		uint8_t sb[SHA3_KECCAK_SPONGE_WORDS * 8];
	} u;

	unsigned digest_length;
} SHA3_CTX;

void SHA3_Init(SHA3_CTX * ctx, unsigned bitSize);
void SHA3_Update(SHA3_CTX * ctx, void const *bufIn, size_t len);
void SHA3_Final(void *, SHA3_CTX * ctx);

#endif
