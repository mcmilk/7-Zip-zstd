// Lzma2Register.cpp

#include "StdAfx.h"

#include "../Common/RegisterCodec.h"

#include "Lzma2Decoder.h"

#ifndef EXTRACT_ONLY
#include "Lzma2Encoder.h"
#endif

namespace NCompress {
namespace NLzma2 {

REGISTER_CODEC_E(LZMA2,
  CDecoder(),
  CEncoder(),
  0x21,
  "LZMA2")
}

namespace NFLzma2 {

REGISTER_CODEC_E(FLZMA2,
  NCompress::NLzma2::CDecoder(),
  NCompress::NLzma2::CFastEncoder(),
  0x21,
  "FLZMA2")
}

}
