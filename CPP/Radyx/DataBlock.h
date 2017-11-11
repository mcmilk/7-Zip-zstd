///////////////////////////////////////////////////////////////////////////////
//
// Structures DataBlock and MutableDataBlock
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

#ifndef RADYX_DATA_BLOCK_H
#define RADYX_DATA_BLOCK_H
#include "common.h"

namespace Radyx {

struct MutableDataBlock
{
	uint8_t* data;
	size_t start;
	size_t end;
	MutableDataBlock() {}
	MutableDataBlock(uint8_t* data_, size_t start_, size_t end_)
		: data(data_), start(start_), end(end_) {}
	MutableDataBlock(const MutableDataBlock&) = delete;
	MutableDataBlock& operator=(const MutableDataBlock&) = delete;
	MutableDataBlock(MutableDataBlock&&) = delete;
	MutableDataBlock& operator=(MutableDataBlock&&) = delete;
};

struct DataBlock
{
	const uint8_t* data;
	size_t start;
	size_t end;
	DataBlock() {}
	DataBlock(const uint8_t* data_, size_t start_, size_t end_)
		: data(data_), start(start_), end(end_) {}
	DataBlock(const MutableDataBlock& mb) 
		: data(mb.data), start(mb.start), end(mb.end) {}
};

}

#endif