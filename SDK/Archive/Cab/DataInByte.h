// Archive/Cab/DataInByte.h

#pragma once

#ifndef __ARCHIVE_CAB_DATAINBYTE_H
#define __ARCHIVE_CAB_DATAINBYTE_H

#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace NCab {

class CInByte
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
  CInByte(UINT32 aBufferSize = 0x20000);
  ~CInByte();
  
  void Init(ISequentialInStream *aStream, BYTE aReservedSize, UINT32 aNumBlocks);
  void ReleaseStream();

  bool ReadByte(BYTE &aByte)
    {
      if(m_Pos >= m_NumReadBytesInBuffer)
        return false;
      aByte = m_Buffer[m_Pos++];
      return true;
    }
  BYTE ReadByte()
    {
      if(m_Pos >= m_NumReadBytesInBuffer)
        return 0;
      return m_Buffer[m_Pos++];
    }
  /*
    void ReadBytes(void *aData, UINT32 aSize, UINT32 &aProcessedSize)
    {
      BYTE *aDataPointer = (BYTE *)aData;
      for(int i = 0; i < aSize; i++)
        if (!ReadByte(aDataPointer[i]))
          break;
      aProcessedSize = i;
    }
  */
  HRESULT ReadBlock(UINT32 &anUncompressedSize, bool &aDataAreCorrect);
  UINT64 GetProcessedSize() const { return m_ProcessedSize + m_Pos; }
};

}}

#endif
