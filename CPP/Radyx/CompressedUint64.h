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

#ifndef RADYX_COMPRESSED_UINT64_H
#define RADYX_COMPRESSED_UINT64_H

namespace Radyx {

class CompressedUint64
{
public:
	CompressedUint64(uint_least64_t u) noexcept;
	CompressedUint64(const uint8_t* in_buffer, size_t byte_count) noexcept;
	void operator=(uint_least64_t u) noexcept;
	void operator=(const CompressedUint64& right) noexcept;
	operator uint_least64_t() const noexcept;
	const uint8_t* GetValue() const noexcept {
		return value;
	}
	unsigned GetSize() const noexcept {
		return size;
	}
	bool CheckEof() const noexcept {
		return size == 0;
	}

private:
	uint8_t value[9];
	unsigned size;
};

}

#endif