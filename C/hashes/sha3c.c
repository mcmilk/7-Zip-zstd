
/**
 * Canonical implementation of Init/Update/Finalize for SHA-3 byte input.
 * Based on code from https://github.com/brainhub/SHA3IUF/
 *
 * This work is released into the public domain with CC0 1.0.
 *
 * Copyright (c) 2015. Andrey Jivsov <crypto@brainhub.org>
 * Copyright (c) 2023 Tino Reichardt <milky-7zip@mcmilk.de>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sha3c.h"

#define SHA3_ASSERT( x )

#if defined(_MSC_VER)
#define SHA3_CONST(x) x
#else
#define SHA3_CONST(x) x##L
#endif

/* 
 * This flag is used to configure "pure" Keccak, as opposed to NIST SHA3.
 */
#define SHA3_USE_KECCAK_FLAG 0x80000000
#define SHA3_CW(x) ((x) & (~SHA3_USE_KECCAK_FLAG))

#ifndef SHA3_ROTL64
#define SHA3_ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

static const uint64_t keccakf_rndc[24] = {
	SHA3_CONST(0x0000000000000001UL), SHA3_CONST(0x0000000000008082UL),
	SHA3_CONST(0x800000000000808aUL), SHA3_CONST(0x8000000080008000UL),
	SHA3_CONST(0x000000000000808bUL), SHA3_CONST(0x0000000080000001UL),
	SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008009UL),
	SHA3_CONST(0x000000000000008aUL), SHA3_CONST(0x0000000000000088UL),
	SHA3_CONST(0x0000000080008009UL), SHA3_CONST(0x000000008000000aUL),
	SHA3_CONST(0x000000008000808bUL), SHA3_CONST(0x800000000000008bUL),
	SHA3_CONST(0x8000000000008089UL), SHA3_CONST(0x8000000000008003UL),
	SHA3_CONST(0x8000000000008002UL), SHA3_CONST(0x8000000000000080UL),
	SHA3_CONST(0x000000000000800aUL), SHA3_CONST(0x800000008000000aUL),
	SHA3_CONST(0x8000000080008081UL), SHA3_CONST(0x8000000000008080UL),
	SHA3_CONST(0x0000000080000001UL), SHA3_CONST(0x8000000080008008UL)
};

static const unsigned keccakf_rotc[24] = {
	1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62,
	18, 39, 61, 20, 44
};

static const unsigned keccakf_piln[24] = {
	10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20,
	14, 22, 9, 6, 1
};

/* generally called after SHA3_KECCAK_SPONGE_WORDS-ctx->capacityWords words 
 * are XORed into the state s 
 */
static void keccakf(uint64_t s[25])
{
	int i, j, round;
	uint64_t t, bc[5];
#define KECCAK_ROUNDS 24

	for (round = 0; round < KECCAK_ROUNDS; round++) {

		/* Theta */
		for (i = 0; i < 5; i++)
			bc[i] =
			    s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];

		for (i = 0; i < 5; i++) {
			t = bc[(i + 4) % 5] ^ SHA3_ROTL64(bc[(i + 1) % 5], 1);
			for (j = 0; j < 25; j += 5)
				s[j + i] ^= t;
		}

		/* Rho Pi */
		t = s[1];
		for (i = 0; i < 24; i++) {
			j = keccakf_piln[i];
			bc[0] = s[j];
			s[j] = SHA3_ROTL64(t, keccakf_rotc[i]);
			t = bc[0];
		}

		/* Chi */
		for (j = 0; j < 25; j += 5) {
			for (i = 0; i < 5; i++)
				bc[i] = s[j + i];
			for (i = 0; i < 5; i++)
				s[j + i] ^=
				    (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
		}

		/* Iota */
		s[0] ^= keccakf_rndc[round];
	}
}

/* *************************** Public Inteface ************************ */

void SHA3_Init(SHA3_CTX * ctx, unsigned bitSize)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->digest_length = bitSize;
	ctx->capacityWords = 2 * bitSize / (8 * sizeof(uint64_t));
}

