// BranchRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "x86_2.h"
static void *CreateCodec() { return (void *)(ICompressCoder2 *)(new NCompress::NBcj2::CDecoder()); }
#ifndef EXTRACT_ONLY
static void *CreateCodecOut() { return (void *)(ICompressCoder2 *)(new NCompress::NBcj2::CEncoder());  }
#else
#define CreateCodecOut 0
#endif

static CCodecInfo g_CodecInfo =
  { CreateCodec, CreateCodecOut, 0x0303011B, L"BCJ2", 4, false };

REGISTER_CODEC(BCJ2)
