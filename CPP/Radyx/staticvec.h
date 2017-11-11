///////////////////////////////////////////////////////////////////////////////
//
// Class: staticvec
//        Partial implementation of a vector with no resize ability.
//        Contained object type needs no copy constructor or operator=
//
// Copyright 2017 Conor McCarthy
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

#pragma once

#include <cassert>
#include <memory>

template<class T>
class staticvec
{
public:

	staticvec()
		: count(0)
	{
	}

	staticvec(size_t count_)
		: count(count_),
		buffer(new T[count_])
	{
	}

	size_t size() const
	{
		return count;
	}

	T& operator[](size_t pos)
	{
		assert(pos < count);
		return buffer[pos];
	}

	const T& operator[](size_t pos) const
	{
		assert(pos < count);
		return buffer[pos];
	}

	T& front()
	{
		assert(count > 0);
		return buffer[0];
	}

	const T& front() const
	{
		assert(count > 0);
		return buffer[0];
	}

private:
	size_t count;
	std::unique_ptr<T[]> buffer;
};

