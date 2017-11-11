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

#ifndef RADYX_RANGE_ENCODER_H
#define RADYX_RANGE_ENCODER_H
#include "common.h"

namespace Radyx {

class RangeEncoder
{
public:
	typedef uint_least16_t Probability;
	static const unsigned kNumTopBits = 24;
	static const uint_fast32_t kTopValue = (UINT32_C(1) << kNumTopBits);
	static const unsigned kNumBitModelTotalBits = 11;
	static const Probability kBitModelTotal = (1 << kNumBitModelTotalBits);
	static const unsigned kNumMoveBits = 5;
	static const Probability kProbInitValue = (kBitModelTotal >> 1);
	static const unsigned kNumMoveReducingBits = 4;
	static const unsigned kNumBitPriceShiftBits = 4;

	RangeEncoder() noexcept;
	~RangeEncoder();
	void SetOutputBuffer(uint8_t *out_buffer_, size_t chunk_size_) noexcept;
	void Reset() noexcept;
	inline void EncodeBit0(Probability &rprob) noexcept;
	inline void EncodeBit1(Probability &rprob) noexcept;
	inline void EncodeBit(Probability &rprob, unsigned bit) noexcept;
	void EncodeBitTree(Probability *prob_table, unsigned bit_count, unsigned symbol) noexcept;
	void EncodeBitTreeReverse(Probability *prob_table, unsigned bit_count, unsigned symbol) noexcept;
	void EncodeDirect(unsigned value, unsigned bit_count) noexcept;
	void Flush() noexcept;
	static inline unsigned GetPrice(unsigned prob, unsigned symbol) noexcept;
	static inline unsigned GetPrice0(unsigned prob) noexcept;
	static inline unsigned GetPrice1(unsigned prob) noexcept;
	static inline unsigned GetTreePrice(const Probability* prob_table, unsigned bit_count, size_t symbol) noexcept;
	static inline unsigned GetReverseTreePrice(const Probability* prob_table, unsigned bit_count, size_t symbol) noexcept;
	static inline unsigned GetDirectBitsPrice(size_t bit_count) noexcept;

	inline size_t GetOutIndex() const noexcept {
		return out_index;
	}
	inline bool IsFull() const noexcept {
		return out_index >= chunk_size;
	}

private:
	static void InitPriceTable() noexcept;
	void ShiftLow() noexcept;

	static unsigned price_table[kBitModelTotal >> kNumMoveReducingBits];
	uint8_t *out_buffer;
	size_t out_index;
	size_t chunk_size;
	uint_fast64_t cache_size;
	uint_fast64_t low;
	uint_fast32_t range;
	uint8_t cache;

	static class init_
	{
	public:
		init_() noexcept { InitPriceTable(); }
	} initializer_;

	RangeEncoder(const RangeEncoder&) = delete;
	RangeEncoder& operator=(const RangeEncoder&) = delete;
	RangeEncoder(RangeEncoder&&) = delete;
	RangeEncoder& operator=(RangeEncoder&&) = delete;
};

void RangeEncoder::EncodeBit0(Probability &rprob) noexcept
{
	unsigned prob = rprob;
	range = (range >> kNumBitModelTotalBits) * prob;
	prob += (kBitModelTotal - prob) >> kNumMoveBits;
	rprob = static_cast<Probability>(prob);
	if (range < kTopValue) {
		range <<= 8;
		ShiftLow();
	}
}

void RangeEncoder::EncodeBit1(Probability &rprob) noexcept
{
	unsigned prob = rprob;
	uint_fast32_t new_bound = (range >> kNumBitModelTotalBits) * prob;
	low += new_bound;
	range -= new_bound;
	prob -= prob >> kNumMoveBits;
	rprob = static_cast<Probability>(prob);
	if (range < kTopValue) {
		range <<= 8;
		ShiftLow();
	}
}

void RangeEncoder::EncodeBit(Probability &rprob, unsigned bit) noexcept
{
	unsigned prob = rprob;
	if (bit != 0) {
		uint_fast32_t new_bound = (range >> kNumBitModelTotalBits) * prob;
		low += new_bound;
		range -= new_bound;
		prob -= prob >> kNumMoveBits;
	}
	else {
		range = (range >> kNumBitModelTotalBits) * prob;
		prob += (kBitModelTotal - prob) >> kNumMoveBits;
	}
	rprob = static_cast<Probability>(prob);
	if (range < kTopValue) {
		range <<= 8;
		ShiftLow();
	}
}

unsigned RangeEncoder::GetPrice(unsigned prob, unsigned symbol) noexcept
{
	return price_table[(prob ^ (-static_cast<int>(symbol) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];
}

unsigned RangeEncoder::GetPrice0(unsigned prob) noexcept
{
	return price_table[prob >> kNumMoveReducingBits];
}

unsigned RangeEncoder::GetPrice1(unsigned prob) noexcept
{
	return price_table[(prob ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits];
}

unsigned RangeEncoder::GetTreePrice(const Probability* prob_table, unsigned bit_count, size_t symbol) noexcept
{
	unsigned price = 0;
	symbol |= (size_t(1) << bit_count);
	while (symbol != 1) {
		size_t next_symbol = symbol >> 1;
		unsigned prob = prob_table[next_symbol];
		unsigned bit = static_cast<unsigned>(symbol) & 1;
		price += GetPrice(prob, bit);
		symbol = next_symbol;
	}
	return price;
}

unsigned RangeEncoder::GetReverseTreePrice(const Probability* prob_table, unsigned bit_count, size_t symbol) noexcept
{
	unsigned price = 0;
	size_t m = 1;
	for (unsigned i = bit_count; i != 0; --i) {
		unsigned prob = prob_table[m];
		unsigned bit = symbol & 1;
		symbol >>= 1;
		price += GetPrice(prob, bit);
		m = (m << 1) | bit;
	}
	return price;
}

unsigned RangeEncoder::GetDirectBitsPrice(size_t bit_count) noexcept
{
	return static_cast<unsigned>(bit_count) << kNumBitPriceShiftBits;
}

}

#endif // RADYX_RANGE_ENCODER_H