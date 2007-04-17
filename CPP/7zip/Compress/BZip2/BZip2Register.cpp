// Bzip2Register.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "BZip2Decoder.h"
static void *CreateCodec() { return (void *)(ICompressCoder *)(new NCompress::NBZip2::CDecoder); }
#if !defined(EXTRACT_ONLY) && !defined(DEFLATE_EXTRACT_ONLY)
#include "BZip2Encoder.h"
static void *CreateCodecOut() { return (void *)(ICompressCoder *)(new NCompress::NBZip2::CEncoder);  }
#else
#define CreateCodecOut 0
#endif

static CCodecInfo g_CodecInfo =
  { CreateCodec, CreateCodecOut, 0x040202, L"BZip2", 1, false };

REGISTER_CODEC(BZip2)
