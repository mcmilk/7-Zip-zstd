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

#include "common.h"
#include "Lzma2Options.h"

namespace Radyx {

const uint8_t Lzma2Options::kDicSizeTable[10] = { 18, 18, 20, 22, 24, 25, 25, 26, 26, 27 };

void Lzma2Options::LoadCompressLevel() NOEXCEPT
{
	if (!dictionary_size.IsSet()) {
		dictionary_size = size_t(1) << kDicSizeTable[compress_level];
	}
	if (!fast_length.IsSet()) {
		fast_length = (compress_level > 5) ? 64 : 32;
	}
	if (!encoder_mode.IsSet()) {
		if (compress_level < 5) {
			encoder_mode = kFastMode;
		}
		else if (compress_level >= 7) {
			encoder_mode = kBestMode;
		}
	}
	if (compress_level >= 7) {
		if (!match_cycles.IsSet()) {
			match_cycles = 8 + (compress_level - 7) * 4;
		}
		if (!second_dict_bits.IsSet()) {
			second_dict_bits = compress_level * 2 - 2;
		}
	}
}

}