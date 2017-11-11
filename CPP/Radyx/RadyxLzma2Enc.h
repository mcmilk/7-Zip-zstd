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

#ifndef __RADYX_LZMA2ENC_H
#define __RADYX_LZMA2ENC_H

#include "Lzma2Compressor.h"
#include "UnitCompressor.h"
#include "../7zip/ICoder.h"
#include "../../C/Lzma2Enc.h"

namespace Radyx {

void Lzma2EncPropsNormalize(CLzma2EncProps *p);

class RadyxLzma2Enc
{
public:
	HRESULT Create(Lzma2Options& params, unsigned numThreads);
	HRESULT Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
		const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
	CoderInfo GetCoderInfo() const noexcept;
	operator bool() {
		return compressor.operator bool();
	}
private:
    std::unique_ptr<CompressorInterface> compressor;
    std::unique_ptr<UnitCompressor> unit_comp;
	std::unique_ptr<ThreadPool> threads;
	UInt64 in_processed;
};

}
#endif