// Stream/OutByte.h

#pragma once

#ifndef __STREAM_OUTBYTE_H
#define __STREAM_OUTBYTE_H

#include "Interface/IInOutStreams.h"
#include "Common/Types.h"

namespace NStream {

class COutByteWriteException
{
public:
  HRESULT m_Result;
  COutByteWriteException(HRESULT aResult): m_Result (aResult) {}
};

class COutByte
{
  BYTE *m_Buffer;
  UINT32 m_Pos;
  UINT32 m_BufferSize;
  CComPtr<ISequentialOutStream> m_Stream;
  UINT64 m_ProcessedSize;

  void WriteBlock();
public:
  COutByte(UINT32 aBufferSize = (1 << 20));
  ~COutByte();

  void Init(ISequentialOutStream *aStream);
  HRESULT Flush();
  void ReleaseStream();

  void WriteByte(BYTE aByte)
  {
    m_Buffer[m_Pos++] = aByte;
    if(m_Pos >= m_BufferSize)
      WriteBlock();
  }
  void WriteBytes(const void *aBytes, UINT32 aSize)
  {
    for (UINT32 i = 0; i < aSize; i++)
      WriteByte(((const BYTE *)aBytes)[i]);
  }

  UINT64 GetProcessedSize() const { return m_ProcessedSize + m_Pos; }
};

}

#endif
