///////////////////////////////////////////////////////////////////////////////
//
// Class: Lzma2Options
//        Options for LZMA2 encoding
//
// Copyright 2015 Conor McCarthy
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

#ifndef RADYX_LZMA2_OPTIONS_H
#define RADYX_LZMA2_OPTIONS_H

#include "OptionalSetting.h"

namespace Radyx {

struct Lzma2Options
{
	static const uint8_t kDicSizeTable[10];
	static const unsigned kOverlapShift = 4;

	enum Mode
	{
		kFastMode,
		kNormalMode,
		kBestMode
	};

	unsigned lc;
	unsigned lp;
	unsigned pb;
	OptionalSetting<unsigned> fast_length;
	OptionalSetting<unsigned> match_cycles;
	OptionalSetting<Mode> encoder_mode;
	unsigned compress_level;
	OptionalSetting<unsigned> second_dict_bits;
	OptionalSetting<size_t> dictionary_size;
	OptionalSetting<size_t> match_buffer_size;
	unsigned block_overlap;
	unsigned random_filter;
	Lzma2Options() NOEXCEPT
		: lc(3),
		lp(0),
		pb(2),
		fast_length(32),
		match_cycles(8),
		encoder_mode(kNormalMode),
		compress_level(5),
		second_dict_bits(12),
		dictionary_size(UINT32_C(32) << 20),
		match_buffer_size(0),
		block_overlap(2),
		random_filter(0) {}
	void LoadCompressLevel() NOEXCEPT;
};

}

#endif // RADYX_LZMA2_OPTIONS_H