// Stream/LSBFDecoder.cpp

#include "StdAfx.h"

#include "Stream/LSBFDecoder.h"

namespace NStream {
namespace NLSBF {

BYTE kInvertTable[256];

class CInverterTableInitializer
{
public:
  CInverterTableInitializer()
  {
    for(int i = 0; i < 256; i++)
    {
      BYTE aByte = BYTE(i);
      BYTE aByteInvert = 0;
      for(int j = 0; j < 8; j++)
      {
        aByteInvert <<= 1;
        if (aByte & 1)
          aByteInvert |= 1;
        aByte >>= 1;
      }
      kInvertTable[i] = aByteInvert;
    }
  }
} g_InverterTableInitializer;


}}
