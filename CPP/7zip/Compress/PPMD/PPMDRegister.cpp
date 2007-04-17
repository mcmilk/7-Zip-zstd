// PPMDRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "PPMDDecoder.h"
static void *CreateCodec() { return (void *)(ICompressCoder *)(new NCompress::NPPMD::CDecoder); }
#ifndef EXTRACT_ONLY
#include "PPMDEncoder.h"
static void *CreateCodecOut() { return (void *)(ICompressCoder *)(new NCompress::NPPMD::CEncoder);  }
#else
#define CreateCodecOut 0
#endif

static CCodecInfo g_CodecInfo =
  { CreateCodec, CreateCodecOut, 0x030401, L"PPMD", 1, false };

REGISTER_CODEC(PPMD)
