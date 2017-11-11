///////////////////////////////////////////////////////////////////////////////
//
// Class:   CompressedUint64
//          Contains a uint64_t in the 7-zip compressed storage format
//          
// Authors: Igor Pavlov
//          Conor McCarthy
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

#include <cstring>
#include "common.h"
#include "CompressedUint64.h"

namespace Radyx {

CompressedUint64::CompressedUint64(uint_least64_t u) noexcept
{
	*this = u;
}

CompressedUint64::CompressedUint64(const uint8_t* in_buffer, size_t byte_count) noexcept
{
	uint8_t mask = 0x80;
	size = 1;
	for (; size < 9 && size <= byte_count; ++size) {
		if ((in_buffer[0] & mask) == 0) {
			break;
		}
		mask >>= 1;
	}
	if (size > byte_count) {
		size = 0;
	}
	else {
		memcpy(value, in_buffer, size);
	}
}

void CompressedUint64::operator=(uint_least64_t u) noexcept
{
	uint8_t first_byte = 0;
	uint8_t mask = 0x80;
	size = 0;
	int i = 0;
	for (; i < 8; ++i) {
		if (u < ((UINT64_C(1) << (7 * (i + 1))))) {
			first_byte |= static_cast<uint8_t>(u >> (8 * i));
			break;
		}
		first_byte |= mask;
		mask >>= 1;
	}
	value[size++] = first_byte;
	for (; i > 0; i--) {
		value[size++] = static_cast<uint8_t>(u);
		u >>= 8;
	}
}

void CompressedUint64::operator=(const CompressedUint64& right) noexcept
{
	if (&right != this) {
		size = right.size;
		memcpy(value, right.value, size);
	}
}

CompressedUint64::operator uint_least64_t() const noexcept
{
	uint_least64_t u = 0;
	for (unsigned i = 1; i < size; ++i) {
		u |= (static_cast<uint_least64_t>(value[i]) << (8 * (i - 1)));
	}
	u += static_cast<uint_least64_t>(value[0] & ((1 << (9 - size)) - 1)) << ((size - 1) * 8);
	return u;
}

}