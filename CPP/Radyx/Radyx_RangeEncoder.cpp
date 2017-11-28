///////////////////////////////////////////////////////////////////////////////
//
// Class:   RangeEncoder
//          Bitwise range encoder
//
// Authors: Igor Pavlov
//          Conor McCarthy
//
// This file is part of Radyx.
//
// Radyx is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Radyx is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Radyx. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "RangeEncoder.h"

namespace Radyx {

RangeEncoder::init_ RangeEncoder::initializer_;
unsigned RangeEncoder::price_table[kBitModelTotal >> kNumMoveReducingBits];

void RangeEncoder::InitPriceTable() NOEXCEPT
{
	for (unsigned i = (1 << kNumMoveReducingBits) / 2; i < kBitModelTotal; i += (1 << kNumMoveReducingBits))
	{
		static const int kCyclesBits = kNumBitPriceShiftBits;
		unsigned w = i;
		unsigned bit_count = 0;
		int j;
		for (j = 0; j < kCyclesBits; ++j)
		{
			w = w * w;
			bit_count <<= 1;
			while (w >= (UINT32_C(1) << 16))
			{
				w >>= 1;
				bit_count++;
			}
		}
		price_table[i >> kNumMoveReducingBits] = ((kNumBitModelTotalBits << kCyclesBits) - 15 - bit_count);
	}
}

RangeEncoder::RangeEncoder() NOEXCEPT
{
	Reset();
}


RangeEncoder::~RangeEncoder()
{
}

void RangeEncoder::SetOutputBuffer(uint8_t *out_buffer_, size_t chunk_size_) NOEXCEPT
{
	out_buffer = out_buffer_;
	chunk_size = chunk_size_;
	out_index = 0;
}

void RangeEncoder::Reset() NOEXCEPT
{
	low = 0;
	range = UINT32_MAX;
	cache_size = 1;
	cache = 0;
}

void RangeEncoder::ShiftLow() NOEXCEPT
{
	if (low < 0xFF000000 || low > 0xFFFFFFFF)
	{
		uint8_t temp = cache;
		do {
			assert (out_index < chunk_size + 4096);
			out_buffer[out_index++] = temp + static_cast<uint8_t>(low >> 32);
			temp = 0xFF;
		} while (--cache_size != 0);
		cache = static_cast<uint8_t>(low >> 24);
	}
	++cache_size;
	low = (low << 8) & 0xFFFFFFFF;
}

void RangeEncoder::EncodeBitTree(Probability *probs, unsigned bit_count, unsigned symbol) NOEXCEPT
{
	assert(bit_count > 0);
	size_t tree_index = 1;
	do {
		--bit_count;
		unsigned bit = (symbol >> bit_count) & 1;
		EncodeBit(probs[tree_index], bit);
		tree_index = (tree_index << 1) | bit;
	} while (bit_count != 0);
}

void RangeEncoder::EncodeBitTreeReverse(Probability *probs, unsigned bit_count, unsigned symbol) NOEXCEPT
{
	assert(bit_count != 0);
	unsigned tree_index = 1;
	do {
		unsigned bit = symbol & 1;
		EncodeBit(probs[tree_index], bit);
		tree_index = (tree_index << 1) + bit;
		symbol >>= 1;
	} while (--bit_count != 0);
}

void RangeEncoder::EncodeDirect(unsigned value, unsigned bit_count) NOEXCEPT
{
	assert(bit_count > 0);
	do {
		range >>= 1;
		--bit_count;
		low += range & -(static_cast<int>(value >> bit_count) & 1);
		if (range < kTopValue) {
			range <<= 8;
			ShiftLow();
		}
	} while (bit_count != 0);
}

void RangeEncoder::Flush() NOEXCEPT
{
	for (int i = 0; i < 5; ++i)
		ShiftLow();
}

}