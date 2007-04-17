// Deflate64Register.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "DeflateDecoder.h"
static void *CreateCodecDeflate64() { return (void *)(ICompressCoder *)(new NCompress::NDeflate::NDecoder::CCOMCoder64); }
#if !defined(EXTRACT_ONLY) && !defined(DEFLATE_EXTRACT_ONLY)
#include "DeflateEncoder.h"
static void *CreateCodecOutDeflate64() { return (void *)(ICompressCoder *)(new NCompress::NDeflate::NEncoder::CCOMCoder64);  }
#else
#define CreateCodecOutDeflate64 0
#endif

static CCodecInfo g_CodecInfo =
  { CreateCodecDeflate64, CreateCodecOutDeflate64, 0x040109, L"Deflate64", 1, false };

REGISTER_CODEC(Deflate64)
