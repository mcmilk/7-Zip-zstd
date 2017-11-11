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

#include <algorithm>
#include <cstring>
#include "common.h"
#include "UnitCompressor.h"

#ifdef UI_EXCEPTIONS
#include "IoException.h"
#include "Strings.h"
#endif

namespace Radyx {

#ifdef UI_EXCEPTIONS

inline void throw_write_exception(int os_error)
{
	throw IoException(Strings::kCannotWriteArchive, os_error, _T(""));
}

#else

inline void throw_write_exception(int /*os_error*/)
{
	throw std::ios_base::failure("");
}

#endif

UnitCompressor::UnitCompressor(size_t dictionary_size_,
	size_t max_buffer_overrun,
	size_t overlap_,
	bool do_bcj,
	bool async_read_)
	: dictionary_size(dictionary_size_),
#ifdef RADYX_BCJ
	overlap((overlap_ > BcjTransform::kMaxUnprocessed) ? overlap_ : BcjTransform::kMaxUnprocessed),
#else
	overlap(overlap_),
#endif
	unprocessed(0),
	buffer_index(0),
	async_read(async_read_),
	working(false)
{
#ifndef RADYX_BCJ
	assert(!do_bcj);
#endif
	data_buffer[0].reset(new uint8_t[dictionary_size_ + max_buffer_overrun]);
	if (async_read_) {
		data_buffer[1].reset(new uint8_t[dictionary_size_ + max_buffer_overrun]);
	}
	Reset(do_bcj, async_read_);
}

void UnitCompressor::Reset(bool do_bcj)
{
	Reset(do_bcj, async_read);
}

void UnitCompressor::Reset(bool do_bcj, bool async_read_)
{
	CheckError();
	WaitCompletion();
	block_start = 0;
	block_end = 0;
	unpack_size = 0;
	pack_size = 0;
	unprocessed = 0;
	do_bcj; // suppress warning
#ifdef RADYX_BCJ
	if (do_bcj) {
		if (bcj.get() == nullptr) {
			bcj.reset(new BcjX86);
		}
		else {
			bcj->Reset();
		}
	}
	else bcj.reset(nullptr);
#endif
	async_read = async_read_ && data_buffer[1].get() != nullptr;
	buffer_index = 0;
}

size_t UnitCompressor::GetAvailableSpace() const
{
	CheckError();
	return dictionary_size - block_end;
}

void UnitCompressor::ThreadFn(void* pwork, int /*unused*/)
{
	UnitCompressor* unit_comp = reinterpret_cast<UnitCompressor*>(pwork);
	try {
		unit_comp->pack_size += unit_comp->args.compressor->Compress(
			unit_comp->args.data_block,
			*unit_comp->args.threads,
			*unit_comp->args.out_stream,
			unit_comp->error_code,
			unit_comp->args.progress);
	}
	// The compressor shouldn't throw anything except bad_alloc
	catch (std::bad_alloc&) {
		unit_comp->error_code.type = ErrorCode::kMemory;
	}
	catch (std::exception&) {
		unit_comp->error_code.type = ErrorCode::kUnknown;
	}
	unit_comp->working = false;
}

void UnitCompressor::CheckError() const
{
	if (error_code.type == ErrorCode::kGood) {
		return;
	}
	if (error_code.type == ErrorCode::kMemory) {
		throw std::bad_alloc();
	}
	else if (error_code.type == ErrorCode::kWrite) {
		throw_write_exception(error_code.os_code);
	}
	else {
		throw std::exception();
	}
}

void UnitCompressor::Compress(CompressorInterface& compressor,
	ThreadPool& threads,
	OutputStream& out_stream,
	Progress* progress)
{
	CheckError();
	if (block_end > block_start - unprocessed)
	{
		size_t processed_end = block_end;
		MutableDataBlock mut_block(data_buffer[buffer_index].get(), block_start - unprocessed, block_end);
#ifdef RADYX_BCJ
		if (bcj.get() != nullptr) {
			processed_end = bcj->Transform(mut_block, true);
			// If the buffer is not full, there is no more data for this unit so
			// process it to the end.
			if (block_end < dictionary_size) {
				processed_end = block_end;
			}
			unprocessed = block_end - processed_end;
		}
#endif
		if (async_read) {
			WaitCompletion();
			args = ThreadArgs(&compressor,
				&threads,
				&out_stream,
				progress,
				DataBlock(data_buffer[buffer_index].get(), mut_block.start, processed_end));
			working = true;
			compress_thread.SetWork(ThreadFn, this, 0);
			buffer_index ^= 1;
		}
		else {
			DataBlock data_block(data_buffer[0].get(), mut_block.start, processed_end);
			pack_size += compressor.Compress(data_block, threads, out_stream, error_code, progress);
			CheckError();
		}
		unpack_size += block_end - block_start;
	}
}

void UnitCompressor::Shift()
{
	CheckError();
	if (block_end > overlap) {
		if (async_read) {
			const uint8_t* data = data_buffer[buffer_index ^ 1].get();
			memcpy(data_buffer[buffer_index].get(), data + block_end - overlap, overlap);
		}
		else {
			uint8_t* data = data_buffer[0].get();
			memmove(data, data + block_end - overlap, overlap);
		}
		block_start = overlap;
	}
	block_end = block_start;
}

void UnitCompressor::Write(OutputStream& out_stream)
{
	CheckError();
	size_t to_write = block_end - block_start;
	out_stream.Write(reinterpret_cast<const char*>(data_buffer[buffer_index].get() + block_start), to_write);
	if (!g_break && out_stream.Fail()) {
		error_code.LoadOsErrorCode();
		throw_write_exception(error_code.os_code);
	}
	unpack_size += to_write;
	pack_size += to_write;
	block_start = 0;
	block_end = 0;
}

}