// Archive/CabInBuffer.h

#ifndef __ARCHIVE_CAB_INBUFFER_H
#define __ARCHIVE_CAB_INBUFFER_H

#include "../../IStream.h"
#include "../../../Common/MyCom.h"

namespace NArchive {
namespace NCab {

class CInBuffer
{
  UInt64 m_ProcessedSize;
  UInt32 m_Pos;
  UInt32 m_NumReadBytesInBuffer;
  Byte *m_Buffer;
  CMyComPtr<ISequentialInStream> m_Stream;
  UInt32 m_BufferSize;
  
  UInt32 m_NumBlocks;
  UInt32 m_CurrentBlockIndex;
  UInt32 m_ReservedSize;

public:
  CInBuffer(): m_Stream(0) {}
  ~CInBuffer() { Free(); }
  bool Create(UInt32 bufferSize);
  void Free();
  
  void SetStream(ISequentialInStream *inStream) { m_Stream = inStream; }
  void ReleaseStream() { m_Stream.Release(); }

  void Init(Byte reservedSize, UInt32 numBlocks);
  void Init() {}

  bool ReadByte(Byte &b)
    {
      if(m_Pos >= m_NumReadBytesInBuffer)
        return false;
      b = m_Buffer[m_Pos++];
      return true;
    }
  Byte ReadByte()
    {
      if(m_Pos >= m_NumReadBytesInBuffer)
        return 0;
      return m_Buffer[m_Pos++];
    }
  /*
    void ReadBytes(void *data, UInt32 size, UInt32 &aProcessedSize)
    {
      Byte *aDataPointer = (Byte *)data;
      for(int i = 0; i < size; i++)
        if (!ReadByte(aDataPointer[i]))
          break;
      aProcessedSize = i;
    }
  */
  HRESULT ReadBlock(UInt32 &uncompressedSize, bool &dataAreCorrect);
  UInt64 GetProcessedSize() const { return m_ProcessedSize + m_Pos; }
};

}}

#endif
