// Archive/CabInBuffer.h

#pragma once

#ifndef __ARCHIVE_CAB_INBUFFER_H
#define __ARCHIVE_CAB_INBUFFER_H

#include "../../IStream.h"

namespace NArchive {
namespace NCab {

class CInBuffer
{
  UINT64 m_ProcessedSize;
  UINT32 m_Pos;
  UINT32 m_NumReadBytesInBuffer;
  BYTE *m_Buffer;
  ISequentialInStream *m_Stream;
  UINT32 m_BufferSize;
  
  UINT32 m_NumBlocks;
  UINT32 m_CurrentBlockIndex;
  UINT32 m_ReservedSize;

public:
  CInBuffer(UINT32 bufferSize = 0x20000);
  ~CInBuffer();
  
  void Init(ISequentialInStream *inStream, BYTE reservedSize, UINT32 numBlocks);
  // void ReleaseStream();

  bool ReadByte(BYTE &b)
    {
      if(m_Pos >= m_NumReadBytesInBuffer)
        return false;
      b = m_Buffer[m_Pos++];
      return true;
    }
  BYTE ReadByte()
    {
      if(m_Pos >= m_NumReadBytesInBuffer)
        return 0;
      return m_Buffer[m_Pos++];
    }
  /*
    void ReadBytes(void *data, UINT32 size, UINT32 &aProcessedSize)
    {
      BYTE *aDataPointer = (BYTE *)data;
      for(int i = 0; i < size; i++)
        if (!ReadByte(aDataPointer[i]))
          break;
      aProcessedSize = i;
    }
  */
  HRESULT ReadBlock(UINT32 &uncompressedSize, bool &dataAreCorrect);
  UINT64 GetProcessedSize() const { return m_ProcessedSize + m_Pos; }
};

}}

#endif
