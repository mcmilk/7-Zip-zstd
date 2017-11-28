///////////////////////////////////////////////////////////////////////////////
//
// Class: UnitCompressor
//        Management of solid data units and data block overlap
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

#ifndef RADYX_UNIT_COMPRESSOR_H
#define RADYX_UNIT_COMPRESSOR_H

#include "common.h"
#include "OutputStream.h"
#include "ThreadPool.h"
#include "CompressorInterface.h"
#ifdef RADYX_BCJ
#include "BcjX86.h"
#endif
#include "Progress.h"
#include "ErrorCode.h"

namespace Radyx {

class UnitCompressor
{
public:
	UnitCompressor(size_t dictionary_size_, size_t max_buffer_overrun, size_t overlap_, bool do_bcj, bool async_read_);
	void Reset(bool do_bcj);
	void Reset(bool do_bcj, bool async_read_);
	size_t GetAvailableSpace() const;

	uint8_t* GetAvailableBuffer() NOEXCEPT {
		return data_buffer[buffer_index].get() + block_end;
	}
	void AddByteCount(size_t count) NOEXCEPT {
		block_end += count;
	}
	void RemoveByteCount(size_t count) NOEXCEPT {
		block_end -= count;
	}
	void Compress(CompressorInterface& compressor,
		ThreadPool& threads,
		OutputStream& out_stream,
		Progress* progress);
	void Shift();
	void Write(OutputStream& out_stream);
	void CheckError() const;
	inline void WaitCompletion();

	bool Unprocessed() const NOEXCEPT {
		return block_end > block_start - unprocessed;
	}
	size_t GetUnpackSize() const NOEXCEPT {
		return unpack_size;
	}
	size_t GetPackSize() const NOEXCEPT {
		return pack_size;
	}
#ifdef RADYX_BCJ
	bool UsedBcj() const NOEXCEPT {
		return bcj.get() != nullptr;
	}
	CoderInfo GetBcjCoderInfo() const NOEXCEPT {
		return bcj->GetCoderInfo();
	}
#endif
	size_t GetMemoryUsage() const NOEXCEPT {
		return dictionary_size * (1 + async_read);
	}

private:
	struct ThreadArgs
	{
		CompressorInterface* compressor;
		ThreadPool* threads;
		OutputStream* out_stream;
		Progress* progress;
		DataBlock data_block;  // Must not be a reference
		ThreadArgs() {}
		ThreadArgs(CompressorInterface* compressor_,
			ThreadPool* threads_,
			OutputStream* out_stream_,
			Progress* progress_,
			const DataBlock& data_block_)
			: compressor(compressor_), threads(threads_), out_stream(out_stream_), progress(progress_), data_block(data_block_) {}
	};

	static void ThreadFn(void* pwork, int unused);

	std::unique_ptr<uint8_t[]> data_buffer[2];
#ifdef RADYX_BCJ
	std::unique_ptr<BcjTransform> bcj;
#endif
	size_t dictionary_size;
	size_t block_start;
	size_t block_end;
	size_t overlap;
	size_t unpack_size;
	size_t pack_size;
	size_t unprocessed;
	ThreadPool::Thread compress_thread;
	ThreadArgs args;
	size_t buffer_index;
	ErrorCode error_code;
	bool async_read;
	volatile bool working;

	UnitCompressor(const UnitCompressor&) = delete;
	UnitCompressor& operator=(const UnitCompressor&) = delete;
	UnitCompressor(UnitCompressor&&) = delete;
	UnitCompressor& operator=(UnitCompressor&&) = delete;
};

void UnitCompressor::WaitCompletion()
{
	if (async_read && working) {
		compress_thread.Join();
	}
}

}

#endif // RADYX_UNIT_COMPRESSOR_H