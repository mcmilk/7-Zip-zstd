// MyAesReg.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "MyAes.h"

static void *CreateCodecCbc() { return (void *)(ICompressFilter *)(new NCrypto::CAesCbcDecoder(32)); }
#ifndef EXTRACT_ONLY
static void *CreateCodecCbcOut() { return (void *)(ICompressFilter *)(new NCrypto::CAesCbcEncoder(32)); }
#else
#define CreateCodecCbcOut 0
#endif

static CCodecInfo g_CodecInfo =
  { CreateCodecCbc, CreateCodecCbcOut, 0x06F00181, L"AES256CBC", 1, true };
REGISTER_CODEC(AES256CBC)

