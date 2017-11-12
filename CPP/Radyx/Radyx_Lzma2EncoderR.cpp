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

#include "common.h"
#include "Lzma2Encoder.h"

namespace Radyx {

Lzma2Encoder::init_ Lzma2Encoder::initializer_;
uint8_t Lzma2Encoder::distance_table[1 << kFastDistBits];
const uint8_t Lzma2Encoder::kLiteralNextStates[kNumStates] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5 };
const uint8_t Lzma2Encoder::kMatchNextStates[kNumStates] = { 7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10 };
const uint8_t Lzma2Encoder::kRepNextStates[kNumStates] = { 8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11 };
const uint8_t Lzma2Encoder::kShortRepNextStates[kNumStates] = { 9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11 };

Lzma2Encoder::Lzma2Encoder() NOEXCEPT
	: lc(3),
	lp(0),
	pb(2),
	fast_length(32),
	len_end_max(kOptimizerBufferSize - 1),
	lit_pos_mask((1 << lp) - 1),
	pos_mask((1 << pb) - 1),
	match_cycles(8),
	encoder_mode(Lzma2Options::kNormalMode),
	match_price_count(kDistanceRepriceFrequency),
	align_price_count(kAlignRepriceFrequency),
	dist_price_table_size(kDistTableSizeMax),
	needed_random_check(false)
{
}

void Lzma2Encoder::LengthStates::Reset(unsigned fast_length) NOEXCEPT
{
	choice = RangeEncoder::kProbInitValue;
	choice_2 = RangeEncoder::kProbInitValue;
	for (size_t i = 0; i < (kNumPositionStatesMax << kLenNumLowBits); ++i) {
		low[i] = RangeEncoder::kProbInitValue;
	}
	for (size_t i = 0; i < (kNumPositionStatesMax << kLenNumMidBits); ++i) {
		mid[i] = RangeEncoder::kProbInitValue;
	}
	for (size_t i = 0; i < kLenNumHighSymbols; ++i) {
		high[i] = RangeEncoder::kProbInitValue;
	}
	table_size = fast_length + 1 - kMatchLenMin;
}

void Lzma2Encoder::LengthStates::SetPrices(size_t pos_state) NOEXCEPT
{
	unsigned prob = choice;
	unsigned a0 = RangeEncoder::GetPrice0(prob);
	unsigned a1 = RangeEncoder::GetPrice1(prob);
	prob = choice_2;
	unsigned b0 = a1 + RangeEncoder::GetPrice0(prob);
	unsigned b1 = a1 + RangeEncoder::GetPrice1(prob);
	size_t i = 0;
	for (; i < kLenNumLowSymbols && i < table_size; ++i) {
		prices[pos_state][i] = a0 + RangeEncoder::GetTreePrice(low + (pos_state << kLenNumLowBits), kLenNumLowBits, i);
	}
	for (; i < kLenNumLowSymbols + kLenNumMidSymbols && i < table_size; ++i) {
		prices[pos_state][i] = b0 + RangeEncoder::GetTreePrice(mid + (pos_state << kLenNumMidBits), kLenNumMidBits, i - kLenNumLowSymbols);
	}
	for (; i < table_size; ++i) {
		prices[pos_state][i] = b1 + RangeEncoder::GetTreePrice(high, kLenNumHighBits, i - kLenNumLowSymbols - kLenNumMidSymbols);
	}
	counters[pos_state] = static_cast<unsigned>(table_size);
}

void Lzma2Encoder::EncoderStates::Reset(unsigned lc, unsigned lp, unsigned fast_length) NOEXCEPT
{
	state = 0;
	for (size_t i = 0; i < kNumReps; ++i) {
		reps[i] = 0;
	}
	for (size_t i = 0; i < kNumStates; ++i) {
		for (size_t j = 0; j < kNumPositionStatesMax; ++j) {
			is_match[i][j] = RangeEncoder::kProbInitValue;
			is_rep0_long[i][j] = RangeEncoder::kProbInitValue;
		}
		is_rep[i] = RangeEncoder::kProbInitValue;
		is_rep_G0[i] = RangeEncoder::kProbInitValue;
		is_rep_G1[i] = RangeEncoder::kProbInitValue;
		is_rep_G2[i] = RangeEncoder::kProbInitValue;
	}
	size_t num = static_cast<size_t>(kNumLiterals * kNumLitTables) << (lp + lc);
	for (size_t i = 0; i < num; ++i) {
		literal_probs[i] = RangeEncoder::kProbInitValue;
	}
	for (size_t i = 0; i < kNumLenToPosStates; ++i) {
		RangeEncoder::Probability *probs = dist_slot_encoders[i];
		for (size_t j = 0; j < (1 << kNumPosSlotBits); ++j) {
			probs[j] = RangeEncoder::kProbInitValue;
		}
	}
	for (size_t i = 0; i < kNumFullDistances - kEndPosModelIndex; ++i) {
		dist_encoders[i] = RangeEncoder::kProbInitValue;
	}
	len_states.Reset(fast_length);
	rep_len_states.Reset(fast_length);
	for (size_t i = 0; i < (1 << kNumAlignBits); ++i) {
		dist_align_encoders[i] = RangeEncoder::kProbInitValue;
	}
}

