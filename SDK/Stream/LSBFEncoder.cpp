// Stream/LSBFEncoder.cpp

#include "StdAfx.h"

#include "Stream/LSBFEncoder.h"
#include "Common/Defs.h"

namespace NStream {
namespace NLSBF {

void CEncoder::WriteBits(UINT32 aValue, UINT32 aNumBits)
{
  while(aNumBits > 0)
  {
    UINT32 aNumNewBits = MyMin(aNumBits, m_BitPos);
    aNumBits -= aNumNewBits;

    UINT32 aMask = (1 << aNumNewBits) - 1;
    m_CurByte |= (aValue & aMask) << (8 - m_BitPos);
    aValue >>= aNumNewBits;

    m_BitPos -= aNumNewBits;

    if (m_BitPos == 0)
    {
      m_Stream.WriteByte(m_CurByte);
      m_BitPos = 8;
      m_CurByte = 0;
    }
  }
}


void CReverseEncoder::WriteBits(UINT32 aValue, UINT32 aNumBits)
{
  UINT32 aReverseValue = 0;
  for(UINT32 i = 0; i < aNumBits; i++) 
  {
    aReverseValue <<= 1;
    aReverseValue |= aValue & 1;
    aValue >>= 1;
  } 
  m_Encoder->WriteBits(aReverseValue, aNumBits);
}

}}
