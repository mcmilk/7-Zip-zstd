// ByteSwapRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "ByteSwap.h"
static void *CreateCodec2() { return (void *)(ICompressFilter *)(new CByteSwap2); }
static void *CreateCodec4() { return (void *)(ICompressFilter *)(new CByteSwap4); }

static CCodecInfo g_CodecsInfo[] =
{
  { CreateCodec2, CreateCodec4, 0x020302, L"Swap2", 1, true },
  { CreateCodec4, CreateCodec4, 0x020304, L"Swap4", 1, true }
};

REGISTER_CODECS(ByteSwap)
