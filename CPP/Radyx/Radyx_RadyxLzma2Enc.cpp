///////////////////////////////////////////////////////////////////////////////
//
// Class: RadyxLzma2Enc
//        Radyx encoder class for 7-zip
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

#define NOMINMAX
#include "../Common/Common.h"

#include "../7zip/Common/StreamUtils.h"

#include "RadyxLzma2Enc.h"
#include "OutStream7z.h"

namespace Radyx {

volatile bool g_break;

HRESULT RadyxLzma2Enc::Create(Lzma2Options& params, unsigned numThreads)
{
	g_break = false;

	try {
		threads.reset(new ThreadPool(numThreads - 1));

		if (params.dictionary_size > PackedMatchTable::kMaxDictionary
			|| (params.fast_length > PackedMatchTable::kMaxLength)) {
			compressor.reset(new Lzma2Compressor<StructuredMatchTable>(params));
		}
		else {
			compressor.reset(new Lzma2Compressor<PackedMatchTable>(params));
		}
		unit_comp.reset(new UnitCompressor(compressor->GetDictionarySize(),
			compressor->GetMaxBufferOverrun(),
			(params.dictionary_size * params.block_overlap) >> Lzma2Options::kOverlapShift,
			false,
			false));
	}
	catch (std::bad_alloc&) {
		return SZ_ERROR_MEM;
	}
	catch (std::exception&) {
		return S_FALSE;
	}

	return S_OK;
}

void RadyxEncPropsNormalize(CLzmaEncProps *p)
{
	int level = p->level;
	if (level < 0) level = 5;
	p->level = level;

	if (p->dictSize != 0) {
		if (p->dictSize < Lzma2Encoder::GetUserDictionarySizeMin()) {
			p->dictSize = (UInt32)Lzma2Encoder::GetUserDictionarySizeMin();
		}
		else if (p->dictSize > Lzma2Encoder::GetDictionarySizeMax()) {
			p->dictSize = (UInt32)Lzma2Encoder::GetDictionarySizeMax();
		}
	}

	if (p->lc < 0) p->lc = 3;
	if (p->lp < 0) p->lp = 0;
	if (p->pb < 0) p->pb = 2;
}

void Lzma2EncPropsNormalize(CLzma2EncProps *p)
{
	if (p->numTotalThreads <= 0) {
		p->numTotalThreads = std::thread::hardware_concurrency();
	}

	if (p->blockSize == LZMA2_ENC_PROPS__BLOCK_SIZE__AUTO)
	{
		// blockSize is not an issue for radyx multithreading
		p->blockSize = LZMA2_ENC_PROPS__BLOCK_SIZE__SOLID;
	}

	if (p->blockSize != LZMA2_ENC_PROPS__BLOCK_SIZE__SOLID
		&& (p->blockSize < p->lzmaProps.reduceSize || p->lzmaProps.reduceSize == (UInt64)(Int64)-1))
	{
		p->lzmaProps.reduceSize = p->blockSize;
	}

	RadyxEncPropsNormalize(&p->lzmaProps);
}

STDMETHODIMP RadyxLzma2Enc::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
	const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
{
	in_processed = 0;
	HRESULT err = S_OK;
	try {
		OutStream7z out_stream(outStream);
		do
		{
			size_t inSize = unit_comp->GetAvailableSpace();
			err = ReadStream(inStream, unit_comp->GetAvailableBuffer(), &inSize);
			unit_comp->AddByteCount(inSize);
			in_processed += inSize;

			if (inSize) {
				unit_comp->Compress(*compressor, *threads, out_stream, nullptr);
				if (progress)
				{
					err = progress->SetRatioInfo(&in_processed, out_stream.GetProcessed());
					if (err != S_OK)
						break;
				}
				if (unit_comp->GetAvailableSpace() == 0) {
					unit_comp->Shift();
				}
				else {
					compressor->Finalize(out_stream);
					break;
				}
			}

		} while (err == S_OK);
	}
	catch (std::bad_alloc&) {
		err = SZ_ERROR_MEM;
	}
	catch (std::ios_base::failure&) {
		err = SZ_ERROR_WRITE;
	}
	catch (std::exception) {
		err = S_FALSE;
	}
	g_break = err != S_OK;
	unit_comp->Reset(false, false);
	return err;
}

CoderInfo RadyxLzma2Enc::GetCoderInfo() const NOEXCEPT
{
	if (compressor) {
		return compressor->GetCoderInfo();
	}
	return CoderInfo();
}

}