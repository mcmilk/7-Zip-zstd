// LZMA.cpp

#include "StdAfx.h"

#include "LZMA.h"

namespace NCompress {
namespace NLZMA {

UINT32 kDistStart[kDistTableSizeMax];

static class CConstInit
{
public:
  CConstInit()
  {
    UINT32 aStartValue = 0;
    int i;
    for (i = 0; i < kDistTableSizeMax; i++)
    {
      kDistStart[i] = aStartValue;
      aStartValue += (1 << kDistDirectBits[i]);
    }
  }
} g_ConstInit;

}}
