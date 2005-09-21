// Lzx86Converter.cpp

#include "StdAfx.h"

#include "Common/Defs.h"

#include "Lzx86Converter.h"

namespace NCompress {
namespace NLzx {

static const int kResidue = 6 + 4;

void Cx86ConvertOutStream::MakeTranslation()
{
  if (m_Pos <= kResidue)
    return;
  UInt32 numBytes = m_Pos - kResidue;
  Byte *buffer = m_Buffer;
  for (UInt32 i = 0; i < numBytes;)
  {
    if (buffer[i++] == 0xE8)
    {
      Int32 absValue = 0;
      int j;
      for(j = 0; j < 4; j++)
        absValue += (UInt32)buffer[i + j] << (j * 8);
      Int32 pos = (Int32)(m_ProcessedSize + i - 1);
      if (absValue >= -pos && absValue < (Int32)m_TranslationSize)
      {
        UInt32 offset = (absValue >= 0) ? 
            absValue - pos :
            absValue + m_TranslationSize;
        for(j = 0; j < 4; j++)
        {
          buffer[i + j] = (Byte)(offset & 0xFF);
          offset >>= 8;
        }
      }
      i += 4;
    }
  }
}

STDMETHODIMP Cx86ConvertOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize != NULL)
    *processedSize = 0;
  if (!m_TranslationMode)
    return m_Stream->Write(data, size, processedSize);
  UInt32 realProcessedSize = 0;
  while (realProcessedSize < size)
  {
    UInt32 writeSize = MyMin(size - realProcessedSize, kUncompressedBlockSize - m_Pos);
    memmove(m_Buffer + m_Pos, (const Byte *)data + realProcessedSize, writeSize);
    m_Pos += writeSize;
    realProcessedSize += writeSize;
    if (m_Pos == kUncompressedBlockSize)
    {
      RINOK(Flush());
    }
  }
  if (processedSize != NULL)
    *processedSize = realProcessedSize;
  return S_OK;
}

HRESULT Cx86ConvertOutStream::Flush()
{
  if (m_Pos == 0)
    return S_OK;
  if (m_TranslationMode)
    MakeTranslation();
  UInt32 pos = 0;
  do
  {
    UInt32 processed;
    RINOK(m_Stream->Write(m_Buffer + pos, m_Pos - pos, &processed));
    if (processed == 0)
      return E_FAIL;
    pos += processed;
  }
  while(pos < m_Pos);
  m_ProcessedSize += m_Pos;
  m_Pos = 0;
  m_TranslationMode = (m_TranslationMode && (m_ProcessedSize < (1 << 30)));
  return S_OK;
}

}}
