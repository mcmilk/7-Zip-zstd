// Stream/InByte.h

#pragma once

#ifndef __STREAM_INBYTE_H
#define __STREAM_INBYTE_H

#include "Interface/IInOutStreams.h"

namespace NStream {

class CInByteReadException
{
public:
  HRESULT m_Result;
  CInByteReadException(HRESULT aResult): m_Result (aResult) {}
};

class CInByte
{
  UINT64 m_ProcessedSize;
  BYTE *m_BufferBase;
  UINT32 m_BufferSize;
  BYTE *m_Buffer;
  BYTE *m_BufferLimit;
  CComPtr<ISequentialInStream> m_Stream;
  bool m_StreamWasExhausted;

  bool ReadBlock();

public:
  CInByte(UINT32 aBufferSize = 0x100000);
  ~CInByte();
  
  void Init(ISequentialInStream *aStream);
  void ReleaseStream()
    { m_Stream.Release(); }


  bool ReadByte(BYTE &aByte)
    {
      if(m_Buffer >= m_BufferLimit)
        if(!ReadBlock())
          return false;
      aByte = *m_Buffer++;
      return true;
    }
  BYTE ReadByte()
    {
      if(m_Buffer >= m_BufferLimit)
        if(!ReadBlock())
          return 0x0;
      return *m_Buffer++;
    }
  void ReadBytes(void *aData, UINT32 aSize, UINT32 &aProcessedSize)
    {
      for(aProcessedSize = 0; aProcessedSize < aSize; aProcessedSize++)
        if (!ReadByte(((BYTE *)aData)[aProcessedSize]))
          return;
    }
  bool ReadBytes(void *aData, UINT32 aSize)
    {
      UINT32 aProcessedSize;
      ReadBytes(aData, aSize, aProcessedSize);
      return (aProcessedSize == aSize);
    }
  UINT64 GetProcessedSize() const { return m_ProcessedSize + (m_Buffer - m_BufferBase); }
};

}

#endif
