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

#ifndef RADYX_MATCH_TABLE_BUILDER_H
#define RADYX_MATCH_TABLE_BUILDER_H

#include <array>
#include <vector>
#include <atomic>
#include "common.h"
#include "DataBlock.h"
#include "FlagTable.h"
#include "OptionalSetting.h"
#include "Progress.h"

namespace Radyx {

class MatchTableBuilder
{
public:
	static const UintFast32 kNullLink = UINT32_MAX;
	static const unsigned kRptCheckBits = 5;
	static const unsigned kRptCheckMask = (1 << kRptCheckBits) - 1;

	struct ListHead {
		UintFast32 head;
		UintFast32 count;
		ListHead() NOEXCEPT {}
	};

	class HeadIndexes
	{
	public:
		inline HeadIndexes(ListHead* head_table, size_t table_size, size_t block_size, unsigned thread_count) NOEXCEPT;
		inline ptrdiff_t GetNextIndex() NOEXCEPT;
	private:
		inline size_t GetEndHeadIndex(ListHead* head_table, size_t end, size_t block_size, unsigned thread_count) NOEXCEPT;

		std::atomic_ptrdiff_t front_index;
		std::atomic_ptrdiff_t back_index;
		ptrdiff_t end_index;
	};

public:
	MatchTableBuilder() {}
	void AllocateMatchBuffer(size_t match_buffer_size);

	template<class MatchTableT>
	void RecurseLists(const DataBlock& block,
		MatchTableT& match_table,
		ListHead* head_table,
		HeadIndexes& head_indexes,
		Progress* progress,
		uint8_t start_depth,
		uint8_t max_depth);
	static size_t GetMaxBufferSize(size_t dictionary_size) NOEXCEPT {
		return dictionary_size / sizeof(StringMatch);
	}
	static size_t GetMemoryUsage(size_t match_buffer_size) NOEXCEPT {
		return match_buffer_size * sizeof(StringMatch);
	}

private:
	static const unsigned kRadixBitsLarge = 16;
	static const unsigned kRadixBitsSmall = 8;
	static const size_t kRadixTableSizeLarge = 1 << kRadixBitsLarge;
	static const size_t kRadixTableSizeSmall = 1 << kRadixBitsSmall;
	static const size_t kStackSize = kRadixTableSizeLarge * 3;
	static const int kMaxOverlappingRpt = 7;
	static const unsigned kMinBufferedListSize = 30;
	static const unsigned kMaxBruteForceListSize = 6;

	struct ListTail
	{
		UintFast32 prev_index;
		UintFast32 list_count;
	};
	struct StringMatch
	{
		UintFast32 from;
		uint8_t pad;
		uint8_t length;
		uint8_t chars[2];
		UintFast32 next;
		StringMatch() {}
	};
	struct BruteForceMatch
	{
		size_t index;
		const uint8_t* data_src;
		unsigned radix_16;
	};

	static inline void Copy2Bytes(uint8_t* dest, const uint8_t* src) NOEXCEPT;

	template<class MatchTableT>
	void RecurseListsBuffered(const DataBlock& block,
		MatchTableT& match_table,
		size_t link,
		uint8_t depth,
		uint8_t max_depth,
		UintFast32 list_count,
		size_t stack_base) NOEXCEPT;

	template<class MatchTableT>
	void RecurseLists8(const DataBlock& block,
		MatchTableT& match_table,
		size_t link,
		UintFast32 count,
		UintFast32 max_depth) NOEXCEPT;

	template<class MatchTableT>
	void RecurseLists16(const DataBlock& block,
		MatchTableT& match_table,
		size_t link,
		UintFast32 count,
		UintFast32 max_depth) NOEXCEPT;

	template<class MatchTableT>
	void RecurseListStack(const DataBlock& block,
		MatchTableT& match_table,
		size_t st_index,
		UintFast32 max_depth) NOEXCEPT;

	UintFast32 RepeatCheck(StringMatch* match_buffer,
		size_t index,
		UintFast32 depth,
		UintFast32 list_count) NOEXCEPT;

	template<class MatchTableT>
	UintFast32 RepeatCheck(MatchTableT& match_table,
		size_t link,
		UintFast32 depth,
		UintFast32 list_count) NOEXCEPT;

	void BruteForceBuffered(const DataBlock& block,
		StringMatch* match_buffer,
		size_t index,
		size_t list_count,
		size_t depth,
		size_t max_depth) NOEXCEPT;

