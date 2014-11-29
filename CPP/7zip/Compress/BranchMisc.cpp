// BranchMisc.cpp

#include "StdAfx.h"

#include "../../../C/Bra.h"

#include "BranchMisc.h"

#define SUB_FILTER_IMP2(name, coderStr, coderNum) \
  UInt32 CBC_ ## name ## coderStr::SubFilter(Byte *data, UInt32 size) \
  { return (UInt32)::name ## Convert(data, size, _bufferPos, coderNum); }

#define SUB_FILTER_IMP(name) \
  SUB_FILTER_IMP2(name, Encoder, 1) \
  SUB_FILTER_IMP2(name, Decoder, 0) \

SUB_FILTER_IMP(ARM_)
SUB_FILTER_IMP(ARMT_)
SUB_FILTER_IMP(PPC_)
SUB_FILTER_IMP(SPARC_)
SUB_FILTER_IMP(IA64_)
