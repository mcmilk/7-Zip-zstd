// Stream/LSBFEncoder.h

#pragma once

#ifndef __STREAM_LSBFENCODER_H
#define __STREAM_LSBFENCODER_H

#include "Interface/IInOutStreams.h"
#include "Stream/OutByte.h"

namespace NStream {
namespace NLSBF {

class CEncoder
{
  COutByte m_Stream;
  UINT32 m_BitPos;
  BYTE m_CurByte;
public:
  void Init(ISequentialOutStream *aStream)
  {
    m_Stream.Init(aStream);
    m_BitPos = 8; 
    m_CurByte = 0;
  }
  HRESULT Flush()
  {
    if(m_BitPos < 8)
      WriteBits(0, m_BitPos);
    return m_Stream.Flush();
  }
  void ReleaseStream()
  {
    m_Stream.ReleaseStream();
  }


  void WriteBits(UINT32 aValue, UINT32 aNumBits);
  
  UINT32 GetBitPosition() const
    {  return (8 - m_BitPos); }

  UINT64 GetProcessedSize() const { 
      return m_Stream.GetProcessedSize() + (8 - m_BitPos + 7) /8; }
};

class CReverseEncoder
{
  CEncoder *m_Encoder;
public:
  void Init(CEncoder *anEncoder)
    { m_Encoder = anEncoder; }
  void WriteBits(UINT32 aValue, UINT32 aNumBits);
};

}}

#endif
