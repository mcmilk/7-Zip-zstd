// RadyxEncoder.h

#ifndef __RADYX_ENCODER_H
#define __RADYX_ENCODER_H

#include "../../Radyx/RadyxLzma2Enc.h"
#include "../../Radyx/Lzma2Options.h"

#include "../../Common/MyCom.h"
#include "../ICoder.h"

namespace NCompress {
	namespace NRadyx {

		class CEncoder :
			public ICompressCoder,
			public ICompressSetCoderProperties,
			public ICompressWriteCoderProperties,
			public CMyUnknownImp
		{
		private:
			Radyx::RadyxLzma2Enc encoder;
			Radyx::Lzma2Options params;
			unsigned num_threads;

		public:
			MY_UNKNOWN_IMP3(
				ICompressCoder,
				ICompressSetCoderProperties,
				ICompressWriteCoderProperties)

			STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
					const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
			STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

			CEncoder();
			virtual ~CEncoder();
		};

	}
}

#endif
