///////////////////////////////////////////////////////////////////////////////
//
// Class: Lzma2Compressor
//        Manage LZMA2 compression by multiple threads
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

#ifndef RADYX_LZMA2_COMPRESSOR_H
#define RADYX_LZMA2_COMPRESSOR_H

#include <algorithm>
#include "winlean.h"
#include "common.h"
#include "OutputStream.h"
#include "Lzma2Encoder.h"
#include "CompressorInterface.h"
#include "AsyncWriter.h"
#include "Lzma2Options.h"
#include "staticvec.h"

namespace Radyx {

template<class MatchTableT>
class Lzma2Compressor : public CompressorInterface
{
public:
	Lzma2Compressor(const Lzma2Options& options);
	virtual ~Lzma2Compressor();
	size_t GetDictionarySize() const NOEXCEPT;
	size_t GetMaxBufferOverrun() const NOEXCEPT;
	inline unsigned GetEncodeWeight() const NOEXCEPT;
	size_t Compress(const DataBlock& data_block,
		ThreadPool& threads,
		OutputStream& out_stream,
		ErrorCode& error_code,
		Progress* progress = nullptr);
	size_t Finalize(OutputStream& out_stream);
	CoderInfo GetCoderInfo() const NOEXCEPT;
	size_t GetMemoryUsage(unsigned thread_count) const NOEXCEPT;

private:
	struct ThreadArgs
	{
		Lzma2Compressor<MatchTableT>& compressor;
		const DataBlock& data_block;
		Progress* progress;
		ThreadArgs(Lzma2Compressor<MatchTableT>& compressor_,
			const DataBlock& data_block_,
			Progress* progress_)
			: compressor(compressor_),
			data_block(data_block_),
			progress(progress_) {}
	};

	struct EncoderArgs
	{
		size_t start;
		size_t end;
		size_t encoded_size;
		Lzma2Encoder encoder;
	};

	static const unsigned kMinBytesPerThread = 0x4000;
	static const uint8_t kChunkEof = 0;

	static void ThreadFn(void* pwork, int encoder_num);

