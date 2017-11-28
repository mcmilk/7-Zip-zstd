///////////////////////////////////////////////////////////////////////////////
//
// Class:   Lzma2Encoder
//          Encode a section of data in LZMA2 using a pre-built match table
//          
// Authors: Igor Pavlov
//          Conor McCarthy
//          Lasse Collin
//
// Copyright 2015 Conor McCarthy
// Based on 7-zip 9.20 copyright 2010 Igor Pavlov
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

#ifndef RADYX_LZMA2_ENCODER_H
#define RADYX_LZMA2_ENCODER_H

#include "common.h"
#include <algorithm>
#include <array>
#include "MatchTable.h"
#include "RangeEncoder.h"
#include "DataBlock.h"
#include "AsyncWriter.h"
#include "Lzma2Options.h"
#include "HashChain.h"

namespace Radyx {

class Lzma2Encoder
{
public:
	Lzma2Encoder() NOEXCEPT;
	template<class MatchTableT>
	size_t Encode(MatchTable<MatchTableT>& match_table,
		const DataBlock& block,
		size_t start,
		size_t end,
		const Lzma2Options& options,
		bool do_random_check,
		Progress* progress,
		AsyncWriter* writer);

	inline bool NeededRandomCheck() const NOEXCEPT {
		return needed_random_check;
	}
	static inline unsigned GetMatchLenMax() NOEXCEPT {
		return kMatchLenMax;
	}
	static inline unsigned GetLiteralContextBitsMax() NOEXCEPT {
		return kNumLiteralContextBitsMax;
	}
	static inline unsigned GetLiteralPositionBitsMax() NOEXCEPT {
		return kNumLiteralPosBitsMax;
	}
	static inline unsigned GetPositionBitsMax() NOEXCEPT {
		return kNumPositionBitsMax;
	}
	static size_t GetDictionarySizeMax() NOEXCEPT;

	static unsigned GetDictionaryBitsMin() NOEXCEPT {
		return kDicLogSizeMin;
	}
	static unsigned Get2ndDictionaryBitsMin() NOEXCEPT {
		return 0;
	}
	static unsigned Get2ndDictionaryBitsMax() NOEXCEPT {
		return 16;
	}
	// User input constraints. The actual max size is 2 bytes less than 2^32 but we allow the user to set 2^32.
	static size_t GetUserDictionarySizeMin() NOEXCEPT {
		return UINT32_C(1) << kDicLogSizeMin;
	}
	static size_t GetUserDictionarySizeMax() NOEXCEPT;
	static size_t GetMemoryUsage(const Lzma2Options& options) NOEXCEPT;
	static uint8_t GetDictSizeProp(size_t dictionary_size) NOEXCEPT;

private:
	static const unsigned kNumReps = 4;
	static const unsigned kNumStates = 12;
	static const uint8_t kLiteralNextStates[kNumStates];
	static const uint8_t kMatchNextStates[kNumStates];
	static const uint8_t kRepNextStates[kNumStates];
	static const uint8_t kShortRepNextStates[kNumStates];

	static const unsigned kNumLiterals = 0x100;
	static const unsigned kNumLitTables = 3;

	static const unsigned kNumLenToPosStates = 4;
	static const unsigned kNumPosSlotBits = 6;
	static const unsigned kDicLogSizeMin = 18;
	static const unsigned kDicLogSizeMax = 32;
	static const uint_fast32_t k32BitDicSizeMax = UINT32_C(192) * 1024 * 1024;
	static const unsigned kDistTableSizeMax = kDicLogSizeMax * 2;

	static const unsigned kNumAlignBits = 4;
	static const unsigned kAlignTableSize = 1 << kNumAlignBits;
	static const unsigned kAlignMask = kAlignTableSize - 1;
	static const unsigned kAlignRepriceFrequency = kAlignTableSize;

	static const unsigned kStartPosModelIndex = 4;
	static const unsigned kEndPosModelIndex = 14;
	static const unsigned kNumPosModels = kEndPosModelIndex - kStartPosModelIndex;

	static const unsigned kNumFullDistancesBits = kEndPosModelIndex >> 1;
	static const unsigned kNumFullDistances = 1 << kNumFullDistancesBits;
	static const unsigned kDistanceRepriceFrequency = 1 << 7;

	static const unsigned kNumPositionBitsMax = 4;
	static const unsigned kNumPositionStatesMax = 1 << kNumPositionBitsMax;
	static const unsigned kNumLiteralContextBitsMax = 4;
	static const unsigned kNumLiteralPosBitsMax = 4;
	static const unsigned kLcLpMax = 4;


