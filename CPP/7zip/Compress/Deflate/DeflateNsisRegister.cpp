// DeflateNsisRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "DeflateDecoder.h"
static void *CreateCodecDeflateNsis() { return (void *)(ICompressCoder *)(new NCompress::NDeflate::NDecoder::CNsisCOMCoder); }

static CCodecInfo g_CodecInfo =
  { CreateCodecDeflateNsis, 0,   0x040901, L"DeflateNSIS", 1, false };

REGISTER_CODEC(DeflateNsis)
