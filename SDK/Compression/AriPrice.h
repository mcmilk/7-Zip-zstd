// Compression/AriPrice.h

#pragma once

#ifndef __COMPRESSION_ARIPRICE_H
#define __COMPRESSION_ARIPRICE_H

#include "Common/Defs.h"

namespace NCompression {
namespace NArithmetic {

const UINT32 kNumBitPriceShiftBits = 6;
const UINT32 kBitPrice = 1 << kNumBitPriceShiftBits;

}}

#endif