size_t Lzma2Encoder::GetUserDictionarySizeMax() NOEXCEPT
{
	bool b = sizeof(size_t) > 4;
	if (b) {
		uint64_t size = UINT64_C(1) << kDicLogSizeMax; // Avoid MSVC warning for Win32
		return static_cast<size_t>(size);
	}
	else {
		return k32BitDicSizeMax;
	}
}

size_t Lzma2Encoder::GetDictionarySizeMax() NOEXCEPT
{
	bool b = sizeof(size_t) > 4;
	if (b) {
		return (UINT64_C(1) << kDicLogSizeMax) - kNumReps + kMatchLenMin;
	}
	else {
		return k32BitDicSizeMax;
	}
}

size_t Lzma2Encoder::GetMemoryUsage(const Lzma2Options& options) NOEXCEPT
{
	return sizeof(Lzma2Encoder) +
		(options.encoder_mode != Lzma2Options::kFastMode) ? sizeof(OptimalNode) * kOptimizerBufferSize : 0;
}

uint8_t Lzma2Encoder::GetDictSizeProp(size_t dictionary_size) NOEXCEPT
{
	uint8_t dict_size_prop = 0;
	for (uint8_t bit = 11; bit < 32; ++bit) {
		if ((size_t(2) << bit) >= dictionary_size) {
			dict_size_prop = (bit - 11) << 1;
			break;
		}
		if ((size_t(3) << bit) >= dictionary_size) {
			dict_size_prop = ((bit - 11) << 1) | 1;
			break;
		}
	}
	return dict_size_prop;
}

void Lzma2Encoder::Reset(size_t max_distance) NOEXCEPT
{
	rc.Reset();
	encoder_states.Reset(lc, lp, fast_length);
	pos_mask = (1 << pb) - 1;
	lit_pos_mask = (1 << lp) - 1;
	uint_fast32_t i = 0;
	for (; max_distance > size_t(1) << i; ++i) {
	}
	dist_price_table_size = i * 2;
}

// ****************************************
// Distance slot functions based on fastpos.h from XZ

unsigned Lzma2Encoder::FastDistShift(unsigned n) NOEXCEPT
{
	return n * (kFastDistBits - 1);
}

unsigned Lzma2Encoder::FastDistResult(uint_fast32_t dist, unsigned n) NOEXCEPT
{
	return distance_table[dist >> FastDistShift(n)]
		+ 2 * FastDistShift(n);
}

size_t Lzma2Encoder::GetDistSlot(uint_fast32_t distance) NOEXCEPT
{
	uint_fast32_t limit = UINT32_C(1) << kFastDistBits;
	// If it is small enough, we can pick the result directly from
	// the precalculated table.
	if (distance < limit) {
		return distance_table[distance];
	}
	limit <<= FastDistShift(1);
	if (distance < limit) {
		return FastDistResult(distance, 1);
	}
	return FastDistResult(distance, 2);
}

// ****************************************

void Lzma2Encoder::UpdateLengthPrices(LengthStates &len_states) NOEXCEPT
{
	for (size_t pos_state = 0; pos_state <= pos_mask; ++pos_state) {
		len_states.SetPrices(pos_state);
	}
}

void Lzma2Encoder::FillAlignPrices() NOEXCEPT
{
	for (size_t i = 0; i < kAlignTableSize; ++i) {
		align_prices[i] = rc.GetReverseTreePrice(encoder_states.dist_align_encoders, kNumAlignBits, i);
	}
	align_price_count = 0;
}

void Lzma2Encoder::FillDistancesPrices() NOEXCEPT
{
	static const size_t kLastLenToPosState = kNumLenToPosStates - 1;
	for (size_t i = kStartPosModelIndex; i < kNumFullDistances; ++i) {
		size_t dist_slot = distance_table[i];
		unsigned footerBits = static_cast<unsigned>((dist_slot >> 1) - 1);
		size_t base = ((2 | (dist_slot & 1)) << footerBits);
		distance_prices[kLastLenToPosState][i] = rc.GetReverseTreePrice(encoder_states.dist_encoders + base - dist_slot - 1,
			footerBits,
			i - base);
	}
	for (size_t lenToPosState = 0; lenToPosState < kNumLenToPosStates; ++lenToPosState) {
		const RangeEncoder::Probability* encoder = encoder_states.dist_slot_encoders[lenToPosState];
		for (size_t dist_slot = 0; dist_slot < dist_price_table_size; ++dist_slot) {
			dist_slot_prices[lenToPosState][dist_slot] = rc.GetTreePrice(encoder, kNumPosSlotBits, dist_slot);
		}
		for (size_t dist_slot = kEndPosModelIndex; dist_slot < dist_price_table_size; ++dist_slot) {
			dist_slot_prices[lenToPosState][dist_slot] += rc.GetDirectBitsPrice(((dist_slot >> 1) - 1) - kNumAlignBits);
		}
		size_t i = 0;
		for (; i < kStartPosModelIndex; ++i) {
			distance_prices[lenToPosState][i] = dist_slot_prices[lenToPosState][i];
		}
		for (; i < kNumFullDistances; ++i) {
			distance_prices[lenToPosState][i] = dist_slot_prices[lenToPosState][distance_table[i]]
				+ distance_prices[kLastLenToPosState][i];
		}
	}
	match_price_count = 0;
}

