// RadyxRegister.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"
#include "RadyxEncoder.h"
#include "Lzma2Decoder.h"

namespace NCompress {

namespace NRadyx {

REGISTER_CODEC_E(RADYX,
	NCompress::NLzma2::CDecoder,
	CEncoder(),
	0x4F711FF,
	"RADYX")

}

}
