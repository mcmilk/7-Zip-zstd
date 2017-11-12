#include "common.h"
#include "LzmaStates.h"

namespace Radyx {

void LzmaStates::LengthStates::Reset(unsigned fast_length) NOEXCEPT
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

void LzmaStates::LengthStates::SetPrices(size_t pos_state) NOEXCEPT
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

void LzmaStates::Reset(unsigned lc, unsigned lp, unsigned fast_length) NOEXCEPT
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

}