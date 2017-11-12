///////////////////////////////////////////////////////////////////////////////
//
// Class: HashChain
//        Find string matches using a hash table and linked-list
//
// Copyright 2015 Conor McCarthy
// Hash function derived from LzHash.h in 7-zip by Igor Pavlov
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

#ifndef RADYX_HASH_CHAIN_H
#define RADYX_HASH_CHAIN_H

#include <vector>
#include <array>
#include "DataBlock.h"
#include "MatchResult.h"
#include "Crc32.h"

namespace Radyx {

template<unsigned kTableBits2, unsigned kTableBits3, size_t kMatchLenMax>
class HashChain
{
public:
	HashChain(unsigned dictionary_bits_3) NOEXCEPT;
	inline void Initialize(ptrdiff_t prev_index_) NOEXCEPT;
	template<class MatchCollection>
	size_t GetMatches(const DataBlock& block,
		ptrdiff_t index,
		unsigned cycles,
		MatchResult match,
		MatchCollection& matches);

private:
	static const UintFast32 kHashMask2 = (1 << kTableBits2) - 1;
	static const UintFast32 kHashMask3 = (1 << kTableBits3) - 1;
	static const ptrdiff_t kChainMask2 = kHashMask2;
	static const UintFast32 kNullLink = UINT32_MAX;

	std::unique_ptr<UintFast32[]> hash_chain_3;
	ptrdiff_t chain_mask_3;
	ptrdiff_t prev_index;
	std::array<UintFast32, 1 << kTableBits2> table_2;
	std::array<UintFast32, 1 << kTableBits2> hash_chain_2;
	std::array<UintFast32, 1 << kTableBits3> table_3;

	HashChain(const HashChain&) = delete;
	HashChain& operator=(const HashChain&) = delete;
	HashChain(HashChain&&) = delete;
	HashChain& operator=(HashChain&&) = delete;
};

template<unsigned kTableBits2, unsigned kTableBits3, size_t kMatchLenMax>
HashChain<kTableBits2, kTableBits3, kMatchLenMax>::HashChain(unsigned dictionary_bits_3) NOEXCEPT
	: hash_chain_3(new UintFast32[size_t(1) << dictionary_bits_3]),
	chain_mask_3((1 << dictionary_bits_3) - 1)
{
	Initialize(-1);
}

template<unsigned kTableBits2, unsigned kTableBits3, size_t kMatchLenMax>
inline void HashChain<kTableBits2, kTableBits3, kMatchLenMax>::Initialize(ptrdiff_t prev_index_) NOEXCEPT
{
	prev_index = prev_index_;
	// GCC is strict about passing a reference to fill()
	UintFast32 n = kNullLink;
	table_2.fill(n);
	table_3.fill(n);
}

static inline size_t GetHash2(const uint8_t* data) NOEXCEPT
{
	return Crc32::GetHash(data[0]) ^ data[1];
}

static inline size_t GetHash3(const uint8_t* data, size_t hash) NOEXCEPT
{
	hash ^= size_t(data[2]) << 8;
	return hash;
}

template<unsigned kTableBits2, unsigned kTableBits3, size_t kMatchLenMax>
template<class MatchCollection>
size_t HashChain<kTableBits2, kTableBits3, kMatchLenMax>::GetMatches(const DataBlock& block,
	ptrdiff_t index,
	unsigned cycles,
	MatchResult match,
	MatchCollection& matches)
{
	if (index == prev_index) {
		return matches.GetMaxLength();
	}
	assert(index > prev_index);
	matches.Clear();
	if (match.length < 3) {
		matches.push_back(match);
		return match.length;
	}
	const uint8_t* data = block.data;
	// Update hash tables and chains for any positions that were skipped
	while (++prev_index < index) {
		size_t hash = GetHash2(data + prev_index);
		hash_chain_2[prev_index & kChainMask2] = table_2[hash & kHashMask2];
		table_2[hash & kHashMask2] = static_cast<UintFast32>(prev_index);
		hash = GetHash3(data + prev_index, hash) & kHashMask3;
		hash_chain_3[prev_index & chain_mask_3] = table_3[hash];
		table_3[hash] = static_cast<UintFast32>(prev_index);
	}
	data += index;
	ptrdiff_t end_index = index - match.dist;
	// The lowest position to be searched
	size_t end = std::max(end_index, index - kChainMask2 - 1);
	// Max match length
	size_t limit = std::min(block.end - index, kMatchLenMax);
	if (limit < 2) {
		return 0;
	}
	size_t max_len = 0;
	size_t hash = GetHash2(data);
	UintFast32 first_match = table_2[hash & kHashMask2];
	table_2[hash & kHashMask2] = static_cast<UintFast32>(index);
	size_t match_2 = first_match;
	unsigned end_length = match.length;
	for (; match_2 >= end && cycles > 0; match_2 = hash_chain_2[match_2 & kChainMask2]) {
		if (match_2 == kNullLink) {
			// No more of any length so prevent further searching
			end_length = 1;
			break;
		}
		--cycles;
		const uint8_t* data_2 = block.data + match_2;
		if (data[0] != data_2[0]) {
			continue;
		}
		max_len = 2;
		for (; max_len < limit && data[max_len] == data_2[max_len]; ++max_len) {
		}
		matches.push_back(MatchResult(static_cast<unsigned>(max_len),
			static_cast<UintFast32>(index - match_2 - 1)));
		// Prevent this position being tested again
		--match_2;
		break;
	}
	hash_chain_2[index & kChainMask2] = first_match;
	hash = GetHash3(data, hash) & kHashMask3;
	first_match = table_3[hash];
	table_3[hash] = static_cast<UintFast32>(index);
	--end_length;
	if (max_len < end_length) {
		end = std::max(end_index, index - chain_mask_3 - 1);
		size_t match_3 = first_match;
		for (; match_3 != kNullLink && match_3 >= end && cycles > 0; match_3 = hash_chain_3[match_3 & chain_mask_3]) {
			if (match_3 > match_2) {
				continue;
			}
			--cycles;
			const uint8_t* data_2 = block.data + match_3;
			// Only the first byte needs to be checked if the 2nd and 3rd bytes don't overlap in the hash value
			if (data[0] != data_2[0]) {
				continue;
			}
			size_t len_test = 3;
			for (; len_test < limit && data[len_test] == data_2[len_test]; ++len_test) {
			}
			if (len_test > max_len) {
				matches.push_back(MatchResult(static_cast<unsigned>(len_test),
					static_cast<UintFast32>(index - match_3 - 1)));
				max_len = len_test;
				if (len_test >= end_length) {
					break;
				}
			}
		}
	}
	hash_chain_3[index & chain_mask_3] = first_match;
	if (static_cast<unsigned>(max_len) < match.length) {
		matches.push_back(match);
		return match.length;
	}
	return max_len;
}

}
#endif // RADYX_HASH_CHAIN_H