	template<class MatchTableT>
	void BruteForce(const DataBlock& block,
		MatchTableT& match_table,
		size_t link,
		size_t list_count,
		UintFast32 depth,
		UintFast32 max_depth) NOEXCEPT;

	// Fast-reset bool tables for recording initial radix value occurrence
	FlagTable<UintFast32, 8> flag_table_8;
	FlagTable<UintFast32, 16> flag_table_16;
	// Table to track the tails of new sub-lists
	ListTail sub_tails[kRadixTableSizeLarge];
	// Stack of new sub-list heads
	std::array<ListHead, kStackSize> stack;
	// Buffer for buffered list processing
	std::vector<StringMatch> match_buffer_;

	MatchTableBuilder(const MatchTableBuilder&) = delete;
	MatchTableBuilder(MatchTableBuilder&&) = delete;
	MatchTableBuilder& operator=(const MatchTableBuilder&) = delete;
	MatchTableBuilder& operator=(MatchTableBuilder&&) = delete;
};

MatchTableBuilder::HeadIndexes::HeadIndexes(ListHead* head_table, size_t table_size, size_t block_size, unsigned thread_count) NOEXCEPT
{
	front_index = 0;
	if (thread_count > 1) {
		// Try to prevent long lists being done last so all threads can finish around the same time
		size_t end = table_size;
		// Find the first list with > 1 string
		for (; end > 1 && head_table[end - 1].count <= 1; --end) {
		}
		back_index = end;
		// Find a reasonable end point
		end_index = GetEndHeadIndex(head_table, end, block_size, thread_count);
	}
	else  {
		back_index = table_size;
		end_index = table_size;
	}
}

// Find an end point preceeded by a bunch of short lists
size_t MatchTableBuilder::HeadIndexes::GetEndHeadIndex(ListHead* head_table, size_t end, size_t block_size, unsigned thread_count) NOEXCEPT
{
	UintFast32 max_count;
	if (end <= MatchTableBuilder::kRadixTableSizeSmall) {
		max_count = static_cast<UintFast32>((block_size << 4) / end);
	}
	else {
		max_count = static_cast<UintFast32>((block_size << 7) / end);
	}
	UintFast32 target_sum = (max_count * (thread_count - 1)) >> 3;
	do {
		UintFast32 sum = 0;
		size_t i = end;
		for (; i > 1 && head_table[i - 1].count <= max_count; --i) {
			sum += head_table[i - 1].count;
			if (sum >= target_sum) {
				return end;
			}
		}
		end = i - 1;
	} while (end > 0);
	return 0;
}

// Atomically take a list from the head table
ptrdiff_t MatchTableBuilder::HeadIndexes::GetNextIndex() NOEXCEPT
{
	if (back_index > end_index) {
		ptrdiff_t index = --back_index;
		if (index >= end_index) {
			return index;
		}
	}
	if (front_index < end_index) {
		ptrdiff_t index = front_index++;
		if (index < end_index) {
			return index;
		}
	}
	return -1;
}

// Iterate the head table concurrently with other threads, and recurse each list until max_depth is reached
template<class MatchTableT>
void MatchTableBuilder::RecurseLists(const DataBlock& block,
	MatchTableT& match_table,
	ListHead* head_table,
	HeadIndexes& head_indexes,
	Progress* progress,
	uint8_t start_depth,
	uint8_t max_depth)
{
	const size_t block_start = block.start;
	const size_t block_size = block.end - block_start;
	while (!g_break)
	{
		// Get the next to process
		ptrdiff_t index = head_indexes.GetNextIndex();
		if (index < 0) {
			break;
		}
		ListHead list_head = head_table[index];
		// Check there's something to do
		if (list_head.count < 2 || list_head.head < block.start) {
			if (progress != nullptr && list_head.count != 0 && list_head.head >= block.start) {
				// Must be length 1
				progress->BuildUpdate(1);
			}
			continue;
		}
		if (start_depth < 2) {
			// 8-bit initialization was used
			RecurseLists8(block, match_table, list_head.head, list_head.count, max_depth);
		}
		else if (list_head.count < kMinBufferedListSize
			|| list_head.count > match_buffer_.size())
		{
			// Not worth buffering or too long
			RecurseLists16(block, match_table, list_head.head, list_head.count, max_depth);
		}
		else {
			RecurseListsBuffered(block, match_table, list_head.head, start_depth, max_depth, list_head.count, 0);
		}
		if (progress != nullptr) {
			progress->BuildUpdate(static_cast<uint_least64_t>(list_head.count) * block_size / block.end);
		}
	}
}

void MatchTableBuilder::Copy2Bytes(uint8_t* dest, const uint8_t* src) NOEXCEPT
{
#ifndef DISALLOW_UNALIGNED_ACCESS
	(reinterpret_cast<uint16_t*>(dest))[0] = (reinterpret_cast<const uint16_t*>(src))[0];
#else
	dest[0] = src[0];
	dest[1] = src[1];
#endif
}

// Copy the list into a buffer and recurse it there. This decreases cache misses and allows
// data characters to be loaded every second pass and stored for use in the next 2 passes
template<class MatchTableT>
void MatchTableBuilder::RecurseListsBuffered(const DataBlock& block,
	MatchTableT& match_table,
	size_t link,
	uint8_t depth,
	uint8_t max_depth,
	UintFast32 list_count,
	size_t stack_base) NOEXCEPT
{
	// Faster than vector operator[] and possible because the vector is not reallocated in this method
	StringMatch* match_buffer = match_buffer_.data();
	const size_t block_start = block.start;
	// Create an offset data buffer pointer for reading the next bytes
	const uint8_t* data_src = block.data + depth;
	size_t count = 0;
	for (; count < list_count; ++count) {
		// Get 2 data characters for later. This doesn't block on a cache miss.
		Copy2Bytes(match_buffer[count].chars, data_src + link);
		// Record the actual location of this suffix
		match_buffer[count].from = static_cast<UintFast32>(link);
		// Next
		link = match_table.GetMatchLink(link);
		// Initialize the next link
		match_buffer[count].next = static_cast<UintFast32>(count + 1);
		// Match length
		match_buffer[count].length = depth;
	}
	// Make the last element circular so pre-loading doesn't read past the end.
	match_buffer[count - 1].next = static_cast<UintFast32>(count - 1);
	// Clear the bool flags
	flag_table_8.Reset();
	++depth;
	++data_src;
	// The last element is done separately and won't be copied back at the end
	--count;
	size_t st_index = stack_base;
	size_t prev_index_8[kRadixTableSizeSmall];
	size_t index = 0;
	do {
		link = match_buffer[index].from;
		size_t radix_8 = match_buffer[index].chars[0];
		// Seen this char before?
		if (flag_table_8.IsSet(radix_8)) {
			const size_t prev = prev_index_8[radix_8];
			++sub_tails[radix_8].list_count;
			// Link the previous occurrence to this one and record the new length
			match_buffer[prev].length = depth;
			match_buffer[prev].next = static_cast<UintFast32>(index);
		}
		else {
			flag_table_8.Set(radix_8);
			sub_tails[radix_8].list_count = 1;
			// Add the new sub list to the stack
			stack[st_index].head = static_cast<UintFast32>(index);
			// This will be converted to a count at the end
			stack[st_index].count = static_cast<UintFast32>(radix_8);
			++st_index;
		}
		prev_index_8[radix_8] = index;
		++index;
	} while (index < count);
	// Do the last element
	link = match_buffer[index].from;
	size_t radix_8 = match_buffer[index].chars[0];
	// Nothing to do if there was no previous
	if (flag_table_8.IsSet(radix_8)) {
		const size_t prev = prev_index_8[radix_8];
		++sub_tails[radix_8].list_count;
		match_buffer[prev].length = depth;
		match_buffer[prev].next = static_cast<UintFast32>(index);
	}
	// Convert radix values on the stack to counts
	for (size_t j = stack_base; j < st_index; ++j) {
		stack[j].count = sub_tails[stack[j].count].list_count;
	}
	while (!g_break && st_index > stack_base) {
		// Pop an item off the stack
		--st_index;
		list_count = stack[st_index].count;
		if (list_count < 2) {
			// Nothing to match with
			continue;
		}
		// Check stack space. The first comparison is unnecessary but it's a constant so should be faster
		if (st_index > stack.size() - kRadixTableSizeSmall
			&& st_index > stack.size() - list_count)
		{
			// Stack may not be able to fit all possible new items. This is very rare.
			continue;
		}
		index = stack[st_index].head;
		link = match_buffer[index].from;
		if (link < block_start) {
			// Chain starts in the overlap region which is already encoded
			continue;
		}
		depth = match_buffer[index].length;
		// Check for overlapping repeated matches. These are rare but potentially a big time waster.
		if ((depth & kRptCheckMask) == 0 && list_count > kMaxOverlappingRpt + 1) {
			list_count = RepeatCheck(match_buffer, index, depth, list_count);
		}
		if (list_count <= kMaxBruteForceListSize) {
			// Quicker to use brute force, each string compared with all previous strings
			BruteForceBuffered(block,
				match_buffer,
				index,
				list_count,
				depth,
				max_depth);
			continue;
		}
		size_t slot = depth & 1;
		++depth;
		// Update the offset data buffer pointer
		data_src = block.data + depth;
		flag_table_8.Reset();
		// Last pass is done separately
		if (depth < max_depth) {
			size_t prev_st_index = st_index;
			// Last element done separately
			--list_count;
			// slot is the char cache index. If 1 then chars need to be loaded.
			if (slot != 0) do {
				radix_8 = match_buffer[index].chars[1];
				const size_t next_index = match_buffer[index].next;
				// Pre-load the next link and data bytes to avoid waiting for RAM access
				Copy2Bytes(match_buffer[index].chars, data_src + link);
				const size_t next_link = match_buffer[next_index].from;
				if (flag_table_8.IsSet(radix_8)) {
					const size_t prev = prev_index_8[radix_8];
					++sub_tails[radix_8].list_count;
					match_buffer[prev].length = depth;
					match_buffer[prev].next = static_cast<UintFast32>(index);
				}
				else {
					flag_table_8.Set(radix_8);
					sub_tails[radix_8].list_count = 1;
					stack[st_index].head = static_cast<UintFast32>(index);
					stack[st_index].count = static_cast<UintFast32>(radix_8);
					++st_index;
				}
				prev_index_8[radix_8] = index;
				index = next_index;
				link = next_link;
			} while (--list_count != 0);
			else do {
				radix_8 = match_buffer[index].chars[0];
				const size_t next_index = match_buffer[index].next;
				// Pre-load the next link to avoid waiting for RAM access
				const size_t next_link = match_buffer[next_index].from;
				if (flag_table_8.IsSet(radix_8)) {
					const size_t prev = prev_index_8[radix_8];
					++sub_tails[radix_8].list_count;
					match_buffer[prev].length = depth;
					match_buffer[prev].next = static_cast<UintFast32>(index);
				}
				else {
					flag_table_8.Set(radix_8);
					sub_tails[radix_8].list_count = 1;
					stack[st_index].head = static_cast<UintFast32>(index);
					stack[st_index].count = static_cast<UintFast32>(radix_8);
					++st_index;
				}
				prev_index_8[radix_8] = index;
				index = next_index;
				link = next_link;
			} while (--list_count != 0);
			radix_8 = match_buffer[index].chars[slot];
			if (flag_table_8.IsSet(radix_8)) {
				if (slot != 0) Copy2Bytes(match_buffer[index].chars, data_src + link);
				const size_t prev = prev_index_8[radix_8];
				++sub_tails[radix_8].list_count;
				match_buffer[prev].length = depth;
				match_buffer[prev].next = static_cast<UintFast32>(index);
			}
			for (size_t j = prev_st_index; j < st_index; ++j) {
				stack[j].count = sub_tails[stack[j].count].list_count;
			}
		}
		else {
			// The last pass at max_depth
			do {
				radix_8 = match_buffer[index].chars[slot];
				const size_t next_index = match_buffer[index].next;
				// Pre-load the next link.
				// The last element in match_buffer is circular so this is never an access violation.
				const size_t next_link = match_buffer[next_index].from;
				if (flag_table_8.IsSet(radix_8)) {
					const size_t prev = prev_index_8[radix_8];
					match_buffer[prev].length = depth;
					match_buffer[prev].next = static_cast<UintFast32>(index);
				}
				else {
					flag_table_8.Set(radix_8);
				}
				prev_index_8[radix_8] = index;
				index = next_index;
				link = next_link;
			} while (--list_count != 0);
		}
	}
	if (!g_break) {
		// Copy everything back, except the last link which never changes
		for (index = 0; index < count; ++index) {
			size_t from = match_buffer[index].from;
			size_t next = match_buffer[index].next;
			match_table.SetMatchLinkAndLength(from, match_buffer[next].from, match_buffer[index].length);
		}
	}
}

template<class MatchTableT>
void MatchTableBuilder::RecurseLists8(const DataBlock& block,
	MatchTableT& match_table,
	size_t link,
	UintFast32 count,
	UintFast32 max_depth) NOEXCEPT
{
	flag_table_8.Reset();
	// Offset data pointer
	const uint8_t* data_src = block.data + 1;
	size_t next_radix_8 = data_src[link];
	size_t st_index = 0;
	--count;
	do
	{
		// Pre-load the next link
		size_t next_link = match_table.GetInitialMatchLink(link);
		// Current data char
		size_t radix_8 = next_radix_8;
		// Pre-load the next char
		next_radix_8 = data_src[next_link];
		// Seen this char before?
		if (flag_table_8.IsSet(radix_8)) {
			const size_t prev = sub_tails[radix_8].prev_index;
			++sub_tails[radix_8].list_count;
			// Link the previous location to here
			match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), UintFast32(2));
		}
		else {
			flag_table_8.Set(radix_8);
			sub_tails[radix_8].list_count = 1;
			// Add new sub-list to the stack
			stack[st_index].head = static_cast<UintFast32>(link);
			// Converted to a count at the end
			stack[st_index].count = static_cast<UintFast32>(radix_8);
			++st_index;
		}
		sub_tails[radix_8].prev_index = static_cast<UintFast32>(link);
		link = next_link;
	} while (--count > 0);
	if (flag_table_8.IsSet(next_radix_8)) {
		const size_t prev = sub_tails[next_radix_8].prev_index;
		++sub_tails[next_radix_8].list_count;
		match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), UintFast32(2));
		sub_tails[next_radix_8].prev_index = static_cast<UintFast32>(link);
	}
	for (size_t j = 0; j < st_index; ++j) {
		size_t radix_8 = stack[j].count;
		stack[j].count = sub_tails[radix_8].list_count;
		match_table.SetNull(sub_tails[radix_8].prev_index);
	}
	// Process the stack
	RecurseListStack(block, match_table, st_index, max_depth);
}

