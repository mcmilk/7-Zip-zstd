// RadyxEncoder.cpp

#include "StdAfx.h"
#include "RadyxEncoder.h"

#include "../../Radyx/Lzma2Compressor.h"
#include "../../Radyx/LzmaStates.h"
#include "../../Radyx/CoderInfo.h"

#include "../../../C/Lzma2Enc.h"
#include "../Common/StreamUtils.h"

namespace NCompress {

namespace NLzma2 {

HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props);

}

namespace NRadyx
{

CEncoder::CEncoder()
	: num_threads(1)
{
}

CEncoder::~CEncoder()
{
}

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs,
	const PROPVARIANT *coderProps, UInt32 numProps)
{
	CLzma2EncProps lzma2Props;
	Lzma2EncProps_Init(&lzma2Props);

	for (UInt32 i = 0; i < numProps; i++)
	{
		RINOK(NLzma2::SetLzma2Prop(propIDs[i], coderProps[i], lzma2Props));
	}

	Radyx::Lzma2EncPropsNormalize(&lzma2Props);

	if (lzma2Props.lzmaProps.lc + lzma2Props.lzmaProps.lp > Radyx::LzmaStates::kLcLpMax)
		return SZ_ERROR_PARAM;

	num_threads = lzma2Props.numTotalThreads;

	params.lc = lzma2Props.lzmaProps.lc;
	params.lp = lzma2Props.lzmaProps.lp;
	params.pb = lzma2Props.lzmaProps.pb;
	if (lzma2Props.lzmaProps.fb > 0) {
		params.fast_length = lzma2Props.lzmaProps.fb;
	}
	if (lzma2Props.lzmaProps.mc > 0) {
		params.match_cycles = lzma2Props.lzmaProps.mc;
	}
	if (lzma2Props.lzmaProps.algo >= 0) {
		params.encoder_mode = lzma2Props.lzmaProps.algo > 0 ? Radyx::Lzma2Options::kBestMode : Radyx::Lzma2Options::kFastMode;
	}
	params.compress_level = lzma2Props.lzmaProps.level;
	if (lzma2Props.lzmaProps.dictSize > 0) {
		params.dictionary_size = lzma2Props.lzmaProps.dictSize;
	}

	params.LoadCompressLevel();
	
	if (params.dictionary_size > lzma2Props.lzmaProps.reduceSize)
	{
		unsigned i;
		UInt32 reduceSize = (UInt32)lzma2Props.lzmaProps.reduceSize;
		for (i = Radyx::Lzma2Encoder::GetDictionaryBitsMin() - 1; i <= 30; i++)
		{
			if (reduceSize <= ((UInt32)2 << i)) { params.dictionary_size = ((UInt32)2 << i); break; }
			if (reduceSize <= ((UInt32)3 << i)) { params.dictionary_size = ((UInt32)3 << i); break; }
		}
	}

	return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{
	uint8_t dict_size_prop = Radyx::Lzma2Encoder::GetDictSizeProp(params.dictionary_size);
	return WriteStream(outStream, &dict_size_prop, 1);
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
	const UInt64 * inSize, const UInt64 * outSize, ICompressProgressInfo *progress)
{
	if (!encoder) {
		RINOK(encoder.Create(params, num_threads));
	}

	return encoder.Code(inStream, outStream, inSize, outSize, progress);
}

} // NRadyx

} // NCompress
