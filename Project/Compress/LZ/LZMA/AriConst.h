// AriConst.h

#pragma once

#ifndef __ARICONST_H
#define __ARICONST_H

#include "Compression/AriBitCoder.h"

typedef NCompression::NArithmetic::CRangeEncoder CMyRangeEncoder;
typedef NCompression::NArithmetic::CRangeDecoder CMyRangeDecoder;

template <int numMoveBits> class CMyBitEncoder: 
    public NCompression::NArithmetic::CBitEncoder<numMoveBits> {};
template <int numMoveBits> class CMyBitDecoder: 
    public NCompression::NArithmetic::CBitDecoder<numMoveBits> {};

#endif