template<class MatchTableT>
void MatchTableBuilder::RecurseLists16(const DataBlock& block,
	MatchTableT& match_table,
	size_t link,
	UintFast32 count,
	UintFast32 max_depth) NOEXCEPT
{
	flag_table_8.Reset();
	flag_table_16.Reset();
	// Offset data pointer. This method is only called at depth 2
	const uint8_t* data_src = block.data + 2;
	size_t block_start = block.start;
	// Load radix values from the data chars
	size_t next_radix_8 = data_src[link];
	size_t next_radix_16 = next_radix_8 + (static_cast<size_t>(data_src[link + 1]) << 8);
	size_t prev_index_8[kRadixTableSizeSmall];
	size_t st_index = 0;
	// Last one is done separately
	--count;
	do
	{
		// Pre-load the next link
		size_t next_link = match_table.GetInitialMatchLink(link);
		// Initialization doesn't set lengths to 2 because it's a waste of time if buffering is used
		match_table.SetMatchLength(link, static_cast<UintFast32>(next_link), UintFast32(2));
		size_t radix_8 = next_radix_8;
		size_t radix_16 = next_radix_16;
		next_radix_8 = data_src[next_link];
		next_radix_16 = next_radix_8 + (static_cast<size_t>(data_src[next_link + 1]) << 8);
		if (flag_table_8.IsSet(radix_8)) {
			const size_t prev = prev_index_8[radix_8];
			// Link the previous occurrence to this one at length 3.
			// This will be overwritten if a 4 is found.
			match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), UintFast32(3));
		}
		else {
			flag_table_8.Set(radix_8);
		}
		prev_index_8[radix_8] = link;
		if (flag_table_16.IsSet(radix_16)) {
			const size_t prev = sub_tails[radix_16].prev_index;
			++sub_tails[radix_16].list_count;
			// Link at length 4, overwriting the 3
			match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), UintFast32(4));
		}
		else if (link >= block_start) {
			flag_table_16.Set(radix_16);
			sub_tails[radix_16].list_count = 1;
			stack[st_index].head = static_cast<UintFast32>(link);
			stack[st_index].count = static_cast<UintFast32>(radix_16);
			++st_index;
		}
		sub_tails[radix_16].prev_index = static_cast<UintFast32>(link);
		link = next_link;
	} while (--count > 0);
	// Do the last location
	if (flag_table_8.IsSet(next_radix_8)) {
		const size_t prev = prev_index_8[next_radix_8];
		match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), UintFast32(3));
	}
	if (flag_table_16.IsSet(next_radix_16)) {
		const size_t prev = sub_tails[next_radix_16].prev_index;
		++sub_tails[next_radix_16].list_count;
		match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), UintFast32(4));
	}
	for (size_t i = 0; i < st_index; ++i) {
		stack[i].count = sub_tails[stack[i].count].list_count;
	}
	RecurseListStack(block, match_table, st_index, max_depth);
}

