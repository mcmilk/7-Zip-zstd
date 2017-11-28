///////////////////////////////////////////////////////////////////////////////
//
// Class:   FlagTable
//          Fast-reset table of boolean values which doesn't require a
//          zero-fill to set all values to false.
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

#ifndef RADYX_FLAG_TABLE_H
#define RADYX_FLAG_TABLE_H

#include <array>

namespace Radyx {

template<class T, int kBits>
class FlagTable
{
public:
	FlagTable() NOEXCEPT;
	bool IsSet(size_t index) const NOEXCEPT {
		return flag_table[index] == flag_value;
	}
	void Set(size_t index) NOEXCEPT {
		flag_table[index] = flag_value;
	}
	inline void Reset() NOEXCEPT;

private:
	void InitTable() NOEXCEPT;

	std::array<T, UINT32_C(1) << kBits> flag_table;
	T flag_value;

	FlagTable(const FlagTable&) = delete;
	FlagTable& operator=(const FlagTable&) = delete;
	FlagTable(FlagTable&&) = delete;
	FlagTable& operator=(FlagTable&&) = delete;
};

template<class T, int kBits>
FlagTable<T, kBits>::FlagTable() NOEXCEPT : flag_value(0)
{
	static_assert(T(T(0) - 1) > 0, "FlagTable must be used with unsigned types only.");
	InitTable();
}

template<class T, int kBits>
inline void FlagTable<T, kBits>::Reset() NOEXCEPT
{
	// Reset all bool values to false by incrementing the flag value
	if (++flag_value == 0) {
		// Value wrapped so set it to 1 and fill the table with 0
		++flag_value;
		InitTable();
	}
}

template<class T, int kBits>
void FlagTable<T, kBits>::InitTable() NOEXCEPT
{
	flag_table.fill(0);
}

}

#endif // RADYX_FLAG_TABLE_H