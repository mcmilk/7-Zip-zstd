// Stream/LSBFEncoder.h

#ifndef __STREAM_LSBFENCODER_H
#define __STREAM_LSBFENCODER_H

#include "../IStream.h"
#include "OutBuffer.h"

namespace NStream {
namespace NLSBF {

class CEncoder
{
  COutBuffer m_Stream;
  UInt32 m_BitPos;
  Byte m_CurByte;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialOutStream *outStream) { m_Stream.SetStream(outStream); }
  void ReleaseStream() { m_Stream.ReleaseStream(); }
  void Init()
  {
    m_Stream.Init();
    m_BitPos = 8; 
    m_CurByte = 0;
  }
  HRESULT Flush()
  {
    if(m_BitPos < 8)
      WriteBits(0, m_BitPos);
    return m_Stream.Flush();
  }
  void WriteBits(UInt32 value, UInt32 numBits);
  UInt32 GetBitPosition() const {  return (8 - m_BitPos); }
  UInt64 GetProcessedSize() const { 
      return m_Stream.GetProcessedSize() + (8 - m_BitPos + 7) /8; }
};

class CReverseEncoder
{
  CEncoder *m_Encoder;
public:
  void Init(CEncoder *encoder) { m_Encoder = encoder; }
  void WriteBits(UInt32 value, UInt32 numBits);
};

}}

#endif