template<class MatchTableT>
void MatchTableBuilder::RecurseListStack(const DataBlock& block,
	MatchTableT& match_table,
	size_t st_index,
	UintFast32 max_depth) NOEXCEPT
{
	// Maximum depth at which lists will be sent to the buffered method.
	UintFast32 max_buffered_depth = max_depth - std::min<UintFast32>(max_depth / 4 + 2, 8u);
	size_t block_start = block.start;
	while (!g_break && st_index > 0) {
		--st_index;
		UintFast32 list_count = stack[st_index].count;
		if (list_count < 2) {
			// Nothing to do
			continue;
		}
		if (st_index > stack.size() - kRadixTableSizeLarge
			&& st_index > stack.size() - list_count)
		{
			// Potential stack overflow. Rare.
			continue;
		}
		size_t link = stack[st_index].head;
		// The current depth
		UintFast32 depth = match_table.GetMatchLength(link);
		if (list_count >= kMinBufferedListSize
			&& list_count <= match_buffer_.size()
			&& depth <= max_buffered_depth)
		{
			// Small enough to fit in the buffer now
			RecurseListsBuffered(block,
				match_table,
				link,
				static_cast<uint8_t>(depth),
				static_cast<uint8_t>(max_depth),
				list_count,
				st_index);
			continue;
		}
		// Check for overlapping repeated matches. These are rare but potentially a big time waster.
		if ((depth & kRptCheckMask) == 0 && list_count > kMaxOverlappingRpt + 1) {
			list_count = RepeatCheck(match_table, link, depth, list_count);
		}
		if (list_count <= kMaxBruteForceListSize) {
			// Quicker to use brute force, each string compared with all previous strings
			BruteForce(block,
				match_table,
				link,
				list_count,
				depth,
				max_depth);
			continue;
		}
		const uint8_t* data_src = block.data + depth;
		size_t next_radix_8 = data_src[link];
		size_t next_radix_16 = next_radix_8 + (static_cast<size_t>(data_src[link + 1]) << 8);
		// Next depth for 1 extra char
		++depth;
		// and for 2
		UintFast32 depth_2 = depth + 1;
		flag_table_8.Reset();
		flag_table_16.Reset();
		size_t prev_index_8[kRadixTableSizeSmall];
		size_t prev_st_index = st_index;
		// Last location is done separately
		--list_count;
		// Last pass is done separately. Both of these values are always even.
		if (depth_2 < max_depth) {
			do {
				size_t radix_8 = next_radix_8;
				size_t radix_16 = next_radix_16;
				size_t next_link = match_table.GetMatchLink(link);
				next_radix_8 = data_src[next_link];
				next_radix_16 = next_radix_8 + (static_cast<size_t>(data_src[next_link + 1]) << 8);
				if (flag_table_8.IsSet(radix_8)) {
					const size_t prev = prev_index_8[radix_8];
					// Odd numbered match length, will be overwritten if 2 chars are matched
					match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth);
				}
				else {
					flag_table_8.Set(radix_8);
				}
				prev_index_8[radix_8] = link;
				if (flag_table_16.IsSet(radix_16)) {
					const size_t prev = sub_tails[radix_16].prev_index;
					++sub_tails[radix_16].list_count;
					match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth_2);
				}
				else if (link >= block_start) {
					flag_table_16.Set(radix_16);
					sub_tails[radix_16].list_count = 1;
					stack[st_index].head = static_cast<UintFast32>(link);
					stack[st_index].count = static_cast<UintFast32>(radix_16);
					++st_index;
				}
				sub_tails[radix_16].prev_index = static_cast<UintFast32>(link);
				link = next_link;
			} while (--list_count != 0);
			if (flag_table_8.IsSet(next_radix_8)) {
				const size_t prev = prev_index_8[next_radix_8];
				match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth);
			}
			if (flag_table_16.IsSet(next_radix_16)) {
				const size_t prev = sub_tails[next_radix_16].prev_index;
				++sub_tails[next_radix_16].list_count;
				match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth_2);
			}
			for (size_t i = prev_st_index; i < st_index; ++i) {
				stack[i].count = sub_tails[stack[i].count].list_count;
			}
		}
		else {
			do {
				size_t radix_8 = next_radix_8;
				size_t radix_16 = next_radix_16;
				size_t next_link = match_table.GetMatchLink(link);
				next_radix_8 = data_src[next_link];
				next_radix_16 = next_radix_8 + (static_cast<size_t>(data_src[next_link + 1]) << 8);
				if (flag_table_8.IsSet(radix_8)) {
					const size_t prev = prev_index_8[radix_8];
					match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth);
				}
				else {
					flag_table_8.Set(radix_8);
				}
				prev_index_8[radix_8] = link;
				if (flag_table_16.IsSet(radix_16)) {
					const size_t prev = sub_tails[radix_16].prev_index;
					match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth_2);
				}
				else if (link >= block_start) {
					flag_table_16.Set(radix_16);
				}
				sub_tails[radix_16].prev_index = static_cast<UintFast32>(link);
				link = next_link;
			} while (--list_count != 0);
			if (flag_table_8.IsSet(next_radix_8)) {
				const size_t prev = prev_index_8[next_radix_8];
				match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth);
			}
			if (flag_table_16.IsSet(next_radix_16)) {
				const size_t prev = sub_tails[next_radix_16].prev_index;
				match_table.SetMatchLinkAndLength(prev, static_cast<UintFast32>(link), depth_2);
			}
		}
	}
}

