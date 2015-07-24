// BZip2Register.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "BZip2Decoder.h"

REGISTER_CODEC_CREATE(CreateDec, NCompress::NBZip2::CDecoder)

#if !defined(EXTRACT_ONLY) && !defined(BZIP2_EXTRACT_ONLY)
#include "BZip2Encoder.h"
REGISTER_CODEC_CREATE(CreateEnc, NCompress::NBZip2::CEncoder)
#else
#define CreateEnc NULL
#endif

REGISTER_CODEC_2(BZip2, CreateDec, CreateEnc, 0x40202, "BZip2")
