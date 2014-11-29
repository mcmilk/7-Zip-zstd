// DeflateRegister.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "DeflateDecoder.h"

static void *CreateCodecDeflate() { return (void *)(ICompressCoder *)(new NCompress::NDeflate::NDecoder::CCOMCoder); }

#if !defined(EXTRACT_ONLY) && !defined(DEFLATE_EXTRACT_ONLY)
#include "DeflateEncoder.h"
static void *CreateCodecOutDeflate() { return (void *)(ICompressCoder *)(new NCompress::NDeflate::NEncoder::CCOMCoder);  }
#else
#define CreateCodecOutDeflate 0
#endif

static CCodecInfo g_CodecInfo =
  { CreateCodecDeflate,   CreateCodecOutDeflate,   0x040108, L"Deflate", 1, false };

REGISTER_CODEC(Deflate)
