// Archive/Cab/LZXi86Converter.cpp

#include "StdAfx.h"

#include "Common/Defs.h"

#include "LZXi86Converter.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

static const int kResidue = 6 + 4;

void Ci86TranslationOutStream::MakeTranslation()
{
  if (m_Pos <= kResidue)
    return;
  UInt32 numBytes = m_Pos - kResidue;
  for (UInt32 i = 0; i < numBytes;)
  {
    if (m_Buffer[i] == 0xE8)
    {
      i++;
      Int32 absValue = 0;
      int j;
      for(j = 0; j < 4; j++)
        absValue += UInt32(m_Buffer[i + j]) << (j * 8);

      Int32 pos = m_ProcessedSize + i - 1;
      UInt32 offset;
      if (absValue < -pos || absValue >= Int32(m_TranslationSize))
      {
      }
      else
      {
        offset = (absValue >= 0) ? 
            absValue - pos :
            absValue + m_TranslationSize;
        for(j = 0; j < 4; j++)
        {
          m_Buffer[i + j] = Byte(offset & 0xFF);
          offset >>= 8;
        }
      }
      i += 4;
    }
    else
      i++;
  }
}

STDMETHODIMP Ci86TranslationOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
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

STDMETHODIMP Ci86TranslationOutStream::WritePart(const void *data, UInt32 size, UInt32 *processedSize)
{
  return Write(data, size, processedSize);
}

HRESULT Ci86TranslationOutStream::Flush()
{
  if (m_Pos == 0)
    return S_OK;
  MakeTranslation();
  RINOK(m_Stream->Write(m_Buffer, m_Pos, NULL));
  m_ProcessedSize += m_Pos;
  m_Pos = 0;
  m_TranslationMode = (m_ProcessedSize < (1 << 30));
  return S_OK;
}

}}}