unsigned Lzma2Encoder::GetLiteralPrice(size_t index, size_t state, unsigned prev_symbol, uint_fast32_t symbol, unsigned match_byte) const NOEXCEPT
{
	const RangeEncoder::Probability* prob_table = GetLiteralProbs(index, prev_symbol);
	if (IsCharState(state)) {
		unsigned price = 0;
		symbol |= 0x100;
		do {
			price += rc.GetPrice(prob_table[symbol >> 8], (symbol >> 7) & 1);
			symbol <<= 1;
		} while (symbol < 0x10000);
		return price;
	}
	return GetLiteralPriceMatched(prob_table, symbol, match_byte);
}

void Lzma2Encoder::EncodeLiteral(size_t index, uint_fast32_t symbol, unsigned prev_symbol) NOEXCEPT
{
	rc.EncodeBit0(encoder_states.is_match[encoder_states.state][index & pos_mask]);
	encoder_states.state = kLiteralNextStates[encoder_states.state];
	RangeEncoder::Probability* prob_table = GetLiteralProbs(index, prev_symbol);
	symbol |= 0x100;
	do {
		size_t prob_index = symbol >> 8;
		rc.EncodeBit(prob_table[prob_index], symbol & (1 << 7));
		symbol <<= 1;
	} while (symbol < 0x10000);
}

void Lzma2Encoder::EncodeLiteralMatched(const uint8_t* data_block, size_t index, uint_fast32_t symbol) NOEXCEPT
{
	rc.EncodeBit0(encoder_states.is_match[encoder_states.state][index & pos_mask]);
	encoder_states.state = kLiteralNextStates[encoder_states.state];
	unsigned match_symbol = data_block[index - encoder_states.reps[0] - 1];
	RangeEncoder::Probability* prob_table = GetLiteralProbs(index, data_block[index - 1]);
	unsigned offset = 0x100;
	symbol |= 0x100;
	do {
		match_symbol <<= 1;
		size_t prob_index = offset + (match_symbol & offset) + (symbol >> 8);
		rc.EncodeBit(prob_table[prob_index], symbol & (1 << 7));
		symbol <<= 1;
		offset &= ~(match_symbol ^ symbol);
	} while (symbol < 0x10000);
}

void Lzma2Encoder::EncodeLength(LengthStates& len_prob_table, unsigned len, size_t pos_state) NOEXCEPT
{
	len -= kMatchLenMin;
	if (len < kLenNumLowSymbols) {
		rc.EncodeBit0(len_prob_table.choice);
		rc.EncodeBitTree(len_prob_table.low + (pos_state << kLenNumLowBits), kLenNumLowBits, len);
	}
	else {
		rc.EncodeBit1(len_prob_table.choice);
		if (len < kLenNumLowSymbols + kLenNumMidSymbols) {
			rc.EncodeBit0(len_prob_table.choice_2);
			rc.EncodeBitTree(len_prob_table.mid + (pos_state << kLenNumMidBits), kLenNumMidBits, len - kLenNumLowSymbols);
		}
		else {
			rc.EncodeBit1(len_prob_table.choice_2);
			rc.EncodeBitTree(len_prob_table.high, kLenNumHighBits, len - kLenNumLowSymbols - kLenNumMidSymbols);
		}
	}
	if (encoder_mode != Lzma2Options::kFastMode && --len_prob_table.counters[pos_state] == 0) {
		len_prob_table.SetPrices(pos_state);
	}
}

void Lzma2Encoder::EncodeRepMatch(unsigned len, unsigned rep, size_t pos_state) NOEXCEPT
{
	rc.EncodeBit1(encoder_states.is_match[encoder_states.state][pos_state]);
	rc.EncodeBit1(encoder_states.is_rep[encoder_states.state]);
	if (rep == 0) {
		rc.EncodeBit0(encoder_states.is_rep_G0[encoder_states.state]);
		rc.EncodeBit(encoder_states.is_rep0_long[encoder_states.state][pos_state], ((len == 1) ? 0 : 1));
	}
	else {
		uint_fast32_t distance = encoder_states.reps[rep];
		rc.EncodeBit1(encoder_states.is_rep_G0[encoder_states.state]);
		if (rep == 1) {
			rc.EncodeBit0(encoder_states.is_rep_G1[encoder_states.state]);
		}
		else {
			rc.EncodeBit1(encoder_states.is_rep_G1[encoder_states.state]);
			rc.EncodeBit(encoder_states.is_rep_G2[encoder_states.state], rep - 2);
			if (rep == 3) {
				encoder_states.reps[3] = encoder_states.reps[2];
			}
			encoder_states.reps[2] = encoder_states.reps[1];
		}
		encoder_states.reps[1] = encoder_states.reps[0];
		encoder_states.reps[0] = distance;
	}
	if (len == 1) {
		encoder_states.state = kShortRepNextStates[encoder_states.state];
	}
	else {
		EncodeLength(encoder_states.rep_len_states, len, pos_state);
		encoder_states.state = kRepNextStates[encoder_states.state];
	}
}

