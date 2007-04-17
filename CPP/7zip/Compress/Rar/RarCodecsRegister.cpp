// RarCodecsRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "Rar1Decoder.h"
#include "Rar2Decoder.h"
#include "Rar3Decoder.h"

#define CREATE_CODEC(x) static void *CreateCodec ## x() { return (void *)(ICompressCoder *)(new NCompress::NRar ## x::CDecoder); }

CREATE_CODEC(1)
CREATE_CODEC(2)
CREATE_CODEC(3)

#define RAR_CODEC(x, name) { CreateCodec ## x, 0, 0x040300 + x, L"Rar" name, 1, false }

static CCodecInfo g_CodecsInfo[] =
{
  RAR_CODEC(1, L"1"),
  RAR_CODEC(2, L"2"),
  RAR_CODEC(3, L"3"),
};

REGISTER_CODECS(Rar)
