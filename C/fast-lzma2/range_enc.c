/*
* Bitwise range encoder by Igor Pavlov
* Modified by Conor McCarthy
*
* Public domain
*/

#include "fl2_internal.h"
#include "mem.h"
#include "range_enc.h"

const unsigned price_table[kBitModelTotal >> kNumMoveReducingBits] = {
    128, 103,  91,  84,  78,  73,  69,  66,
    63,  61,  58,  56,  54,  52,  51,  49,
    48,  46,  45,  44,  43,  42,  41,  40,
    39,  38,  37,  36,  35,  34,  34,  33,
    32,  31,  31,  30,  29,  29,  28,  28,
    27,  26,  26,  25,  25,  24,  24,  23,
    23,  22,  22,  22,  21,  21,  20,  20,
    19,  19,  19,  18,  18,  17,  17,  17,
    16,  16,  16,  15,  15,  15,  14,  14,
    14,  13,  13,  13,  12,  12,  12,  11,
    11,  11,  11,  10,  10,  10,  10,   9,
    9,   9,   9,   8,   8,   8,   8,   7,
    7,   7,   7,   6,   6,   6,   6,   5,
    5,   5,   5,   5,   4,   4,   4,   4,
    3,   3,   3,   3,   3,   2,   2,   2,
    2,   2,   2,   1,   1,   1,   1,   1
};

void SetOutputBuffer(RangeEncoder* const rc, BYTE *const out_buffer, size_t chunk_size)
{
    rc->out_buffer = out_buffer;
    rc->chunk_size = chunk_size;
    rc->out_index = 0;
}

void RangeEncReset(RangeEncoder* const rc)
{
    rc->low = 0;
    rc->range = (U32)-1;
    rc->cache_size = 1;
    rc->cache = 0;
}

void ShiftLow(RangeEncoder* const rc)
{
	if (rc->low < 0xFF000000 || rc->low > 0xFFFFFFFF)
	{
		BYTE temp = rc->cache;
		do {
			assert (rc->out_index < rc->chunk_size - 4096);
            rc->out_buffer[rc->out_index++] = temp + (BYTE)(rc->low >> 32);
            temp = 0xFF;
		} while (--rc->cache_size != 0);
        rc->cache = (BYTE)(rc->low >> 24);
	}
	++rc->cache_size;
    rc->low = (rc->low << 8) & 0xFFFFFFFF;
}

void EncodeBitTree(RangeEncoder* const rc, Probability *const probs, unsigned bit_count, unsigned symbol)
{
	size_t tree_index = 1;
    assert(bit_count > 0);
    do {
        unsigned bit;
		--bit_count;
		bit = (symbol >> bit_count) & 1;
		EncodeBit(rc, &probs[tree_index], bit);
		tree_index = (tree_index << 1) | bit;
	} while (bit_count != 0);
}

void EncodeBitTreeReverse(RangeEncoder* const rc, Probability *const probs, unsigned bit_count, unsigned symbol)
{
	unsigned tree_index = 1;
    assert(bit_count != 0);
    do {
		unsigned bit = symbol & 1;
		EncodeBit(rc, &probs[tree_index], bit);
		tree_index = (tree_index << 1) + bit;
		symbol >>= 1;
	} while (--bit_count != 0);
}

void EncodeDirect(RangeEncoder* const rc, unsigned value, unsigned bit_count)
{
	assert(bit_count > 0);
	do {
        rc->range >>= 1;
		--bit_count;
        rc->low += rc->range & -((int)(value >> bit_count) & 1);
		if (rc->range < kTopValue) {
            rc->range <<= 8;
			ShiftLow(rc);
		}
	} while (bit_count != 0);
}


