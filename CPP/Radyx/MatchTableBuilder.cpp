///////////////////////////////////////////////////////////////////////////////
//
// Class: MatchTableBuilder
//        Create a table of string matches concurrently with other threads
//
// Copyright 1998-2000, 2015 Conor McCarthy
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
#include "MatchTableBuilder.h"

namespace Radyx {

void MatchTableBuilder::AllocateMatchBuffer(size_t match_buffer_size)
{
	if (match_buffer_.size() < match_buffer_size) {
		match_buffer_.resize(match_buffer_size);
	}
}

UintFast32 MatchTableBuilder::RepeatCheck(StringMatch* match_buffer,
	size_t index,
	UintFast32 depth,
	UintFast32 list_count) noexcept
{
	int_fast32_t n = list_count - 1;
	int_fast32_t rpt = -1;
	size_t rpt_tail = index;
	do {
		size_t next_i = match_buffer[index].next;
		if (match_buffer[index].from - match_buffer[next_i].from <= depth) {
			if (++rpt == 0)
				rpt_tail = index;
		}
		else {
			if (rpt > kMaxOverlappingRpt - 1) {
				match_buffer[rpt_tail].next = static_cast<UintFast32>(index);
				list_count -= rpt;
			}
			rpt = -1;
		}
		index = next_i;
	} while (--n);
	if (rpt > kMaxOverlappingRpt - 1) {
		match_buffer[rpt_tail].next = static_cast<UintFast32>(index);
		list_count -= rpt;
	}
	return list_count;
}

void MatchTableBuilder::BruteForceBuffered(const DataBlock& block,
	StringMatch* match_buffer,
	size_t index,
	size_t list_count,
	size_t depth,
	size_t max_depth) noexcept
{
	BruteForceMatch buffer[kMaxBruteForceListSize + 1];
	const uint8_t* data_src = block.data + depth;
	size_t i = 0;
	size_t char_count = 2 - (depth & 1);
	for (;;) {
		buffer[i].index = index;
		buffer[i].data_src = data_src + match_buffer[index].from;
		buffer[i].radix_16 = match_buffer[index].chars[0]
			+ (static_cast<unsigned>(match_buffer[index].chars[1]) << 8);
		if (++i >= list_count) {
			break;
		}
		index = match_buffer[index].next;
	}
	size_t limit = max_depth - depth;
	const uint8_t* start = data_src + block.start;
	i = 0;
	do {
		size_t longest = 0;
		size_t j = i + 1;
		size_t longest_index = j;
		const uint8_t* data = buffer[i].data_src;
		unsigned radix_16 = buffer[i].radix_16;
		do {
			size_t len_test;
			if (buffer[j].radix_16 != radix_16) {
				len_test = char_count == 2 && uint8_t(buffer[j].radix_16) == uint8_t(radix_16);
			}
			else {
				const uint8_t* data_2 = buffer[j].data_src;
				len_test = char_count;
				while (data[len_test] == data_2[len_test] && len_test < limit) {
					++len_test;
				}
			}
			if (len_test > longest) {
				longest_index = j;
				longest = len_test;
				if (len_test >= limit) {
					break;
				}
			}
		} while (++j < list_count);
		if (longest > 0) {
			index = buffer[i].index;
			match_buffer[index].next = static_cast<UintFast32>(buffer[longest_index].index);
			match_buffer[index].length = static_cast<uint8_t>(depth)+static_cast<uint8_t>(longest);
		}
		++i;
	} while (i < list_count - 1 && buffer[i].data_src >= start);
}

}