	static const unsigned kLenNumLowBits = 3;
	static const unsigned kLenNumLowSymbols = 1 << kLenNumLowBits;
	static const unsigned kLenNumMidBits = 3;
	static const unsigned kLenNumMidSymbols = 1 << kLenNumMidBits;
	static const unsigned kLenNumHighBits = 8;
	static const unsigned kLenNumHighSymbols = 1 << kLenNumHighBits;

	static const unsigned kLenNumSymbolsTotal = kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols;

	static const unsigned kMatchLenMin = 2;
	static const unsigned kMatchLenMax = kMatchLenMin + kLenNumSymbolsTotal - 1;

	static const uint8_t kFastDistBits = 12;

	static const unsigned kOptimizerBufferSize = 1 << 11;
	static const uint_fast32_t kInfinityPrice = UINT32_C(1) << 30;

	static const uint_fast32_t kChunkSize = (UINT32_C(1) << 16) - 8192;
	static const uint_fast32_t kChunkBufferSize = UINT32_C(1) << 16;
	static const uint_fast32_t kMaxChunkUncompressedSize = (UINT32_C(1) << 21) - kMatchLenMax;
	static const unsigned kChunkHeaderSize = 5;
	static const unsigned kChunkResetShift = 5;
	static const uint8_t kChunkUncompressedDictReset = 1;
	static const uint8_t kChunkUncompressed = 2;
	static const uint8_t kChunkCompressedFlag = 0x80u;
	static const uint8_t kChunkNothingReset = 0;
	static const uint8_t kChunkStateReset = 1 << kChunkResetShift;
	static const uint8_t kChunkStatePropertiesReset = 2 << kChunkResetShift;
	static const uint8_t kChunkAllReset = 3 << kChunkResetShift;
	static const unsigned kRandomFilterMarginBits = 7;

	static const unsigned kHash2Bits = 10;
	static const unsigned kHash3Bits = 16;
	typedef HashChain<kHash2Bits, kHash3Bits, kMatchLenMax> LzmaHashChain;

	struct RepDistances
	{
		union {
			uint_fast32_t reps[kNumReps];
			size_t rep_copier[kNumReps / 2];
		};
		uint_fast32_t& operator[](size_t index) NOEXCEPT {
			return reps[index];
		}
		const uint_fast32_t& operator[](size_t index) const NOEXCEPT {
			return reps[index];
		}
		inline void operator=(const RepDistances& rvalue) NOEXCEPT;
	};

	struct LengthStates
	{
		size_t table_size;
		unsigned prices[kNumPositionStatesMax][kLenNumSymbolsTotal];
		unsigned counters[kNumPositionStatesMax];
		RangeEncoder::Probability choice;
		RangeEncoder::Probability choice_2;
		RangeEncoder::Probability low[kNumPositionStatesMax << kLenNumLowBits];
		RangeEncoder::Probability mid[kNumPositionStatesMax << kLenNumMidBits];
		RangeEncoder::Probability high[kLenNumHighSymbols];
		void Reset(unsigned fast_length) NOEXCEPT;
		void SetPrices(size_t pos_state) NOEXCEPT;
	};

	struct EncoderStates
	{
		RepDistances reps;
		size_t state;

		RangeEncoder::Probability is_rep[kNumStates];
		RangeEncoder::Probability is_rep_G0[kNumStates];
		RangeEncoder::Probability is_rep_G1[kNumStates];
		RangeEncoder::Probability is_rep_G2[kNumStates];
		RangeEncoder::Probability is_rep0_long[kNumStates][kNumPositionStatesMax];
		RangeEncoder::Probability is_match[kNumStates][kNumPositionStatesMax];

		RangeEncoder::Probability dist_slot_encoders[kNumLenToPosStates][1 << kNumPosSlotBits];
		RangeEncoder::Probability dist_align_encoders[1 << kNumAlignBits];
		RangeEncoder::Probability dist_encoders[kNumFullDistances - kEndPosModelIndex];

		LengthStates len_states;
		LengthStates rep_len_states;

		RangeEncoder::Probability literal_probs[(kNumLiterals * kNumLitTables) << kLcLpMax];

		void Reset(unsigned lc, unsigned lp, unsigned fast_length) NOEXCEPT;
	};

	struct OptimalNode
	{
		size_t state;
		RepDistances reps;
		uint_fast32_t price;
		unsigned prev_index;
		uint_fast32_t prev_dist;
		unsigned prev_index_2;
		uint_fast32_t prev_dist_2;
		bool is_combination;
		bool prev_2;

