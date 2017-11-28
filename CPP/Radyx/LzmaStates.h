#ifndef RADYX_LZMA_STATES_H
#define RADYX_LZMA_STATES_H

#include "RangeEncoder.h"
#include "RepDistances.h"

namespace Radyx {

struct LzmaStates
{
	static const unsigned kNumReps = 4;
	static const unsigned kNumStates = 12;
	static const uint8_t kLiteralNextStates[kNumStates];
	static const uint8_t kMatchNextStates[kNumStates];
	static const uint8_t kRepNextStates[kNumStates];
	static const uint8_t kShortRepNextStates[kNumStates];
	static const unsigned kNumPositionBitsMax = 4;
	static const unsigned kNumPositionStatesMax = 1 << kNumPositionBitsMax;
	static const unsigned kNumLenToPosStates = 4;
	static const unsigned kNumPosSlotBits = 6;
	static const unsigned kNumAlignBits = 4;
	static const unsigned kStartPosModelIndex = 4;
	static const unsigned kEndPosModelIndex = 14;
	static const unsigned kNumFullDistancesBits = kEndPosModelIndex >> 1;
	static const unsigned kNumFullDistances = 1 << kNumFullDistancesBits;
	static const unsigned kNumLiterals = 0x100;
	static const unsigned kNumLitTables = 3;
	static const unsigned kLcLpMax = 4;

	struct LengthStates
	{
		static const unsigned kLenNumLowBits = 3;
		static const unsigned kLenNumLowSymbols = 1 << kLenNumLowBits;
		static const unsigned kLenNumMidBits = 3;
		static const unsigned kLenNumMidSymbols = 1 << kLenNumMidBits;
		static const unsigned kLenNumHighBits = 8;
		static const unsigned kLenNumHighSymbols = 1 << kLenNumHighBits;
		static const unsigned kLenNumSymbolsTotal = kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols;
		static const unsigned kMatchLenMin = 2;
		static const unsigned kMatchLenMax = kMatchLenMin + kLenNumSymbolsTotal - 1;

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

	typedef RepDistances<kNumReps> LzmaRepDistances;

	LzmaRepDistances reps;
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

}

#endif // RADYX_LZMA_STATES_H