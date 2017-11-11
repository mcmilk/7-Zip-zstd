///////////////////////////////////////////////////////////////////////////////
//
// Class: MatchTable
//        Create a table of string matches
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

#ifndef RADYX_MATCH_TABLE_H
#define RADYX_MATCH_TABLE_H

#include <iostream>
#include <array>
#include <memory>
#include <stdint.h>
#include "winlean.h"
#include "common.h"
#include "DataBlock.h"
#include "MatchTableBuilder.h"
#include "ThreadPool.h"
#include "Progress.h"
#include "PackedMatchTable.h"
#include "StructuredMatchTable.h"
#include "MatchResult.h"
#include "staticvec.h"

namespace Radyx {

template<class MatchTableT>
class MatchTable
{
public:
	MatchTable(size_t dictionary_size_,
		OptionalSetting<size_t> match_buffer_size_,
		uint8_t search_depth,
		unsigned random_filter_ = 0);
	~MatchTable();
	size_t GetDictionarySize() const noexcept {
		return dictionary_size;
	}
	size_t GetMemoryUsage(unsigned thread_count) const noexcept;
	uint8_t* GetOutputByteBuffer(size_t index) noexcept {
		return reinterpret_cast<uint8_t*>(match_table.GetBuffer(index)); }
	char* GetOutputCharBuffer(size_t index) noexcept {
		return reinterpret_cast<char*>(match_table.GetBuffer(index)); }
	inline bool HaveMatch(size_t index) const noexcept {
		return match_table.HaveMatch(index);
	}
	template<unsigned kMatchLenMax>
	inline MatchResult GetMatch(const DataBlock& block, size_t index) const noexcept;

	void BuildTable(const DataBlock& block, ThreadPool& threads, Progress* progress = nullptr);
	void CreateDivision(size_t index) noexcept;
	bool IsRandom(const DataBlock& block, size_t index, size_t size) const noexcept;
	bool IsRandom() const noexcept {
		return is_random;
	}

private:
	static const UintFast32 kNullLink = MatchTableBuilder::kNullLink;
	static const size_t kMin16BitInit = 1 << 19;
	static const UintFast32 kMaxRepeat = 16;
	static const unsigned kHeadTableBits = 16;
	static const unsigned kHeadTableBitsSmall = 8;
	static const size_t kHeadTableSize = size_t(1) << kHeadTableBits;
	static const size_t kHeadTableSizeSmall = 1 << kHeadTableBitsSmall;
	static const size_t kMinRandomCheckSize = size_t(1) << 17;
	static const size_t kRandomCheckEarlyExit = 4096;
	static const unsigned kNonrandomBaseDivisor = 1000;
	static const unsigned kNonrandomCountDivisor = 310;
	static const size_t kMinEncoderRandomCheckSize = 0x1000;
	static const unsigned kLengthSumScaleShift = 4;
	static const float_t kMaxCharDeviation;
	static const float_t kMaxMatchLenFactor;

	struct ThreadArgs
	{
		MatchTable& match_table;
		const DataBlock& block;
		MatchTableBuilder::HeadIndexes& head_indexes;
		Progress* progress;
		uint8_t start_depth;
		ThreadArgs(MatchTable& match_table_,
			const DataBlock& block_,
			MatchTableBuilder::HeadIndexes& head_indexes_,
			Progress* progress_,
			uint8_t start_depth_)
			: match_table(match_table_),
			block(block_),
			head_indexes(head_indexes_),
			progress(progress_),
			start_depth(start_depth_) {}
	};

	static void ThreadFn(void* pwork, int thread_num);
	void InitHeadTable(size_t table_size) noexcept;
	size_t HandleRepeat(const DataBlock& block, ptrdiff_t block_size, ptrdiff_t i, size_t radix_8) noexcept;
	void InitLinks8(const DataBlock& block, Progress* progress);
	void InitLinks16(const DataBlock& block, Progress* progress);

