///////////////////////////////////////////////////////////////////////////////
//
// Class: MatchResult and MatchCollection
//        Storage of string matches
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

#ifndef RADYX_MATCH_RESULT_H
#define RADYX_MATCH_RESULT_H

namespace Radyx {

struct MatchResult
{
	unsigned length;
	UintFast32 dist;
	MatchResult() {}
	MatchResult(unsigned length_, UintFast32 dist_) : length(length_), dist(dist_) {}
};

template<size_t kMatchLenMin, size_t kMatchLenMax>
class MatchCollection
{
public:
	MatchCollection() : count(0) {}
	void push_back(const MatchResult& match) NOEXCEPT {
		matches[count++] = match;
	}
	MatchResult& back() NOEXCEPT {
		return matches[count - 1];
	}
	size_t size() const NOEXCEPT {
		return count;
	}
	MatchResult& operator[](size_t index) NOEXCEPT {
		return matches[index];
	}
	unsigned GetMaxLength() const NOEXCEPT {
		return matches[count - 1].length;
	}
	void Set(const MatchResult& match) NOEXCEPT {
		matches[0] = match; count = 1;
	}
	void Clear() NOEXCEPT {
		count = 0;
	}

private:
	size_t count;
	MatchResult matches[kMatchLenMax - kMatchLenMin + 1];
};

}

#endif