// Check to see if there are multiple overlapping matches in a row and delete them
template<class MatchTableT>
UintFast32 MatchTableBuilder::RepeatCheck(MatchTableT& match_table,
	size_t link,
	UintFast32 depth,
	UintFast32 list_count) NOEXCEPT
{
	int_fast32_t n = list_count - 1;
	int_fast32_t rpt = -1;
	size_t rpt_tail = link;
	do {
		size_t next_link = match_table.GetMatchLink(link);
		if (link - next_link <= depth) {
			if (++rpt == 0) {
				rpt_tail = link;
			}
		}
		else {
			if (rpt > kMaxOverlappingRpt - 1) {
				// Link the last one to the first
				match_table.SetMatchLink(rpt_tail, static_cast<UintFast32>(link), depth);
				list_count -= rpt;
			}
			rpt = -1;
		}
		link = next_link;
	} while (--n);
	if (rpt > kMaxOverlappingRpt - 1) {
		match_table.SetMatchLink(rpt_tail, static_cast<UintFast32>(link), depth);
		list_count -= rpt;
	}
	return list_count;
}

// Compare each string with all others to find the best match
template<class MatchTableT>
void MatchTableBuilder::BruteForce(const DataBlock& block,
	MatchTableT& match_table,
	size_t link,
	size_t list_count,
	UintFast32 depth,
	UintFast32 max_depth) NOEXCEPT
{
	size_t buffer[kMaxBruteForceListSize + 1];
	buffer[0] = link;
	size_t i = 1;
	// Pre-load all locations
	do {
		link = match_table.GetMatchLink(link);
		buffer[i] = link;
	} while (++i < list_count);
	size_t limit = max_depth - depth;
	const uint8_t* data_src = block.data + depth;
	size_t block_start = block.start;
	i = 0;
	do {
		size_t longest = 0;
		size_t j = i + 1;
		size_t longest_index = j;
		const uint8_t* data = data_src + buffer[i];
		do {
			const uint8_t* data_2 = data_src + buffer[j];
			size_t len_test = 0;
			while (data[len_test] == data_2[len_test] && len_test < limit) {
				++len_test;
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
			match_table.SetMatchLinkAndLength(buffer[i],
				static_cast<UintFast32>(buffer[longest_index]),
				depth + static_cast<UintFast32>(longest));
		}
		++i;
	} while (i < list_count - 1 && buffer[i] >= block_start);
}

}

#endif // RADYX_MATCH_TABLE_BUILDER_H