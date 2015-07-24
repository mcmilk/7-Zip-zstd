// DeflateRegister.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "DeflateDecoder.h"

REGISTER_CODEC_CREATE(CreateDec, NCompress::NDeflate::NDecoder::CCOMCoder)

#if !defined(EXTRACT_ONLY) && !defined(DEFLATE_EXTRACT_ONLY)
#include "DeflateEncoder.h"
REGISTER_CODEC_CREATE(CreateEnc, NCompress::NDeflate::NEncoder::CCOMCoder)
#else
#define CreateEnc NULL
#endif

REGISTER_CODEC_2(Deflate, CreateDec, CreateEnc, 0x40108, "Deflate")