		void MakeAsLiteral() NOEXCEPT {
			prev_dist = UINT32_MAX; is_combination = false;
		}
		void MakeAsShortRep() NOEXCEPT {
			prev_dist = 0; is_combination = false;
		}
		bool IsShortRep() const NOEXCEPT {
			return prev_dist == 0;
		}
		bool IsLiteral() const NOEXCEPT {
			return prev_dist == UINT32_MAX;
		}
	};
	typedef std::array<OptimalNode, kOptimizerBufferSize> OptimalArray;

	static void InitDistanceTable() NOEXCEPT;
	void Reset(size_t max_distance) NOEXCEPT;
	static inline unsigned Get2Bytes(const uint8_t* data) NOEXCEPT;
	static inline bool Compare2Bytes(const uint8_t* data, const uint8_t* data_2) NOEXCEPT;
	static inline size_t FindRepMatchLength(const uint8_t* data, const uint8_t* data_2, size_t start_length, size_t max_length) NOEXCEPT;
	inline RangeEncoder::Probability* GetLiteralProbs(size_t pos, unsigned prev_symbol) NOEXCEPT;
	inline const RangeEncoder::Probability* GetLiteralProbs(size_t pos, unsigned prev_symbol) const NOEXCEPT;
	static inline unsigned FastDistShift(unsigned n) NOEXCEPT;
	static inline unsigned FastDistResult(uint_fast32_t dist, unsigned n) NOEXCEPT;
	static size_t GetDistSlot(uint_fast32_t distance) NOEXCEPT;
	template<class T>
	static inline size_t GetLenToDistState(T len) NOEXCEPT;
	static inline bool IsCharState(size_t state) NOEXCEPT;
	inline unsigned GetRepLen1Price(size_t state, size_t pos_state) const NOEXCEPT;
	inline unsigned GetRepPrice(size_t rep_index, size_t state, size_t pos_state) const NOEXCEPT;
	inline unsigned GetRepMatch0Price(size_t len, size_t state, size_t pos_state) const NOEXCEPT;
	unsigned GetLiteralPrice(size_t index, size_t state, unsigned prev_symbol, uint_fast32_t symbol, unsigned match_byte) const NOEXCEPT;
	inline unsigned GetLiteralPriceMatched(const RangeEncoder::Probability *prob_table, uint_fast32_t symbol, unsigned match_byte) const NOEXCEPT;
	void FillAlignPrices() NOEXCEPT;
	void FillDistancesPrices() NOEXCEPT;
	inline void EncodeLiteral(const uint8_t* data_block, size_t index) NOEXCEPT;
	void EncodeLiteral(size_t index, uint_fast32_t symbol, unsigned prev_symbol) NOEXCEPT;
	void EncodeLiteralMatched(const uint8_t* data_block, size_t index, uint_fast32_t symbol) NOEXCEPT;
	void UpdateLengthPrices(LengthStates &len_states) NOEXCEPT;
	void EncodeLength(LengthStates& len_prob_table, unsigned len, size_t pos_state) NOEXCEPT;
	void EncodeRepMatch(unsigned len, unsigned rep, size_t pos_state) NOEXCEPT;
	void EncodeNormalMatch(unsigned len, uint_fast32_t dist, size_t pos_state) NOEXCEPT;
	template<class MatchTableT>
	inline MatchResult GetOptimumFast(const DataBlock& block, MatchTable<MatchTableT> &match_table, size_t index) const NOEXCEPT;
	size_t InitOptimizerPos0(const DataBlock& block,
		MatchResult match,
		size_t index,
		RepDistances& reps,
		OptimalArray& opt_buf) NOEXCEPT;
	void InitMatchesPos0(const DataBlock& block,
		MatchResult match,
		size_t pos_state,
		size_t start_len,
		unsigned normal_match_price,
		OptimalArray& opt_buf) NOEXCEPT;
	size_t InitMatchesPos0Best(const DataBlock& block,
		MatchResult match,
		size_t index,
		size_t start_len,
		unsigned normal_match_price,
		OptimalArray& opt_buf) NOEXCEPT;
	size_t OptimalParse(const DataBlock& block,
		MatchResult match,
		size_t index,
		size_t cur,
		size_t len_end,
		RepDistances& reps,
		OptimalArray& opt_buf) NOEXCEPT;
	inline void ReverseOptimalChain(OptimalArray& opt_buf, size_t cur) const NOEXCEPT;
	template<class MatchTableT>
	inline size_t EncodeOptimumSequence(const DataBlock& block,
		MatchTable<MatchTableT> &match_table,
		size_t index,
		size_t uncompressed_end,
		OptimalArray& opt_buf) NOEXCEPT;
	uint8_t GetLcLpPbCode() NOEXCEPT;
	template<class MatchTableT>
	size_t EncodeChunkFast(const DataBlock& block,
		MatchTable<MatchTableT> &match_table,
		size_t index,
		size_t uncompressed_end) NOEXCEPT;
	template<class MatchTableT>
	size_t EncodeChunkBest(const DataBlock& block,
		MatchTable<MatchTableT> &match_table,
		size_t index,
		size_t uncompressed_end,
		OptimalArray& opt_buf) NOEXCEPT;

