// LSBFEncoder.cpp

#include "StdAfx.h"

#include "LSBFEncoder.h"
#include "Common/Defs.h"

namespace NStream {
namespace NLSBF {

void CEncoder::WriteBits(UInt32 value, UInt32 numBits)
{
  while(numBits > 0)
  {
    UInt32 numNewBits = MyMin(numBits, m_BitPos);
    numBits -= numNewBits;

    UInt32 mask = (1 << numNewBits) - 1;
    m_CurByte |= (value & mask) << (8 - m_BitPos);
    value >>= numNewBits;

    m_BitPos -= numNewBits;

    if (m_BitPos == 0)
    {
      m_Stream.WriteByte(m_CurByte);
      m_BitPos = 8;
      m_CurByte = 0;
    }
  }
}


void CReverseEncoder::WriteBits(UInt32 value, UInt32 numBits)
{
  UInt32 reverseValue = 0;
  for(UInt32 i = 0; i < numBits; i++) 
  {
    reverseValue <<= 1;
    reverseValue |= value & 1;
    value >>= 1;
  } 
  m_Encoder->WriteBits(reverseValue, numBits);
}

}}