void SHA3_Update(SHA3_CTX * ctx, void const *bufIn, size_t len)
{
	/* 0...7 -- how much is needed to have a word */
	unsigned old_tail = (8 - ctx->byteIndex) & 7;

	size_t words;
	size_t tail;
	size_t i;

	const uint8_t *buf = bufIn;

	SHA3_ASSERT(ctx->byteIndex < 8);
	SHA3_ASSERT(ctx->wordIndex < sizeof(ctx->u.s) / sizeof(ctx->u.s[0]));

	if (len < old_tail) {	/* have no complete word or haven't started 
				 * the word yet */
		/* endian-independent code follows: */
		while (len--)
			ctx->saved |=
			    (uint64_t) (*(buf++)) << ((ctx->byteIndex++) * 8);
		SHA3_ASSERT(ctx->byteIndex < 8);
		return;
	}

	if (old_tail) {		/* will have one word to process */
		/* endian-independent code follows: */
		len -= old_tail;
		while (old_tail--)
			ctx->saved |=
			    (uint64_t) (*(buf++)) << ((ctx->byteIndex++) * 8);

		/* now ready to add saved to the sponge */
		ctx->u.s[ctx->wordIndex] ^= ctx->saved;
		SHA3_ASSERT(ctx->byteIndex == 8);
		ctx->byteIndex = 0;
		ctx->saved = 0;
		if (++ctx->wordIndex ==
		    (SHA3_KECCAK_SPONGE_WORDS - SHA3_CW(ctx->capacityWords))) {
			keccakf(ctx->u.s);
			ctx->wordIndex = 0;
		}
	}

	/* now work in full words directly from input */

	SHA3_ASSERT(ctx->byteIndex == 0);

	words = len / sizeof(uint64_t);
	tail = len - words * sizeof(uint64_t);

	for (i = 0; i < words; i++, buf += sizeof(uint64_t)) {
		const uint64_t t = (uint64_t) (buf[0]) |
		    ((uint64_t) (buf[1]) << 8 * 1) |
		    ((uint64_t) (buf[2]) << 8 * 2) |
		    ((uint64_t) (buf[3]) << 8 * 3) |
		    ((uint64_t) (buf[4]) << 8 * 4) |
		    ((uint64_t) (buf[5]) << 8 * 5) |
		    ((uint64_t) (buf[6]) << 8 * 6) |
		    ((uint64_t) (buf[7]) << 8 * 7);
#if defined(__x86_64__ ) || defined(__i386__)
		SHA3_ASSERT(memcmp(&t, buf, 8) == 0);
#endif
		ctx->u.s[ctx->wordIndex] ^= t;
		if (++ctx->wordIndex ==
		    (SHA3_KECCAK_SPONGE_WORDS - SHA3_CW(ctx->capacityWords))) {
			keccakf(ctx->u.s);
			ctx->wordIndex = 0;
		}
	}

	/* finally, save the partial word */
	SHA3_ASSERT(ctx->byteIndex == 0 && tail < 8);
	while (tail--) {
		ctx->saved |= (uint64_t) (*(buf++)) << ((ctx->byteIndex++) * 8);
	}
	SHA3_ASSERT(ctx->byteIndex < 8);
}

/* This is simply the 'update' with the padding block.
 * The padding block is 0x01 || 0x00* || 0x80. First 0x01 and last 0x80 
 * bytes are always present, but they can be the same byte.
 */
void SHA3_Final(void *res, SHA3_CTX * ctx)
{
	/* Append 2-bit suffix 01, per SHA-3 spec. Instead of 1 for padding we
	 * use 1<<2 below. The 0x02 below corresponds to the suffix 01.
	 * Overall, we feed 0, then 1, and finally 1 to start padding. Without
	 * M || 01, we would simply use 1 to start padding. */

	uint64_t t;

	/* SHA3 version */
	t = (uint64_t) (((uint64_t) (0x02 | (1 << 2))) <<
			((ctx->byteIndex) * 8));

	ctx->u.s[ctx->wordIndex] ^= ctx->saved ^ t;

	ctx->u.s[SHA3_KECCAK_SPONGE_WORDS - SHA3_CW(ctx->capacityWords) - 1] ^=
	    SHA3_CONST(0x8000000000000000UL);
	keccakf(ctx->u.s);

	/* Return first bytes of the ctx->s. This conversion is not needed for
	 * little-endian platforms e.g. wrap with #if !defined(__BYTE_ORDER__)
	 * || !defined(__ORDER_LITTLE_ENDIAN__) || __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__ 
	 *    ... the conversion below ...
	 * #endif */
	{
		unsigned i;
		for (i = 0; i < SHA3_KECCAK_SPONGE_WORDS; i++) {
			const unsigned t1 = (uint32_t) ctx->u.s[i];
			const unsigned t2 =
			    (uint32_t) ((ctx->u.s[i] >> 16) >> 16);
			ctx->u.sb[i * 8 + 0] = (uint8_t) (t1);
			ctx->u.sb[i * 8 + 1] = (uint8_t) (t1 >> 8);
			ctx->u.sb[i * 8 + 2] = (uint8_t) (t1 >> 16);
			ctx->u.sb[i * 8 + 3] = (uint8_t) (t1 >> 24);
			ctx->u.sb[i * 8 + 4] = (uint8_t) (t2);
			ctx->u.sb[i * 8 + 5] = (uint8_t) (t2 >> 8);
			ctx->u.sb[i * 8 + 6] = (uint8_t) (t2 >> 16);
			ctx->u.sb[i * 8 + 7] = (uint8_t) (t2 >> 24);
		}
	}

	memcpy(res, ctx->u.sb, ctx->digest_length/8);
}