	static uint8_t distance_table[1 << kFastDistBits];

	unsigned lc;
	unsigned lp;
	unsigned pb;
	unsigned fast_length;
	size_t len_end_max;
	size_t lit_pos_mask;
	size_t pos_mask;
	unsigned match_cycles;
	Lzma2Options::Mode encoder_mode;

	RangeEncoder rc;

	EncoderStates encoder_states;

	unsigned match_price_count;
	unsigned align_price_count;
	size_t dist_price_table_size;
	unsigned align_prices[kAlignTableSize];
	unsigned dist_slot_prices[kNumLenToPosStates][kDistTableSizeMax];
	unsigned distance_prices[kNumLenToPosStates][kNumFullDistances];

	std::unique_ptr<LzmaHashChain> hash_chain;
	MatchCollection<kMatchLenMin, kMatchLenMax> matches;

	bool needed_random_check;

	static class init_
	{
	public:
		init_() NOEXCEPT { InitDistanceTable(); }
	} initializer_;

	Lzma2Encoder(const Lzma2Encoder&) = delete;
	Lzma2Encoder(Lzma2Encoder&&) = delete;
	Lzma2Encoder& operator=(const Lzma2Encoder&) = delete;
	Lzma2Encoder& operator=(Lzma2Encoder&&) = delete;
};

__pragma(warning(push))
__pragma(warning(disable:4127))
void Lzma2Encoder::RepDistances::operator=(const RepDistances& rvalue) NOEXCEPT
{
	if (sizeof(rep_copier) == sizeof(reps)) {
		rep_copier[0] = rvalue.rep_copier[0];
		rep_copier[1] = rvalue.rep_copier[1];
	}
	else {
		reps[0] = rvalue.reps[0];
		reps[1] = rvalue.reps[1];
		reps[2] = rvalue.reps[2];
		reps[3] = rvalue.reps[3];
	}
}
__pragma(warning(pop))

unsigned Lzma2Encoder::Get2Bytes(const uint8_t* data) NOEXCEPT
{
#ifndef DISALLOW_UNALIGNED_ACCESS
	return (reinterpret_cast<const uint16_t*>(data))[0];
#else
	return data[0] | (data[1] << 8);
#endif
}

bool Lzma2Encoder::Compare2Bytes(const uint8_t* data, const uint8_t* data_2) NOEXCEPT
{
#ifndef DISALLOW_UNALIGNED_ACCESS
	return (reinterpret_cast<const uint16_t*>(data))[0] == (reinterpret_cast<const uint16_t*>(data_2))[0];
#else
	return data[0] == data_2[0] && data[1] == data_2[1];
#endif
}

inline size_t Lzma2Encoder::FindRepMatchLength(const uint8_t* data, const uint8_t* data_2, size_t start_length, size_t max_length) NOEXCEPT
{
	size_t len = start_length;
	for (; len < max_length && data[len] == data_2[len]; ++len) {
	}
	return len;
}

RangeEncoder::Probability* Lzma2Encoder::GetLiteralProbs(size_t pos, unsigned prev_symbol) NOEXCEPT
{
	return encoder_states.literal_probs + (((pos & lit_pos_mask) << lc) + (prev_symbol >> (8 - lc))) * kNumLiterals * kNumLitTables;
}

const RangeEncoder::Probability* Lzma2Encoder::GetLiteralProbs(size_t pos, unsigned prev_symbol) const NOEXCEPT
{
	return encoder_states.literal_probs + (((pos & lit_pos_mask) << lc) + (prev_symbol >> (8 - lc))) * kNumLiterals * kNumLitTables;
}

template<class T>
size_t Lzma2Encoder::GetLenToDistState(T len) NOEXCEPT
{
	return (len < kNumLenToPosStates + 1) ? len - 2 : kNumLenToPosStates - 1;
}

bool Lzma2Encoder::IsCharState(size_t state) NOEXCEPT
{
	return state < 7;
}

unsigned Lzma2Encoder::GetRepLen1Price(size_t state, size_t pos_state) const NOEXCEPT
{
	unsigned rep_G0_prob = encoder_states.is_rep_G0[state];
	unsigned rep0_long_prob = encoder_states.is_rep0_long[state][pos_state];
	return rc.GetPrice0(rep_G0_prob) + rc.GetPrice0(rep0_long_prob);
}

unsigned Lzma2Encoder::GetRepPrice(size_t rep_index, size_t state, size_t pos_state) const NOEXCEPT
{
	unsigned price;
	unsigned rep_G0_prob = encoder_states.is_rep_G0[state];
	if (rep_index == 0) {
		unsigned rep0_long_prob = encoder_states.is_rep0_long[state][pos_state];
		price = rc.GetPrice0(rep_G0_prob);
		price += rc.GetPrice1(rep0_long_prob);
	}
	else {
		unsigned rep_G1_prob = encoder_states.is_rep_G1[state];
		price = rc.GetPrice1(rep_G0_prob);
		if (rep_index == 1) {
			price += rc.GetPrice0(rep_G1_prob);
		}
		else {
			unsigned rep_G2_prob = encoder_states.is_rep_G2[state];
			price += rc.GetPrice1(rep_G1_prob);
			price += rc.GetPrice(rep_G2_prob, static_cast<uint_fast32_t>(rep_index) - 2);
		}
	}
	return price;
}

unsigned Lzma2Encoder::GetRepMatch0Price(size_t len, size_t state, size_t pos_state) const NOEXCEPT
{
	unsigned rep_G0_prob = encoder_states.is_rep_G0[state];
	unsigned rep0_long_prob = encoder_states.is_rep0_long[state][pos_state];
	return encoder_states.rep_len_states.prices[pos_state][len - kMatchLenMin]
		+ rc.GetPrice0(rep_G0_prob)
		+ rc.GetPrice1(rep0_long_prob);
}

unsigned Lzma2Encoder::GetLiteralPriceMatched(const RangeEncoder::Probability *prob_table, uint_fast32_t symbol, unsigned match_byte) const NOEXCEPT
{
	unsigned price = 0;
	unsigned offs = 0x100;
	symbol |= 0x100;
	do {
		match_byte <<= 1;
		price += rc.GetPrice(prob_table[offs + (match_byte & offs) + (symbol >> 8)], (symbol >> 7) & 1);
		symbol <<= 1;
		offs &= ~(match_byte ^ symbol);
	} while (symbol < 0x10000);
	return price;
}

void Lzma2Encoder::EncodeLiteral(const uint8_t* data_block, size_t index) NOEXCEPT
{
	uint_fast32_t symbol = data_block[index];
	if (IsCharState(encoder_states.state)) {
		unsigned prev_symbol = data_block[index - 1];
		EncodeLiteral(index, symbol, prev_symbol);
	}
	else {
		EncodeLiteralMatched(data_block, index, symbol);
	}
}

static inline bool ChangePair(uint_fast32_t small_dist, uint_fast32_t big_dist) NOEXCEPT
{
	return (big_dist >> 7) > small_dist;
}

__pragma(warning(push))
__pragma(warning(disable:4701))

template<class MatchTableT>
MatchResult Lzma2Encoder::GetOptimumFast(const DataBlock& block, MatchTable<MatchTableT> &match_table, size_t index) const NOEXCEPT
{
	// Table of distance restrictions for short matches
	static const std::array<uint_fast32_t, 5> max_dist_table = { 0, 0, 1 << 7, 1 << 16, 1 << 23 };
	// Get a match from the table, extended to its full length
	MatchResult match = match_table.template GetMatch<kMatchLenMax>(block, index);
	// Discard if too far
	if (match.length < max_dist_table.size() && max_dist_table[match.length] < match.dist) {
		match.length = 0;
	}
	size_t max_len = std::min<size_t>(kMatchLenMax, block.end - index);
	unsigned rep_len = 0;
	unsigned rep_index;
	const uint8_t* data = block.data + index;
	for (unsigned i = 0; i < kNumReps; ++i)	{
		const uint8_t *data_2 = data - encoder_states.reps[i] - 1;
		if (!Compare2Bytes(data, data_2)) {
			continue;
		}
		unsigned len = static_cast<unsigned>(FindRepMatchLength(data, data_2, 2, max_len));
		if (len >= fast_length) {
			return MatchResult(len, i);
		}
		if (len > rep_len) {
			rep_index = i;
			rep_len = len;
		}
	}
	if (match.length >= fast_length) {
		return MatchResult(match.length, match.dist + kNumReps);
	}
	if (rep_len >= 2 && (
		(rep_len + 1 >= match.length) ||
		(rep_len + 2 >= match.length && match.dist >= (1 << 9)) ||
		(rep_len + 3 >= match.length && match.dist >= (1 << 15))))
	{
		return MatchResult(rep_len, rep_index);
	}
	if (match.length == 0) {
		return MatchResult(0, 0);
	}
	if (match_table.HaveMatch(index + 1)) {
		// A more sophisticated kind of "lazy matching", from 7-zip
		MatchResult next_match = match_table.template GetMatch<kMatchLenMax>(block, index + 1);
		if ((next_match.length >= match.length && next_match.dist < match.dist) ||
			(next_match.length == match.length + 1 && !ChangePair(match.dist, next_match.dist)) ||
			(next_match.length > match.length + 1) ||
			(next_match.length + 1 >= match.length && match.length >= 3 && ChangePair(next_match.dist, match.dist)))
		{
			return MatchResult(0, 0);
		}
		++data;
		// If there's a rep match at the next byte, encode a literal
		for (unsigned i = 0; i < kNumReps; ++i) {
			const uint8_t* data_2 = data - (encoder_states.reps[i] + 1);
			if (!Compare2Bytes(data, data_2)) {
				continue;
			}
			unsigned limit = match.length - 1;
			unsigned len = static_cast<unsigned>(FindRepMatchLength(data, data_2, 2, limit));
			if (len >= limit) {
				return MatchResult(0, 0);
			}
		}
	}
	return MatchResult(match.length, match.dist + kNumReps);
}

__pragma(warning(pop))

template<class MatchTableT>
size_t Lzma2Encoder::EncodeChunkFast(const DataBlock& block,
	MatchTable<MatchTableT> &match_table,
	size_t index,
	size_t uncompressed_end) NOEXCEPT
{
	while (index < uncompressed_end && !rc.IsFull())
	{
		MatchResult match;
		match.length = 0;
		if (match_table.HaveMatch(index)) {
			match = GetOptimumFast(block, match_table, index);
		}
		assert(index + match.length <= block.end);
		if (match.length == 0) {
			if (block.data[index] == block.data[index - encoder_states.reps[0] - 1]) {
				EncodeRepMatch(1, 0, index & pos_mask);
			}
			else {
				EncodeLiteral(block.data, index);
			}
			++index;
		}
		else {
			if (match.dist < kNumReps) {
				EncodeRepMatch(match.length, match.dist, index & pos_mask);
			}
			else {
				EncodeNormalMatch(match.length, match.dist - kNumReps, index & pos_mask);
			}
			index += match.length;
		}
	}
	rc.Flush();
	return index;
}

// Reverse the direction of the linked list generated by the optimal parser
void Lzma2Encoder::ReverseOptimalChain(OptimalArray& opt_buf, size_t cur) const NOEXCEPT
{
	size_t next_index = opt_buf[cur].prev_index;
	uint_fast32_t next_dist = opt_buf[cur].prev_dist;
	do
	{
		if (opt_buf[cur].is_combination)
		{
			opt_buf[next_index].MakeAsLiteral();
			opt_buf[next_index].prev_index = static_cast<unsigned>(next_index - 1);
			if (opt_buf[cur].prev_2)
			{
				opt_buf[next_index - 1].is_combination = false;
				opt_buf[next_index - 1].prev_index = opt_buf[cur].prev_index_2;
				opt_buf[next_index - 1].prev_dist = opt_buf[cur].prev_dist_2;
			}
		}
		std::swap(next_dist, opt_buf[next_index].prev_dist);
		size_t prev_index = next_index;
		next_index = opt_buf[prev_index].prev_index;
		opt_buf[prev_index].prev_index = static_cast<unsigned>(cur);
		cur = prev_index;
	} while (cur != 0);
}

template<class MatchTableT>
size_t Lzma2Encoder::EncodeOptimumSequence(const DataBlock& block,
	MatchTable<MatchTableT> &match_table,
	size_t start_index,
	size_t uncompressed_end,
	OptimalArray& opt_buf) NOEXCEPT
{
	size_t len_end = len_end_max;
	MatchResult match = match_table.template GetMatch<kMatchLenMax>(block, start_index);
	do {
		for (; (len_end & 3) != 0; --len_end) {
			opt_buf[len_end].price = kInfinityPrice;
		}
		for (; len_end >= 4; len_end -= 4) {
			opt_buf[len_end].price = kInfinityPrice;
			opt_buf[len_end - 1].price = kInfinityPrice;
			opt_buf[len_end - 2].price = kInfinityPrice;
			opt_buf[len_end - 3].price = kInfinityPrice;
		}
		RepDistances reps;
		size_t index = start_index;
		// Set everything up at position 0
		len_end = InitOptimizerPos0(block, match, index, reps, opt_buf);
		match.length = 0;
		size_t cur = 1;
		// len_end == 0 if a match of fast_length was found
		if (len_end > 0) {
			++index;
			// Lazy termination of the optimal parser. In the second half of the buffer
			// a resolution within one byte is enough
			for (; cur < (len_end - cur / (opt_buf.size() / 2u)); ++cur, ++index) {
				if (match_table.HaveMatch(index)) {
					match = match_table.template GetMatch<kMatchLenMax>(block, index);
					if (match.length >= fast_length) {
						break;
					}
				}
				else match.length = 0;
				len_end = OptimalParse(block, match, index, cur, len_end, reps, opt_buf);
			}
			if (cur < len_end && match.length < fast_length) {
				// Adjust the end point base on scaling up the price.
				cur += (opt_buf[cur].price + opt_buf[cur].price / cur) >= opt_buf[cur + 1].price;
			}
			ReverseOptimalChain(opt_buf, cur);
		}
		unsigned prev_index = 0;
		size_t i = 0;
		// Encode the selections in the buffer
		do {
			unsigned len = opt_buf[i].prev_index - prev_index;
			prev_index = opt_buf[i].prev_index;
			if (len == 1 && opt_buf[i].IsLiteral())
			{
				EncodeLiteral(block.data, start_index + i);
			}
			else {
				size_t match_index = start_index + i;
				uint_fast32_t dist = opt_buf[i].prev_dist;
				// The last match will be truncated to fit in the optimal buffer so get the full length
				if (i + len >= opt_buf.size() - 1 && dist >= kNumReps && match_table.HaveMatch(match_index)) {
					MatchResult lastmatch = match_table.template GetMatch<kMatchLenMax>(block, match_index);
					if (lastmatch.length > len) {
						len = lastmatch.length;
						dist = lastmatch.dist + kNumReps;
					}
				}
				if (dist < kNumReps) {
					EncodeRepMatch(len, dist, match_index & pos_mask);
				}
				else {
					EncodeNormalMatch(len, dist - kNumReps, match_index & pos_mask);
				}
			}
			i += len;
		} while (i < cur);
		start_index += i;
		// Do another round if there is a long match pending, because the reps must be checked
		// and the match encoded.
	} while (match.length >= fast_length && start_index < uncompressed_end && !rc.IsFull());
	len_end_max = len_end;
	return start_index;
}

template<class MatchTableT>
size_t Lzma2Encoder::EncodeChunkBest(const DataBlock& block,
	MatchTable<MatchTableT> &match_table,
	size_t index,
	size_t uncompressed_end,
	OptimalArray& opt_buf) NOEXCEPT
{
	FillDistancesPrices();
	FillAlignPrices();
	UpdateLengthPrices(encoder_states.len_states);
	UpdateLengthPrices(encoder_states.rep_len_states);
	while (index < uncompressed_end && !rc.IsFull())
	{
		if (match_table.HaveMatch(index)) {
			index = EncodeOptimumSequence(block, match_table, index, uncompressed_end, opt_buf);
			if (match_price_count >= kDistanceRepriceFrequency) {
				FillDistancesPrices();
			}
			if (align_price_count >= kAlignRepriceFrequency) {
				FillAlignPrices();
			}
		}
		else {
			if (block.data[index] == block.data[index - encoder_states.reps[0] - 1]) {
				EncodeRepMatch(1, 0, index & pos_mask);
			}
			else {
				EncodeLiteral(block.data, index);
			}
			++index;
		}
	}
	rc.Flush();
	return index;
}

template<class MatchTableT>
size_t Lzma2Encoder::Encode(MatchTable<MatchTableT>& match_table,
	const DataBlock& block,
	size_t start,
	size_t end,
	const Lzma2Options& options,
	bool do_random_check,
	Progress* progress,
	AsyncWriter* writer)
{
	if (end <= start) {
		return 0;
	}
	lc = options.lc;
	lp = options.lp;
	if (lc + lp > 4) {
		lc = 3;
		lp = 0;
	}
	pb = options.pb;
	encoder_mode = options.encoder_mode;
	fast_length = options.fast_length; 
	match_cycles = options.match_cycles;
	Reset(block.end);
	if (encoder_mode == Lzma2Options::kBestMode) {
		// Create a hash chain to put the encoder into hybrid mode
		if (hash_chain.get() == nullptr) {
			hash_chain.reset(new LzmaHashChain(options.second_dict_bits));
		}
		hash_chain->Initialize(start >= (1 << kHash2Bits) ? start - (1 << kHash2Bits) : -1);
	}
	uint8_t out_buffer[kChunkBufferSize];
	uint8_t* out_dest = out_buffer;
	DataBlock encoder_block = block;
	encoder_block.end = end;
	// Each encoder writes a properties byte because the upstream encoder(s) could
	// write only uncompressed chunks with no properties.
	bool encode_properties = true;
	bool next_is_random = false;
	needed_random_check = false;
	if (do_random_check) {
		// Check if the next chunk is compressible
		next_is_random = match_table.IsRandom(encoder_block, start, kChunkSize);
	}
	OptimalArray opt_buf;
	len_end_max = opt_buf.size() - 1;
	for (size_t index = start; index < end && !g_break;)
	{
		unsigned header_size = encode_properties ? kChunkHeaderSize + 1 : kChunkHeaderSize;
		rc.Reset();
		rc.SetOutputBuffer(out_dest + header_size, kChunkSize);
		EncoderStates saved_states;
		bool try_encoding = !next_is_random && !match_table.IsRandom();
		size_t next_index;
		if (try_encoding) {
			saved_states = encoder_states;
			if (index == 0) {
				EncodeLiteral(0, block.data[0], 0);
			}
			if (encoder_mode == Lzma2Options::kFastMode) {
				next_index = EncodeChunkFast(encoder_block,
					match_table,
					index + (index == 0),
					std::min(encoder_block.end, index + kMaxChunkUncompressedSize));
			}
			else {
				next_index = EncodeChunkBest(encoder_block,
					match_table,
					index + (index == 0),
					std::min(encoder_block.end, index + kMaxChunkUncompressedSize - opt_buf.size()),
					opt_buf);
			}
		}
		else {
			next_index = std::min(index + kChunkSize, end);
		}
		size_t compressed_size = rc.GetOutIndex();
		size_t uncompressed_size = next_index - index;
		out_dest[1] = static_cast<uint8_t>((uncompressed_size - 1) >> 8);
		out_dest[2] = static_cast<uint8_t>(uncompressed_size - 1);
		// Output an uncompressed chunk if necessary
		if (next_is_random || match_table.IsRandom() || uncompressed_size + 3 <= compressed_size + header_size) {
			if (index == 0) {
				out_dest[0] = kChunkUncompressedDictReset;
			}
			else {
				out_dest[0] = kChunkUncompressed;
			}
			memcpy(out_dest + 3, block.data + index, uncompressed_size);
			compressed_size = uncompressed_size;
			header_size = 3;
			if (try_encoding) {
				encoder_states = saved_states;
			}
		}
		else {
			if (index == 0) {
				out_dest[0] = kChunkCompressedFlag | kChunkAllReset;
			}
			else if (encode_properties) {
				out_dest[0] = kChunkCompressedFlag | kChunkStatePropertiesReset;
			}
			else {
				out_dest[0] = kChunkCompressedFlag | kChunkNothingReset;
			}
			out_dest[0] |= static_cast<uint8_t>((uncompressed_size - 1) >> 16);
			out_dest[3] = static_cast<uint8_t>((compressed_size - 1) >> 8);
			out_dest[4] = static_cast<uint8_t>(compressed_size - 1);
			if (encode_properties) {
				out_dest[5] = GetLcLpPbCode();
				encode_properties = false;
			}
		}
		if (!match_table.IsRandom()
			&& (next_is_random || uncompressed_size + 3 <= compressed_size + (compressed_size >> kRandomFilterMarginBits) + header_size))
		{
			if (end - index < kChunkSize * 2u) {
				// Record that this encoder may have had incompressible data at the end
				needed_random_check |= true;
			}
			// Test the next chunk for compressibility
			next_is_random = match_table.IsRandom(encoder_block, next_index, kChunkSize);
		}
		if (index == start) {
			// After the first chunk we can write data to the match table because the
			// compressed data will never catch up with the table position being read.
			out_dest = match_table.GetOutputByteBuffer(start);
			memcpy(out_dest, out_buffer, compressed_size + header_size);
		}
		if (writer != nullptr) {
			if (writer->Fail()) {
				break;
			}
			writer->Write(out_dest, compressed_size + header_size);
		}
		out_dest += compressed_size + header_size;
		if (progress != nullptr) {
			progress->EncodeUpdate(uncompressed_size);
		}
		index = next_index;
	}
	return out_dest - match_table.GetOutputByteBuffer(start);
}

}

#endif // RADYX_LZMA2_ENCODER_H