	MatchTable<MatchTableT> match_table;
	staticvec<EncoderArgs> encoders;
	const Lzma2Options options;
	ThreadPool::Thread writer_thread;
	size_t dictionary_max;
	bool needed_random_check;
#ifdef RADYX_STATS
	volatile LONGLONG total_time;
#endif
	Lzma2Compressor(const Lzma2Compressor&) = delete;
	Lzma2Compressor& operator=(const Lzma2Compressor&) = delete;
	Lzma2Compressor(Lzma2Compressor&&) = delete;
	Lzma2Compressor& operator=(Lzma2Compressor&&) = delete;
};

template<class MatchTableT>
Lzma2Compressor<MatchTableT>::Lzma2Compressor(const Lzma2Options& options_)
	: match_table(std::min<size_t>(options_.dictionary_size, Lzma2Encoder::GetDictionarySizeMax()),
		options_.match_buffer_size,
		static_cast<uint8_t>(std::min<unsigned>(options_.fast_length, 0xFF)),
		options_.random_filter),
	options(options_),
	dictionary_max(0),
	needed_random_check(false)
#ifdef RADYX_STATS
	,total_time(0)
#endif
{
}

template<class MatchTableT>
Lzma2Compressor<MatchTableT>::~Lzma2Compressor()
{
#ifdef RADYX_STATS
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	std::Tcerr << "Encoder total time: " << (total_time * 1000 / freq.QuadPart) << std::endl;
#endif
}

template<class MatchTableT>
size_t Lzma2Compressor<MatchTableT>::GetDictionarySize() const NOEXCEPT
{
	return match_table.GetDictionarySize();
}

template<class MatchTableT>
size_t Lzma2Compressor<MatchTableT>::GetMaxBufferOverrun() const NOEXCEPT
{
	return Lzma2Encoder::GetMatchLenMax();
}

template<class MatchTableT>
unsigned Lzma2Compressor<MatchTableT>::GetEncodeWeight() const NOEXCEPT
{
	if (options.encoder_mode == Lzma2Options::kFastMode) {
		return 1;
	}
	return (options.encoder_mode == Lzma2Options::kBestMode)
		? 4 - (options.dictionary_size >= (UINT32_C(1) << 28))
		: 3;
}

template<class MatchTableT>
void Lzma2Compressor<MatchTableT>::ThreadFn(void* pwork, int encoder_num)
{
	ThreadArgs* args = reinterpret_cast<ThreadArgs*>(pwork);
#ifdef RADYX_STATS
	AsyncSubPerformanceCounter(args->compressor.total_time);
#endif
	EncoderArgs& encoder = args->compressor.encoders[encoder_num];
	encoder.encoded_size = encoder.encoder.Encode(args->compressor.match_table,
		args->data_block,
		encoder.start,
		encoder.end,
		args->compressor.options,
		args->compressor.needed_random_check,
		args->progress,
		nullptr);
#ifdef RADYX_STATS
	AsyncAddPerformanceCounter(args->compressor.total_time);
#endif
}

// This method should throw only bad_alloc and return I/O errors in error_code
template<class MatchTableT>
size_t Lzma2Compressor<MatchTableT>::Compress(const DataBlock& data_block,
	ThreadPool& threads,
	OutputStream& out_stream,
	ErrorCode& error_code,
	Progress* progress)
{
	assert(sizeof(size_t) > 4 || data_block.end < (UINT32_C(1) << 28));
	assert(data_block.end <= (UINT64_C(1) << 32));
	if (data_block.end <= data_block.start) {
		return 0;
	}
	out_stream.DisableExceptions();
	dictionary_max = std::max(dictionary_max, data_block.end);
	match_table.BuildTable(data_block, threads, progress);
	if (g_break) {
		return 0;
	}
#ifdef RADYX_STATS
	AsyncSubPerformanceCounter(total_time);
#endif
	size_t block_size = data_block.end - data_block.start;
	unsigned extra_thread_count = threads.GetCount();
	if ((block_size / (1 + extra_thread_count)) < kMinBytesPerThread) {
		extra_thread_count = static_cast<unsigned>(block_size / kMinBytesPerThread);
		extra_thread_count -= extra_thread_count != 0;
	}
	unsigned thread_count = 1 + extra_thread_count;
	if (encoders.size() < thread_count) {
		encoders = staticvec<EncoderArgs>(thread_count);
	}
	for (unsigned i = 0; i < thread_count; ++i) {
		encoders[i].start = data_block.start + i * block_size / thread_count;
		encoders[i].end = data_block.start + (i + 1) * block_size / thread_count;
	}
	if (encoders.front().end < data_block.end) {
		match_table.CreateDivision(encoders.front().end);
	}
	ThreadArgs args(*this, data_block, progress);
	for (unsigned i = 1; i < thread_count; ++i) {
		if (encoders[i].end < data_block.end) {
			match_table.CreateDivision(encoders[i].end);
		}
		threads[i - 1].SetWork(Lzma2Compressor<MatchTableT>::ThreadFn, &args, i);
	}
	AsyncWriter writer(out_stream, writer_thread);
	size_t total = encoders.front().encoder.Encode(match_table,
		data_block,
		encoders.front().start,
		encoders.front().end,
		options,
		needed_random_check,
		progress,
		&writer);
#ifdef RADYX_STATS
	AsyncAddPerformanceCounter(total_time);
#endif
	writer_thread.Join();
	if (writer.Fail()) {
		error_code = writer.GetErrorCode();
	}
	for (unsigned i = 1; i < thread_count; ++i) {
		threads[i - 1].Join();
		if (!g_break && !out_stream.Fail()) {
			out_stream.Write(match_table.GetOutputCharBuffer(encoders[i].start),
				encoders[i].encoded_size);
			if (!g_break && out_stream.Fail()) {
				error_code.LoadOsErrorCode();
				error_code.type = ErrorCode::kWrite;
			}
			total += encoders[i].encoded_size;
		}
	}
	needed_random_check = encoders[thread_count - 1].encoder.NeededRandomCheck();
	out_stream.RestoreExceptions();
	return total;
}

template<class MatchTableT>
size_t Lzma2Compressor<MatchTableT>::Finalize(OutputStream& out_stream)
{
	out_stream.Put(kChunkEof);
	needed_random_check = false;
	return 1;
}

template<class MatchTableT>
CoderInfo Lzma2Compressor<MatchTableT>::GetCoderInfo() const NOEXCEPT
{
	uint8_t dict_size_prop = Lzma2Encoder::GetDictSizeProp(dictionary_max);
	return CoderInfo(&dict_size_prop, 1, 0x21, 1, 1);
}

template<class MatchTableT>
size_t Lzma2Compressor<MatchTableT>::GetMemoryUsage(unsigned thread_count) const NOEXCEPT
{
	return match_table.GetMemoryUsage(thread_count) +
		Lzma2Encoder::GetMemoryUsage(options) * thread_count;
}


}

#endif // RADYX_LZMA2_COMPRESSOR_H