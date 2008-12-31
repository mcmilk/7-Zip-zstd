// PpmdRegister.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "PpmdDecoder.h"

static void *CreateCodec() { return (void *)(ICompressCoder *)(new NCompress::NPpmd::CDecoder); }
#ifndef EXTRACT_ONLY
#include "PpmdEncoder.h"
static void *CreateCodecOut() { return (void *)(ICompressCoder *)(new NCompress::NPpmd::CEncoder);  }
#else
#define CreateCodecOut 0
#endif

static CCodecInfo g_CodecInfo =
  { CreateCodec, CreateCodecOut, 0x030401, L"PPMD", 1, false };

REGISTER_CODEC(PPMD)
