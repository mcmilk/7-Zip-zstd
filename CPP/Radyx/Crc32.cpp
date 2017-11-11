///////////////////////////////////////////////////////////////////////////////
//
// Class:   Crc32
//          Calculate CRC-32 values
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
#include "Crc32.h"

namespace Radyx {

Crc32::init_ Crc32::initializer_;
uint_fast32_t Crc32::crc_table[256];

void Crc32::InitCrcTable() noexcept
{
	for (uint_fast32_t i = 0; i < 256; ++i)
	{
		uint_fast32_t crc32 = i;
		for (uint8_t c = 8; c; c--) {
			crc32 = (crc32 & 1) ? UINT32_C(0xEDB88320) ^ (crc32 >> 1) : crc32 >> 1;
		}
		crc_table[i] = crc32;
	}
}

}