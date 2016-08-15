// ZstdRegister.cpp
// (C) 2016 Rich Geldreich, Tino Reichardt

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "ZstdDecoder.h"

#ifndef EXTRACT_ONLY
#include "ZstdEncoder.h"
#endif

#ifndef EXTERNAL_CODEC
REGISTER_CODEC_E(
  ZSTD,
  NCompress::NZSTD::CDecoder(),
  NCompress::NZSTD::CEncoder(),
  0x4F71101, "ZSTD")
#else
static void *CreateCodecOut() { return (void *)(ICompressCoder *)(new NCompress::NZSTD::CEncoder); }
static void *CreateCodec() { return (void *)(ICompressCoder *)(new NCompress::NZSTD::CDecoder); }
static CCodecInfo g_CodecsInfo[1] = {
  CreateCodec,
  CreateCodecOut,
  0x4F71101,
  "ZSTD",
  1,
  false
};
REGISTER_CODECS(ZSTD)
#endif
