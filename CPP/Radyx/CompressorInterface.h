///////////////////////////////////////////////////////////////////////////////
//
// Class:   CompressorInterface
//          Abstract base class for compressors
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

#ifndef RADYX_COMPRESSOR_INTERFACE_H
#define RADYX_COMPRESSOR_INTERFACE_H

#include "OutputStream.h"
#include "DataBlock.h"
#include "ThreadPool.h"
#include "Progress.h"
#include "CoderInfo.h"
#include "ErrorCode.h"

namespace Radyx {

class CompressorInterface
{
public:
	virtual ~CompressorInterface() {}
	virtual size_t GetDictionarySize() const = 0;
	// Extra allocation required for the dictionary buffer
	virtual size_t GetMaxBufferOverrun() const = 0;
	// Encoding weight for progress display, in units of 1/8th of the total time
	virtual unsigned GetEncodeWeight() const = 0;
	// Compression method. This method should throw only bad_alloc and return I/O errors in error_code
	virtual size_t Compress(const DataBlock& data_block,
		ThreadPool& threads,
		OutputStream& out_stream,
		ErrorCode& error_code,
		Progress* progress = nullptr) = 0;
	// Anything to do at the end of a unit
	virtual size_t Finalize(OutputStream& out_stream) = 0;
	// 7-zip coder info
	virtual CoderInfo GetCoderInfo() const = 0;
	// Estimated memory usage
	virtual size_t GetMemoryUsage(unsigned thread_count) const = 0;
};

}

#endif // RADYX_COMPRESSOR_INTERFACE_H