	template<unsigned kMatchLenMax>
	inline unsigned ExtendMatch(const DataBlock& block, size_t index, UintFast32 link, unsigned length) const noexcept;

	// Match table storage and manipulation object
	MatchTableT match_table;
	// Table of list heads
	std::unique_ptr<MatchTableBuilder::ListHead[]> head_table;
	// Dict size fixed at contruction; match_table can't be reallocated
	size_t dictionary_size;
	// Size for buffering
	OptionalSetting<size_t> match_buffer_size;
	// One table builder per thread
	staticvec<MatchTableBuilder> table_builders;
	// Cutoff value for random filtration
	unsigned random_filter;
	// Derived cutoff value for the filtration algorithm
	float_t random_limit;
	// Maximum search depth
	uint8_t search_depth;
	// Random filter activation and the result
	bool check_random;
	bool is_random;
#ifdef RADYX_STATS
	volatile LONGLONG real_time;
	volatile LONGLONG total_time;
#endif
	MatchTable(const MatchTable&) = delete;
	MatchTable& operator=(const MatchTable&) = delete;
	MatchTable(MatchTable&&) = delete;
	MatchTable& operator=(MatchTable&&) = delete;
};

template<class MatchTableT>
const float_t MatchTable<MatchTableT>::kMaxCharDeviation = 10.0f;
template<class MatchTableT>
const float_t MatchTable<MatchTableT>::kMaxMatchLenFactor = 0.175f;

template<class MatchTableT>
MatchTable<MatchTableT>::MatchTable(size_t dictionary_size_,
	OptionalSetting<size_t> match_buffer_size_,
	uint8_t search_depth_,
	unsigned random_filter_)
	: match_table(dictionary_size_),
	head_table(new MatchTableBuilder::ListHead[kHeadTableSize]),
	dictionary_size(dictionary_size_),
	match_buffer_size(match_buffer_size_),
	random_filter(random_filter_),
	random_limit(static_cast<float_t>(random_filter_) * 1.25f - 0.125f),
	search_depth(std::min<uint8_t>(search_depth_, match_table.kMaxLength) & ~1u),
	check_random(random_filter != 0),
	is_random(false)
#ifdef RADYX_STATS
	, real_time(0),
	total_time(0)
#endif
{
	assert(search_depth >= 6);
	// With correct user input checking this won't throw
	if (sizeof(size_t) > 4 && dictionary_size_ > (UINT64_C(1) << 32)) {
		throw std::runtime_error("Dictionary size exceeds limit.");
	}
}

template<class MatchTableT>
size_t MatchTable<MatchTableT>::GetMemoryUsage(unsigned thread_count) const noexcept
{
	size_t buf_size = match_buffer_size.IsSet() ?
		match_buffer_size.Get() : match_table.CalcMatchBufferSize(dictionary_size, thread_count - 1);
	return match_table.GetMemoryUsage(dictionary_size) +
		MatchTableBuilder::GetMemoryUsage(buf_size) * thread_count;
}

template<class MatchTableT>
MatchTable<MatchTableT>::~MatchTable()
{
#ifdef RADYX_STATS
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	std::Tcerr << "Build real time: " << (real_time * 1000 / freq.QuadPart) << std::endl;
	std::Tcerr << "Build total time: " << (total_time * 1000 / freq.QuadPart) << std::endl;
#endif
}

// Get a match from the table and extend it beyond the max depth
template<class MatchTableT>
template<unsigned kMatchLenMax>
MatchResult MatchTable<MatchTableT>::GetMatch(const DataBlock& block, size_t index) const noexcept
{
	MatchResult match;
	UintFast32 link = match_table.GetMatchLinkAndLength(index, match.length);
	match.dist = static_cast<UintFast32>(index) - link - 1;
	if (static_cast<uint8_t>(match.length) == search_depth
		|| match.length == match_table.kMaxLength // from HandleRepeat
		|| ((match.length & MatchTableBuilder::kRptCheckMask) == 0 && match.dist < match.length))
		match.length = ExtendMatch<kMatchLenMax>(block, index, link, match.length);
	return match;
}

template<class MatchTableT>
void MatchTable<MatchTableT>::BuildTable(const DataBlock& block, ThreadPool& threads, Progress* progress)
{
	if (block.end < 3) {
		for (size_t i = 0; i < block.end; ++i) {
			match_table.SetNull(i);
		}
		return;
	}
#ifdef RADYX_STATS
	SubPerformanceCounter(real_time);
	SubPerformanceCounter(total_time);
#endif
	unsigned thread_count = 1 + threads.GetCount();
	if (table_builders.size() < thread_count) {
		table_builders = staticvec<MatchTableBuilder>(thread_count);
	}
	uint8_t start_depth = 2;
	size_t table_size = kHeadTableSize;
	// 8-bit initialization is faster for small dictionaries
	if (block.end < kMin16BitInit && !check_random) {
		// Init the match table to depth 1
		InitLinks8(block, progress);
		start_depth = 1;
		table_size = kHeadTableSizeSmall;
	}
	else {
		// Init the match table to depth 2
		InitLinks16(block, progress);
	}
	if (is_random) {
#ifdef RADYX_STATS
		AsyncAddPerformanceCounter(total_time);
		AddPerformanceCounter(real_time);
#endif
		// Data is probably not compressible
		return;
	}
	// Try to calculate a good buffer size if not user-defined
	if (!match_buffer_size.IsSet()) {
		match_buffer_size.Set(match_table.CalcMatchBufferSize(dictionary_size, threads.GetCount()));
#ifdef RADYX_STATS
		if (progress != nullptr) {
			progress->Erase();
		}
		std::Tcerr << "Buffer size: " << (match_buffer_size / 1024) << "kb" << std::endl;
#endif
	}
	for (unsigned i = 0; i < thread_count; ++i) {
		table_builders[i].AllocateMatchBuffer(match_buffer_size);
	}
	// Create an object for allocating lists to threads
	MatchTableBuilder::HeadIndexes head_indexes(head_table.get(), table_size, block.end, thread_count);
	ThreadArgs args(*this, block, head_indexes, progress, start_depth);
	// Start the worker threads
	for (unsigned i = 0; i < threads.GetCount(); ++i) {
		threads[i].SetWork(MatchTable::ThreadFn, &args, i + 1);
	}
	// Do the main thread's work
	table_builders.front().RecurseLists(block,
		match_table,
		head_table.get(),
		head_indexes,
		progress,
		start_depth,
		search_depth);
#ifdef RADYX_STATS
	AsyncAddPerformanceCounter(total_time);
#endif
	for (unsigned i = 0; i < threads.GetCount(); ++i) {
		threads[i].Join();
	}
	// Restrict the match lengths at the end
	CreateDivision(block.end);
#ifdef RADYX_STATS
	AddPerformanceCounter(real_time);
#endif
}

template<class MatchTableT>
void MatchTable<MatchTableT>::CreateDivision(size_t index) noexcept
{
	// Restrict the match lengths so that they don't reach beyond index
	match_table.SetNull(index - 1);
	for (UintFast32 length = 2; length < match_table.kMaxLength && length <= index; ++length) {
		match_table.RestrictMatchLength(index - length, length);
	}
}

template<class MatchTableT>
void MatchTable<MatchTableT>::ThreadFn(void* pwork, int thread_num)
{
	ThreadArgs* args = reinterpret_cast<ThreadArgs*>(pwork);
	MatchTable<MatchTableT>& match_table = args->match_table;
#ifdef RADYX_STATS
	AsyncSubPerformanceCounter(match_table.total_time);
#endif
	match_table.table_builders[thread_num].RecurseLists(args->block,
		match_table.match_table,
		match_table.head_table.get(),
		args->head_indexes,
		args->progress,
		args->start_depth,
		match_table.search_depth);
#ifdef RADYX_STATS
	AsyncAddPerformanceCounter(match_table.total_time);
#endif
}

template<class MatchTableT>
void MatchTable<MatchTableT>::InitHeadTable(size_t table_size) noexcept
{
	for (size_t i = 0; i < table_size; i += 2) {
		head_table[i].head = kNullLink;
		head_table[i].count = 0;
		head_table[i + 1].head = kNullLink;
		head_table[i + 1].count = 0;
	}
}

// If a repeating byte is found, fill that section of the table with matches of distance 1
template<class MatchTableT>
size_t MatchTable<MatchTableT>::HandleRepeat(const DataBlock& block, ptrdiff_t block_size, ptrdiff_t i, size_t radix_8) noexcept
{
	ptrdiff_t rpt_index = i - (kMaxRepeat - 3);
	// Set the head to the first byte of the repeat and adjust the count
	head_table[radix_8].head = static_cast<UintFast32>(rpt_index - 1);
	head_table[radix_8].count -= kMaxRepeat - 3;
	const uint8_t* data_block = block.data;
	// Find the end
	for (; i < block_size && data_block[i + 2] == static_cast<uint8_t>(radix_8); ++i);
	// Point at which length is less than the max allowable
	ptrdiff_t max_match_end = i - (match_table.kMaxLength - 3);
	// Start processing again at the last 4 bytes
	i -= 3;
	// No point if it's in the overlap region
	if (i >= static_cast<ptrdiff_t>(block.start)) {
		// Set matches at distance 1 and max length
		for (; rpt_index < max_match_end; ++rpt_index) {
			match_table.SetMatchLinkAndLength(rpt_index, static_cast<UintFast32>(rpt_index - 1), match_table.kMaxLength);
		}
		UintFast32 len = static_cast<UintFast32>(i - rpt_index + 5);
		// Set matches at distance 1 and available length
		for (; rpt_index <= i; ++rpt_index, --len) {
			match_table.SetMatchLinkAndLength(rpt_index, static_cast<UintFast32>(rpt_index - 1), len);
		}
	}
	return i;
}

// Initialize the links using only 1 byte. Faster for small dictionaries
template<class MatchTableT>
void MatchTable<MatchTableT>::InitLinks8(const DataBlock& block, Progress* progress)
{
	InitHeadTable(kHeadTableSizeSmall);
	const uint8_t* data_block = block.data;
	size_t radix_8 = data_block[0];
	const ptrdiff_t block_size = block.end - 1;
	UintFast32 count = 0;
	// Pre-load the next byte
	size_t next_next_radix = data_block[1];
	for (ptrdiff_t i = 0; i < block_size; ++i) {
		size_t next_radix = next_next_radix;
		next_next_radix = data_block[i + 2];
		// Check for byte repeat
		if (next_radix != radix_8) {
			count = 0;
			MatchTableBuilder::ListHead& cur_list_head = head_table[radix_8];
			// Link this position to the previous occurance
			match_table.InitMatchLink(i, cur_list_head.head);
			// Set the previous to this position
			cur_list_head.head = static_cast<UintFast32>(i);
			++cur_list_head.count;
			radix_8 = next_radix;
		}
		else {
			++count;
			// Do the usual if the repeat is too short
			if (count < kMaxRepeat - 1) {
				MatchTableBuilder::ListHead& cur_list_head = head_table[radix_8];
				match_table.InitMatchLink(i, cur_list_head.head);
				cur_list_head.head = static_cast<UintFast32>(i);
				++cur_list_head.count;
				radix_8 = next_radix;
			}
			else {
				ptrdiff_t prev_i = i;
				// Eliminate the repeat from the linked list to save time
				i = HandleRepeat(block, block_size - 1, i, radix_8);
				if (progress != nullptr && i >= static_cast<ptrdiff_t>(block.start)) {
					progress->BuildUpdate(i - prev_i + kMaxRepeat - 2);
				}
				next_next_radix = radix_8;
				count = 0;
			}
		}
	}
	is_random = false;
	// Never a match at the last byte
	match_table.SetNull(block.end - 1);
}

template<class MatchTableT>
void MatchTable<MatchTableT>::InitLinks16(const DataBlock& block, Progress* progress)
{
	InitHeadTable(kHeadTableSize);
	const uint8_t* data_block = block.data;
	// Initial 2-byte radix value
	size_t radix_16 = (size_t(data_block[0]) << 8) | data_block[1];
	const ptrdiff_t block_size = block.end - 2;
	UintFast32 count = 0;
	ptrdiff_t rpt_total = 0;
	for (ptrdiff_t i = 0; i < block_size; ++i)
	{
		// Pre-load the next value for speed increase
		size_t next_radix = (size_t(uint8_t(radix_16)) << 8) | data_block[i + 2];
		// Check for byte repeat
		if ((radix_16 & 255) != (radix_16 >> 8)) {
			count = 0;
			MatchTableBuilder::ListHead& cur_list_head = head_table[radix_16];
			// Link this position to the previous occurance
			match_table.InitMatchLink(i, cur_list_head.head);
			// Set the previous to this position
			cur_list_head.head = static_cast<UintFast32>(i);
			++cur_list_head.count;
			radix_16 = next_radix;
		}
		else {
			++count;
			// Do the usual if the repeat is too short
			if (count < kMaxRepeat - 1) {
				MatchTableBuilder::ListHead& cur_list_head = head_table[radix_16];
				match_table.InitMatchLink(i, cur_list_head.head);
				cur_list_head.head = static_cast<UintFast32>(i);
				++cur_list_head.count;
				radix_16 = next_radix;
			}
			else {
				ptrdiff_t prev_i = i;
				// Eliminate the repeat from the linked list to save time
				i = HandleRepeat(block, block_size, i, radix_16);
				rpt_total += i - prev_i + kMaxRepeat - 2;
				if (progress != nullptr && i >= static_cast<ptrdiff_t>(block.start)) {
					progress->BuildUpdate(i - prev_i + kMaxRepeat - 2);
				}
				count = 0;
			}
		}
	}
	// Handle the last value
	if (head_table[radix_16].head != kNullLink) {
		match_table.SetMatchLinkAndLength(block_size, head_table[radix_16].head, UintFast32(2));
	}
	else {
		match_table.SetNull(block_size);
	}
	// Never a match at the last byte
	match_table.SetNull(block.end - 1);
	is_random = false;
	// Too little data can affect the random check. It's compressible if there are many byte repeats.
	if (check_random && block.end >= kMinRandomCheckSize && rpt_total < (block_size >> 5)) {
		float_t limit = random_limit;
		unsigned shift = 0;
		if (block.end < size_t(1) << 27) {
			// Need more precision for small blocks
			shift = 4;
			limit *= 16;
		}
		// The average count that we expect
		int_fast32_t avg = static_cast<int_fast32_t>((block_size << shift) / kHeadTableSize);
		// Sqrt of the block size for later calculation
		float_t sqrt_size = sqrt(static_cast<float_t>(block_size));
		int_least64_t char_total = 0;
		size_t i = 0;
		// Stop after checking kRandomCheckEarlyExit counts to see if it's already clearly non-random
		for (; i < kRandomCheckEarlyExit; ++i) {
			int_fast32_t dev = static_cast<int_fast32_t>(head_table[i].count << shift) - avg;
			char_total += static_cast<int_least64_t>(dev) * dev;
		}
		// Using square roots allows the same limit value to work for different block sizes
		if (sqrt(static_cast<float_t>(char_total)) / sqrt_size > limit) {
			return;
		}
		// May still be random; check the rest
		for (; i < kHeadTableSize; ++i) {
			int_fast32_t dev = static_cast<int_fast32_t>(head_table[i].count << shift) - avg;
			char_total += static_cast<int_least64_t>(dev) * dev;
		}
		// Get a value independant of block size
		float_t deviation = sqrt(static_cast<float_t>(char_total)) / sqrt_size;
		if (deviation <= limit) {
			// We have an even distribution of 16-bit values. Now check for matches by counting the occurrence
			// of distance[n] == distance[n+1]. Crude but it helps.
			UintFast32 nonrnd_count = 0;
			UintFast32 prev_link = kNullLink;
			for (ptrdiff_t j = block.start; j < block_size; ++j) {
				UintFast32 link = match_table.GetInitialMatchLink(j);
				// if prev_link == kNullLink and link == 0 this increments erroneously but it can only happen once.
				nonrnd_count += (link == prev_link + 1);
				prev_link = link;
			}
			size_t divisor = static_cast<unsigned>(kNonrandomCountDivisor / sqrt(static_cast<float_t>(random_filter)));
			size_t encoding_size = block.end - block.start;
			is_random = (nonrnd_count <= (encoding_size / divisor - encoding_size / kNonrandomBaseDivisor));
		}
	}
}

// Load a match from the table and see if it extends beyond the search depth.
// Return the full length and (distance - 1)
template<class MatchTableT>
template<unsigned kMatchLenMax>
unsigned MatchTable<MatchTableT>::ExtendMatch(const DataBlock& block,
	size_t start_index,
	UintFast32 link,
	unsigned length) const noexcept
{
	size_t end_index = start_index + length;
	size_t dist = start_index - link;
	const size_t block_end = std::min(start_index + kMatchLenMax, block.end);
	while (end_index < block_end && end_index - match_table.GetMatchLink(end_index) == dist) {
		end_index += match_table.GetMatchLength(end_index);
	}
	if (end_index >= block_end) {
		return static_cast<unsigned>(block_end - start_index);
	}
	const uint8_t* data = block.data;
	while (end_index < block_end && data[end_index - dist] == data[end_index]) {
		++end_index;
	}
	return static_cast<unsigned>(end_index - start_index);
}

// Determine if a section of the data block is compressible by checking match lengths
// in the match table and the distribution of character values.
template<class MatchTableT>
bool MatchTable<MatchTableT>::IsRandom(const DataBlock& block, size_t index, size_t size) const noexcept
{
	size = std::min(size, block.end - index);
	if (size < kMinEncoderRandomCheckSize) {
		return false;
	}
	size_t end = index + size;
	std::array<int_fast32_t, 0x100> char_count;
	char_count.fill(0);
	UintFast32 length_total = 0;
	// Calculate a target length sum from the size of the chunk and the logarithm
	// of the available dictionary at this point.
	UintFast32 length_target = static_cast<UintFast32>(
		static_cast<float_t>(size) * log(static_cast<float_t>(end)) * kMaxMatchLenFactor);
	for (; index < end; ++index) {
		if (match_table.HaveMatch(index)) {
			unsigned len = match_table.GetMatchLength(index);
			length_total += len;
		}
		else {
			++length_total;
		}
		if (length_total + static_cast<UintFast32>(end - index) > length_target) {
			return false;
		}
		++char_count[block.data[index]];
	}
	float_t char_total = 0.0f;
	// Expected normal character count
	int_fast32_t avg = static_cast<int_fast32_t>(size >> 8);
	// Sum the deviations
	for (size_t i = 0; i < char_count.size(); ++i) {
		float_t delta = static_cast<float_t>(char_count[i] - avg);
		char_total += delta * delta;
	}
	float_t deviation = sqrt(char_total) / sqrt(static_cast<float_t>(size));
	return deviation <= kMaxCharDeviation;
}

}

#endif // RADYX_MATCH_TABLE_H