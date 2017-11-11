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

#ifndef RADYX_CRC32_H
#define RADYX_CRC32_H

namespace Radyx {

class Crc32
{
public:
	Crc32() noexcept : crc32(0xFFFFFFFF) {}
	inline void Add(uint8_t byte) noexcept;
	inline void Add(uint8_t* buffer, size_t count) noexcept;
	operator uint_fast32_t() const noexcept {
		return crc32 ^ 0xFFFFFFFF;
	}
	static uint_fast32_t GetHash(uint8_t byte) noexcept {
		return crc_table[byte];
	}

private:
	static void InitCrcTable() noexcept;

	static uint_fast32_t crc_table[256];
	uint_fast32_t crc32;

	static class init_
	{
	public:
		init_() noexcept { InitCrcTable(); }
	} initializer_;
};

void Crc32::Add(uint8_t byte) noexcept
{
	crc32 = crc_table[(crc32 ^ byte) & 0xFF] ^ (crc32 >> 8);
}

void Crc32::Add(uint8_t* buffer, size_t count) noexcept
{
	for (size_t i = 0; i < count; ++i) {
		Add(buffer[i]);
	}
}

}

#endif // RADYX_CRC32_H