void Lzma2Encoder::EncodeNormalMatch(unsigned len, uint_fast32_t dist, size_t pos_state) NOEXCEPT
{
	rc.EncodeBit1(encoder_states.is_match[encoder_states.state][pos_state]);
	rc.EncodeBit0(encoder_states.is_rep[encoder_states.state]);
	encoder_states.state = kMatchNextStates[encoder_states.state];
	EncodeLength(encoder_states.len_states, len, pos_state);
	size_t dist_slot = GetDistSlot(dist);
	rc.EncodeBitTree(encoder_states.dist_slot_encoders[GetLenToDistState(len)], kNumPosSlotBits, static_cast<unsigned>(dist_slot));
	if (dist_slot >= kStartPosModelIndex) {
		unsigned footerBits = (static_cast<unsigned>(dist_slot >> 1) - 1);
		size_t base = ((2 | (dist_slot & 1)) << footerBits);
		unsigned posReduced = static_cast<unsigned>(dist - base);
		if (dist_slot < kEndPosModelIndex) {
			rc.EncodeBitTreeReverse(encoder_states.dist_encoders + base - dist_slot - 1, footerBits, posReduced);
		}
		else {
			rc.EncodeDirect(posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
			rc.EncodeBitTreeReverse(encoder_states.dist_align_encoders, kNumAlignBits, posReduced & kAlignMask);
			++align_price_count;
		}
	}
	encoder_states.reps[3] = encoder_states.reps[2];
	encoder_states.reps[2] = encoder_states.reps[1];
	encoder_states.reps[1] = encoder_states.reps[0];
	encoder_states.reps[0] = dist;
	++match_price_count;
}

uint8_t Lzma2Encoder::GetLcLpPbCode() NOEXCEPT
{
	return static_cast<uint8_t>((pb * 5 + lp) * 9 + lc);
}

void Lzma2Encoder::InitDistanceTable() NOEXCEPT
{
	distance_table[0] = 0;
	distance_table[1] = 1;
	size_t c = 2;
	for (uint8_t slot = 2; slot < kFastDistBits * 2; ++slot) {
		size_t k = size_t(1) << ((slot >> 1) - 1);
		for (size_t j = 0; j < k; ++j, ++c) {
			distance_table[c] = slot;
		}
	}
}

// Test all available options at position 0 of the optimizer buffer.
// The prices at this point are all initialized to kInfinityPrice.
// This function must not be called at a position where no match is
// available.
size_t Lzma2Encoder::InitOptimizerPos0(const DataBlock& block,
	MatchResult match,
	size_t index,
	RepDistances& reps,
	OptimalArray& opt_buf) NOEXCEPT
{
	size_t max_length = std::min<size_t>(block.end - index, kMatchLenMax);
	const uint8_t *data = block.data + index;
	size_t rep_max_index = 0;
	size_t rep_lens[kNumReps];
	// Find any rep matches
	for (size_t i = 0; i < kNumReps; ++i) {
		reps[i] = encoder_states.reps[i];
		const uint8_t *data_2 = data - reps[i] - 1;
		if (!Compare2Bytes(data, data_2)) {
			rep_lens[i] = 0;
			continue;
		}
		rep_lens[i] = FindRepMatchLength(data, data_2, 2, max_length);
		if (rep_lens[i] > rep_lens[rep_max_index]) {
			rep_max_index = i;
		}
	}
	if (rep_lens[rep_max_index] >= fast_length) {
		opt_buf[0].prev_index = static_cast<unsigned>(rep_lens[rep_max_index]);
		opt_buf[0].prev_dist = static_cast<uint_fast32_t>(rep_max_index);
		return 0;
	}
	if (match.length >= fast_length) {
		opt_buf[0].prev_index = match.length;
		opt_buf[0].prev_dist = match.dist + kNumReps;
		return 0;
	}
	unsigned cur_byte = *data;
	unsigned match_byte = *(data - reps[0] - 1);
	size_t state = encoder_states.state;
	size_t pos_state = index & pos_mask;
	RangeEncoder::Probability is_match_prob = encoder_states.is_match[state][pos_state];
	RangeEncoder::Probability is_rep_prob = encoder_states.is_rep[state];
	opt_buf[0].state = state;
	// Set the price for literal
	opt_buf[1].price = rc.GetPrice0(is_match_prob) +
		GetLiteralPrice(index, state, data[-1], cur_byte, match_byte);
	opt_buf[1].MakeAsLiteral();
	unsigned match_price = rc.GetPrice1(is_match_prob);
	unsigned rep_match_price = match_price + rc.GetPrice1(is_rep_prob);
	if (match_byte == cur_byte) {
		// Try 1-byte rep0
		unsigned short_rep_price = rep_match_price + GetRepLen1Price(state, pos_state);
		if (short_rep_price < opt_buf[1].price) {
			opt_buf[1].price = short_rep_price;
			opt_buf[1].MakeAsShortRep();
		}
	}
	opt_buf[0].reps = reps;
	opt_buf[1].prev_index = 0;
	// Test the rep match prices
	for (size_t i = 0; i < kNumReps; ++i) {
		size_t rep_len = rep_lens[i];
		if (rep_len < 2) {
			continue;
		}
		unsigned price = rep_match_price + GetRepPrice(i, state, pos_state);
		// Test every available length of the rep
		do {
			unsigned cur_and_len_price = price + encoder_states.rep_len_states.prices[pos_state][rep_len - kMatchLenMin];
			if (cur_and_len_price < opt_buf[rep_len].price) {
				opt_buf[rep_len].price = cur_and_len_price;
				opt_buf[rep_len].prev_index = 0;
				opt_buf[rep_len].prev_dist = static_cast<uint_fast32_t>(i);
				opt_buf[rep_len].is_combination = false;
			}
		} while (--rep_len >= kMatchLenMin);
	}
	unsigned normal_match_price = match_price + rc.GetPrice0(is_rep_prob);
	size_t len = std::max<size_t>(rep_lens[0] + 1, 2);
	// Test the match prices
	if (hash_chain.get() == nullptr) {
		// Normal mode
		InitMatchesPos0(block, match, pos_state, len, normal_match_price, opt_buf);
		return std::max<size_t>(match.length, rep_lens[rep_max_index]);
	}
	else {
		// Hybrid mode
		size_t main_len = InitMatchesPos0Best(block, match, index, len, normal_match_price, opt_buf);
		return std::max(main_len, rep_lens[rep_max_index]);
	}
}

void Lzma2Encoder::InitMatchesPos0(const DataBlock& /*block*/,
	MatchResult match,
	size_t pos_state,
	size_t len,
	unsigned normal_match_price,
	OptimalArray& opt_buf) NOEXCEPT
{
	if (static_cast<unsigned>(len) <= match.length) {
		size_t distance = match.dist;
		size_t slot = GetDistSlot(match.dist);
		// Test every available length of the match
		do
		{
			unsigned cur_and_len_price = normal_match_price + encoder_states.len_states.prices[pos_state][len - kMatchLenMin];
			size_t len_to_dist_state = GetLenToDistState(len);
			if (distance < kNumFullDistances) {
				cur_and_len_price += distance_prices[len_to_dist_state][distance];
			}
			else {
				cur_and_len_price += align_prices[distance & kAlignMask] + dist_slot_prices[len_to_dist_state][slot];
			}
			if (cur_and_len_price < opt_buf[len].price) {
				opt_buf[len].price = cur_and_len_price;
				opt_buf[len].prev_index = 0;
				opt_buf[len].prev_dist = static_cast<uint_fast32_t>(distance + kNumReps);
				opt_buf[len].is_combination = false;
			}
			++len;
		} while (static_cast<unsigned>(len) <= match.length);
	}
}

size_t Lzma2Encoder::InitMatchesPos0Best(const DataBlock& block,
	MatchResult match,
	size_t index,
	size_t len,
	unsigned normal_match_price,
	OptimalArray& opt_buf) NOEXCEPT
{
	if (len <= match.length) {
		size_t main_len = hash_chain->GetMatches(block, index, match_cycles, match, matches);
		size_t match_index = 0;
		while (len > matches[match_index].length) {
			++match_index;
		}
		size_t distance = matches[match_index].dist;
		size_t slot = GetDistSlot(matches[match_index].dist);
		// Test every available match length at the shortest distance. The buffer is sorted
		// in order of increasing length, and therefore increasing distance too.
		for (;; ++len) {
			unsigned cur_and_len_price = normal_match_price
				+ encoder_states.len_states.prices[index & pos_mask][len - kMatchLenMin];
			size_t len_to_dist_state = GetLenToDistState(len);
			if (distance < kNumFullDistances) {
				cur_and_len_price += distance_prices[len_to_dist_state][distance];
			}
			else {
				cur_and_len_price += align_prices[distance & kAlignMask] + dist_slot_prices[len_to_dist_state][slot];
			}
			if (cur_and_len_price < opt_buf[len].price) {
				opt_buf[len].price = cur_and_len_price;
				opt_buf[len].prev_index = 0;
				opt_buf[len].prev_dist = static_cast<uint_fast32_t>(distance + kNumReps);
				opt_buf[len].is_combination = false;
			}
			if (len == matches[match_index].length) {
				// Run out of length for this match. Get the next if any.
				if (len == main_len) {
					break;
				}
				++match_index;
				distance = matches[match_index].dist;
				slot = GetDistSlot(matches[match_index].dist);
			}
		}
		return main_len;
	}
	return 0;
}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#else
__pragma(warning(disable:4701))
#endif

// The speed of this method is critical and the sections have so many variables
// in common that breaking it up would be inefficient, so it remains a monolith.
// For each position cur, starting at 1, this is called to check all possible
// encoding choices - a literal, 1-byte rep 0 match, all rep match lengths, and
// all match lengths at available distances. It also checks the combined
// sequences literal+rep0, rep+rep0 and match+rep0.
// If the encoder has a hash chain, this method works in hybrid mode, using the
// hash chain to find shorter matches at near distances.
size_t Lzma2Encoder::OptimalParse(const DataBlock& block,
	MatchResult match,
	size_t index,
	size_t cur,
	size_t len_end,
	RepDistances& reps,
	OptimalArray& opt_buf) NOEXCEPT
{
	OptimalNode& cur_opt = opt_buf[cur];
	size_t prev_index = cur_opt.prev_index;
	size_t state = opt_buf[prev_index].state;
	if (cur_opt.is_combination) {
		--prev_index;
		if (cur_opt.prev_2) {
			state = opt_buf[cur_opt.prev_index_2].state;
			if (cur_opt.prev_dist_2 < kNumReps) {
				state = kRepNextStates[state];
			}
			else {
				state = kMatchNextStates[state];
			}
		}
		else {
			state = opt_buf[prev_index].state;
		}
		state = kLiteralNextStates[state];
	}
	if (prev_index == cur - 1) {
		if (cur_opt.IsShortRep()) {
			state = kShortRepNextStates[state];
		}
		else {
			state = kLiteralNextStates[state];
		}
	}
	else {
		size_t dist;
		if (cur_opt.is_combination && cur_opt.prev_2) {
			prev_index = cur_opt.prev_index_2;
			dist = cur_opt.prev_dist_2;
			state = kRepNextStates[state];
		}
		else {
			dist = cur_opt.prev_dist;
			if (dist < kNumReps) {
				state = kRepNextStates[state];
			}
			else {
				state = kMatchNextStates[state];
			}
		}
		const OptimalNode& prev_opt = opt_buf[prev_index];
		if (dist < kNumReps) {
			reps[0] = prev_opt.reps[dist];
			size_t i = 1;
			for (; i <= dist; ++i) {
				reps[i] = prev_opt.reps[i - 1];
			}
			for (; i < kNumReps; ++i) {
				reps[i] = prev_opt.reps[i];
			}
		}
		else {
			reps[0] = static_cast<uint_fast32_t>(dist - kNumReps);
			for (size_t i = 1; i < kNumReps; ++i) {
				reps[i] = prev_opt.reps[i - 1];
			}
		}
	}
	const uint8_t* data = block.data + index;
	unsigned cur_byte = *data;
	unsigned match_byte = *(data - reps[0] - 1);
	size_t pos_state = (index & pos_mask);
	RangeEncoder::Probability is_match_prob = encoder_states.is_match[state][pos_state];
	RangeEncoder::Probability is_rep_prob = encoder_states.is_rep[state];
	cur_opt.state = state;
	cur_opt.reps = reps;
	uint_fast32_t cur_price = cur_opt.price;
	uint_fast32_t cur_and_lit_price = cur_price + rc.GetPrice0(is_match_prob) +
		GetLiteralPrice(index, state, data[-1], cur_byte, match_byte);
	OptimalNode& next_opt = opt_buf[cur + 1];
	bool next_is_char = false;
	// Try literal
	if (cur_and_lit_price < next_opt.price) {
		next_opt.price = cur_and_lit_price;
		next_opt.prev_index = static_cast<unsigned>(cur);
		next_opt.MakeAsLiteral();
		next_is_char = true;
	}
	uint_fast32_t match_price = cur_price + rc.GetPrice1(is_match_prob);
	uint_fast32_t rep_match_price = match_price + rc.GetPrice1(is_rep_prob);
	if (match_byte == cur_byte) {
		// Try 1-byte rep0
		uint_fast32_t short_rep_price = rep_match_price + GetRepLen1Price(state, pos_state);
		if (short_rep_price <= next_opt.price) {
			next_opt.price = short_rep_price;
			next_opt.prev_index = static_cast<unsigned>(cur);
			next_opt.MakeAsShortRep();
			next_is_char = true;
		}
	}
	size_t bytes_avail = std::min(block.end - index, opt_buf.size() - 1 - cur);
	if (bytes_avail < 2) {
		return len_end;
	}
	if (!next_is_char && match_byte != cur_byte) {
		// Try literal + rep0
		const uint8_t *data_2 = data - reps[0];
		size_t limit = std::min<size_t>(bytes_avail - 1, fast_length);
		size_t len_test_2 = FindRepMatchLength(data + 1, data_2, 0, limit);
		if (len_test_2 >= 2) {
			size_t state_2 = kLiteralNextStates[state];
			size_t pos_state_next = (index + 1) & pos_mask;
			uint_fast32_t next_rep_match_price = cur_and_lit_price +
				rc.GetPrice1(encoder_states.is_match[state_2][pos_state_next]) +
				rc.GetPrice1(encoder_states.is_rep[state_2]);
			size_t offset = cur + 1 + len_test_2;
			uint_fast32_t cur_and_len_price = next_rep_match_price + GetRepMatch0Price(len_test_2, state_2, pos_state_next);
			if (cur_and_len_price < opt_buf[offset].price) {
				len_end = std::max(len_end, offset);
				opt_buf[offset].price = cur_and_len_price;
				opt_buf[offset].prev_index = static_cast<unsigned>(cur + 1);
				opt_buf[offset].prev_dist = 0;
				opt_buf[offset].is_combination = true;
				opt_buf[offset].prev_2 = false;
			}
		}
	}
	size_t max_length = std::min<size_t>(bytes_avail, fast_length);
	size_t start_len = 2;
	if (match.length > 0) {
		for (size_t rep_index = 0; rep_index < kNumReps; ++rep_index) {
			const uint8_t *data_2 = data - reps[rep_index] - 1;
			if (!Compare2Bytes(data, data_2)) {
				continue;
			}
			size_t len_test = FindRepMatchLength(data, data_2, 2, max_length);
			len_end = std::max(len_end, cur + len_test);
			uint_fast32_t cur_rep_price = rep_match_price + GetRepPrice(rep_index, state, pos_state);
			size_t len = 2;
			// Try rep match
			do {
				uint_fast32_t cur_and_len_price = cur_rep_price + encoder_states.rep_len_states.prices[pos_state][len - kMatchLenMin];
				OptimalNode& opt = opt_buf[cur + len];
				if (cur_and_len_price < opt.price) {
					opt.price = cur_and_len_price;
					opt.prev_index = static_cast<unsigned>(cur);
					opt.prev_dist = static_cast<uint_fast32_t>(rep_index);
					opt.is_combination = false;
				}
			} while (++len <= len_test);

			if (rep_index == 0) {
				// Save time by exluding normal matches not longer than the rep
				start_len = len_test + 1;
			}
			size_t len_test_2 = FindRepMatchLength(data,
				data_2,
				len_test + 1,
				std::min(len_test + 1 + fast_length, bytes_avail)) - len_test - 1;
			if (len_test_2 >= 2) {
				// Try rep + literal + rep0
				size_t state_2 = kRepNextStates[state];
				size_t pos_state_next = (index + len_test) & pos_mask;
				uint_fast32_t rep_lit_rep_total_price =
					cur_rep_price + encoder_states.rep_len_states.prices[pos_state][len_test - kMatchLenMin] +
					rc.GetPrice0(encoder_states.is_match[state_2][pos_state_next]) +
					GetLiteralPriceMatched(GetLiteralProbs(index + len_test, data[len_test - 1]),
					data[len_test], data_2[len_test]);
				state_2 = kLiteralNextStates[state_2];
				pos_state_next = (index + len_test + 1) & pos_mask;
				rep_lit_rep_total_price +=
					rc.GetPrice1(encoder_states.is_match[state_2][pos_state_next]) +
					rc.GetPrice1(encoder_states.is_rep[state_2]);
				size_t offset = cur + len_test + 1 + len_test_2;
				rep_lit_rep_total_price += GetRepMatch0Price(len_test_2, state_2, pos_state_next);
				if (rep_lit_rep_total_price < opt_buf[offset].price) {
					len_end = std::max(len_end, offset);
					opt_buf[offset].price = rep_lit_rep_total_price;
					opt_buf[offset].prev_index = static_cast<unsigned>(cur + len_test + 1);
					opt_buf[offset].prev_dist = 0;
					opt_buf[offset].is_combination = true;
					opt_buf[offset].prev_2 = true;
					opt_buf[offset].prev_index_2 = static_cast<unsigned>(cur);
					opt_buf[offset].prev_dist_2 = static_cast<uint_fast32_t>(rep_index);
				}
			}
		}
	}
	if (match.length >= start_len && max_length >= start_len) {
		// Try normal match
		uint_fast32_t normal_match_price = match_price + rc.GetPrice0(is_rep_prob);
		if (hash_chain.get() == nullptr) {
			// Normal mode - single match
			size_t length = std::min<size_t>(match.length, max_length);
			len_end = std::max(len_end, cur + length);
			size_t cur_dist = match.dist;
			size_t dist_slot = GetDistSlot(match.dist);
			uint_fast32_t cur_and_len_price;
			size_t len_test = std::max<size_t>(start_len, length - (length >> 1));
			// Pre-load rep0 data bytes
			unsigned rep_0_bytes = Get2Bytes(data - cur_dist + length);
			for (; len_test <= length; ++len_test) {
				cur_and_len_price = normal_match_price + encoder_states.len_states.prices[pos_state][len_test - kMatchLenMin];
				size_t len_to_dist_state = GetLenToDistState(len_test);
				if (cur_dist < kNumFullDistances) {
					cur_and_len_price += distance_prices[len_to_dist_state][cur_dist];
				}
				else {
					cur_and_len_price += dist_slot_prices[len_to_dist_state][dist_slot] + align_prices[cur_dist & kAlignMask];
				}
				OptimalNode& opt = opt_buf[cur + len_test];
				if (cur_and_len_price < opt.price) {
					opt.price = cur_and_len_price;
					opt.prev_index = static_cast<unsigned>(cur);
					opt.prev_dist = static_cast<uint_fast32_t>(cur_dist + kNumReps);
					opt.is_combination = false;
				}
			}
			if (rep_0_bytes == Get2Bytes(data + len_test) && len_test + 2 <= bytes_avail) {
				// Try match + literal + rep0
				const uint8_t *data_2 = data - cur_dist - 1;
				size_t limit = std::min(len_test + fast_length, bytes_avail);
				size_t len_test_2 = FindRepMatchLength(data, data_2, len_test + 2, limit) - len_test;
				size_t state_2 = kMatchNextStates[state];
				size_t pos_state_next = (index + length) & pos_mask;
				uint_fast32_t match_lit_rep_total_price = cur_and_len_price +
					rc.GetPrice0(encoder_states.is_match[state_2][pos_state_next]) +
					GetLiteralPriceMatched(GetLiteralProbs(index + length, data[len_test - 2]),
					data[len_test - 1], data_2[len_test - 1]);
				state_2 = kLiteralNextStates[state_2];
				pos_state_next = (pos_state_next + 1) & pos_mask;
				match_lit_rep_total_price +=
					rc.GetPrice1(encoder_states.is_match[state_2][pos_state_next]) +
					rc.GetPrice1(encoder_states.is_rep[state_2]);
				size_t offset = cur + len_test + len_test_2;
				match_lit_rep_total_price += GetRepMatch0Price(len_test_2, state_2, pos_state_next);
				if (match_lit_rep_total_price < opt_buf[offset].price) {
					len_end = std::max(len_end, offset);
					opt_buf[offset].price = match_lit_rep_total_price;
					opt_buf[offset].prev_index = static_cast<unsigned>(cur + len_test);
					opt_buf[offset].prev_dist = 0;
					opt_buf[offset].is_combination = true;
					opt_buf[offset].prev_2 = true;
					opt_buf[offset].prev_index_2 = static_cast<unsigned>(cur);
					opt_buf[offset].prev_dist_2 = static_cast<uint_fast32_t>(cur_dist + kNumReps);
				}
			}
		}
		else {
			// Hybrid mode
			size_t main_len = hash_chain->GetMatches(block, index, match_cycles, match, matches);
			main_len = std::min(main_len, max_length);
			len_end = std::max(len_end, cur + main_len);
			size_t match_index = 0;
			while (static_cast<unsigned>(main_len) > matches[match_index].length) {
				++match_index;
			}
			matches[match_index].length = static_cast<unsigned>(main_len);
			match_index = 0;
			while (start_len > matches[match_index].length) {
				++match_index;
			}
			size_t cur_dist = matches[match_index].dist;
			size_t dist_slot = GetDistSlot(matches[match_index].dist);
			uint_fast32_t cur_and_len_price;
			size_t len_test = start_len;
			unsigned rep_0_bytes = Get2Bytes(data - cur_dist + matches[match_index].length);
			for (;; ++len_test) {
				cur_and_len_price = normal_match_price + encoder_states.len_states.prices[pos_state][len_test - kMatchLenMin];
				size_t len_to_dist_state = GetLenToDistState(len_test);
				if (cur_dist < kNumFullDistances) {
					cur_and_len_price += distance_prices[len_to_dist_state][cur_dist];
				}
				else {
					cur_and_len_price += dist_slot_prices[len_to_dist_state][dist_slot] + align_prices[cur_dist & kAlignMask];
				}
				OptimalNode& opt = opt_buf[cur + len_test];
				if (cur_and_len_price < opt.price) {
					opt.price = cur_and_len_price;
					opt.prev_index = static_cast<unsigned>(cur);
					opt.prev_dist = static_cast<uint_fast32_t>(cur_dist + kNumReps);
					opt.is_combination = false;
				}
				if (len_test == matches[match_index].length) {
					size_t rep_0_pos = len_test + 1;
					if (rep_0_bytes == Get2Bytes(data + rep_0_pos) && rep_0_pos + 2 <= bytes_avail) {
						// Try match + literal + rep0
						const uint8_t *data_2 = data - cur_dist - 1;
						size_t limit = std::min(rep_0_pos + fast_length, bytes_avail);
						size_t len_test_2 = FindRepMatchLength(data, data_2, rep_0_pos + 2, limit) - rep_0_pos;
						size_t state_2 = kMatchNextStates[state];
						size_t pos_state_next = (index + len_test) & pos_mask;
						uint_fast32_t match_lit_rep_total_price = cur_and_len_price +
							rc.GetPrice0(encoder_states.is_match[state_2][pos_state_next]) +
							GetLiteralPriceMatched(GetLiteralProbs(index + len_test, data[len_test - 1]),
							data[len_test], data_2[len_test]);
						state_2 = kLiteralNextStates[state_2];
						pos_state_next = (pos_state_next + 1) & pos_mask;
						match_lit_rep_total_price +=
							rc.GetPrice1(encoder_states.is_match[state_2][pos_state_next]) +
							rc.GetPrice1(encoder_states.is_rep[state_2]);
						size_t offset = cur + rep_0_pos + len_test_2;
						match_lit_rep_total_price += GetRepMatch0Price(len_test_2, state_2, pos_state_next);
						if (match_lit_rep_total_price < opt_buf[offset].price) {
							len_end = std::max(len_end, offset);
							opt_buf[offset].price = match_lit_rep_total_price;
							opt_buf[offset].prev_index = static_cast<unsigned>(cur + rep_0_pos);
							opt_buf[offset].prev_dist = 0;
							opt_buf[offset].is_combination = true;
							opt_buf[offset].prev_2 = true;
							opt_buf[offset].prev_index_2 = static_cast<unsigned>(cur);
							opt_buf[offset].prev_dist_2 = static_cast<uint_fast32_t>(cur_dist + kNumReps);
						}
					}
					if (len_test == main_len) {
						break;
					}
					++match_index;
					cur_dist = matches[match_index].dist;
					dist_slot = GetDistSlot(matches[match_index].dist);
					rep_0_bytes = Get2Bytes(data - cur_dist + matches[match_index].length);
				}
			}
		}
	}
	return len_end;
}

}