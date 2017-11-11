///////////////////////////////////////////////////////////////////////////////
//
// Class: StructuredMatchTable
//        Match table with links and lengths stored in a structure
//        to improve cacheability
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

#ifndef RADYX_STRUCTURED_MATCH_TABLE_H
#define RADYX_STRUCTURED_MATCH_TABLE_H

#include <cinttypes>
#include <memory>
#include <algorithm>

namespace Radyx {

class StructuredMatchTable
{
private:
	static const UintFast32 kNullLink = MatchTableBuilder::kNullLink;
	static const unsigned kUnitBits = 2;
	static const size_t kUnitMask = (1 << kUnitBits) - 1;

	struct MatchUnit
	{
		UintFast32 links[1 << kUnitBits];
		uint8_t lengths[1 << kUnitBits];
	};

public:
	static const UintFast32 kMaxLength = 255;

	inline StructuredMatchTable(size_t dictionary_size);
	inline void InitMatchLink(size_t index, UintFast32 link) noexcept;
	inline UintFast32 GetInitialMatchLink(size_t index) const noexcept;
	inline UintFast32 GetMatchLink(size_t index) const noexcept;
	inline UintFast32 GetMatchLength(size_t index) const noexcept;
	inline UintFast32 GetMatchLinkAndLength(size_t index, uint8_t& length) const noexcept;
	inline UintFast32 GetMatchLinkAndLength(size_t index, unsigned& length) const noexcept;
	inline void SetMatchLink(size_t index, UintFast32 link, UintFast32 length) noexcept;
	inline void SetMatchLength(size_t index, UintFast32 link, UintFast32 length) noexcept;
	inline void SetMatchLinkAndLength(size_t index, UintFast32 link, uint8_t length) noexcept;
	inline void SetMatchLinkAndLength(size_t index, UintFast32 link, UintFast32 length) noexcept;
	inline void RestrictMatchLength(size_t index, UintFast32 length) noexcept;
	inline void SetNull(size_t index) noexcept;
	inline bool HaveMatch(size_t index) const noexcept;
	inline MatchUnit* GetBuffer(size_t index) noexcept;
	inline size_t CalcMatchBufferSize(size_t block_size, unsigned extra_thread_count) const noexcept;
	static inline size_t GetMemoryUsage(size_t dictionary_size) noexcept;

private:
	std::unique_ptr<MatchUnit[]> match_table;

	StructuredMatchTable(const StructuredMatchTable&) = delete;
	StructuredMatchTable& operator=(const StructuredMatchTable&) = delete;
	StructuredMatchTable(StructuredMatchTable&&) = delete;
	StructuredMatchTable& operator=(StructuredMatchTable&&) = delete;
};

StructuredMatchTable::StructuredMatchTable(size_t dictionary_size)
	: match_table(new MatchUnit[(dictionary_size >> kUnitBits) + 1])
{
}

void StructuredMatchTable::InitMatchLink(size_t index, UintFast32 link) noexcept
{
	match_table[index >> kUnitBits].links[index & kUnitMask] = link;
}

UintFast32 StructuredMatchTable::GetInitialMatchLink(size_t index) const noexcept
{
	return match_table[index >> kUnitBits].links[index & kUnitMask];
}

UintFast32 StructuredMatchTable::GetMatchLink(size_t index) const noexcept
{
	return match_table[index >> kUnitBits].links[index & kUnitMask];
}

UintFast32 StructuredMatchTable::GetMatchLength(size_t index) const noexcept
{
	return match_table[index >> kUnitBits].lengths[index & kUnitMask];
}

void StructuredMatchTable::SetMatchLink(size_t index, UintFast32 link, UintFast32 /*length*/) noexcept
{
	match_table[index >> kUnitBits].links[index & kUnitMask] = link;
}

void StructuredMatchTable::SetMatchLength(size_t index, UintFast32 /*link*/, UintFast32 length) noexcept
{
	match_table[index >> kUnitBits].lengths[index & kUnitMask] = static_cast<uint8_t>(length);
}

void StructuredMatchTable::SetMatchLinkAndLength(size_t index, UintFast32 link, uint8_t length) noexcept
{
	size_t slot = index & kUnitMask;
	index >>= kUnitBits;
	match_table[index].links[slot] = link;
	match_table[index].lengths[slot] = length;
}

void StructuredMatchTable::SetMatchLinkAndLength(size_t index, UintFast32 link, UintFast32 length) noexcept
{
	size_t slot = index & kUnitMask;
	index >>= kUnitBits;
	match_table[index].links[slot] = link;
	match_table[index].lengths[slot] = static_cast<uint8_t>(length);
}

UintFast32 StructuredMatchTable::GetMatchLinkAndLength(size_t index, uint8_t& length) const noexcept
{
	size_t slot = index & kUnitMask;
	index >>= kUnitBits;
	length = match_table[index].lengths[slot];
	return match_table[index].links[slot];
}

UintFast32 StructuredMatchTable::GetMatchLinkAndLength(size_t index, unsigned& length) const noexcept
{
	size_t slot = index & kUnitMask;
	index >>= kUnitBits;
	length = match_table[index].lengths[slot];
	return match_table[index].links[slot];
}

void StructuredMatchTable::RestrictMatchLength(size_t index, UintFast32 length) noexcept
{
	size_t slot = index & kUnitMask;
	index >>= kUnitBits;
	if (match_table[index].links[slot] != kNullLink && match_table[index].lengths[slot] > length) {
		match_table[index].lengths[slot] = static_cast<uint8_t>(length);
	}
}

void StructuredMatchTable::SetNull(size_t index) noexcept
{
	match_table[index >> kUnitBits].links[index & kUnitMask] = kNullLink;
}

bool StructuredMatchTable::HaveMatch(size_t index) const noexcept
{
	return match_table[index >> kUnitBits].links[index & kUnitMask] != kNullLink;
}

StructuredMatchTable::MatchUnit* StructuredMatchTable::GetBuffer(size_t index) noexcept
{
	return match_table.get() + (index >> kUnitBits) + ((index & kUnitMask) != 0);
}

size_t StructuredMatchTable::CalcMatchBufferSize(size_t block_size, unsigned extra_thread_count) const noexcept
{
	if (block_size < (size_t(1) << 27)) {
		if (extra_thread_count >= 3) {
			return (block_size >> 9) + 64 * 1024;
		}
		else if (extra_thread_count == 2) {
			return (block_size >> 9) + 128 * 1024;
		}
		else if (extra_thread_count == 1) {
			return (block_size >> 8) + 128 * 1024;
		}
		return std::max<size_t>((block_size * 3) >> 8, 384 * 1024);
	}
	else {
		return (block_size >> 7) / (extra_thread_count + 1);
	}
}

size_t StructuredMatchTable::GetMemoryUsage(size_t dictionary_size) noexcept
{
	return ((dictionary_size >> kUnitBits) + 1) * sizeof(MatchUnit);
}

}

#endif // RADYX_STRUCTURED_MATCH_TABLE_H