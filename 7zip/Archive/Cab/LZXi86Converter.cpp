// Archive/Cab/LZXi86Converter.cpp

#include "StdAfx.h"

#include "Common/Defs.h"

#include "LZXi86Converter.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

Ci86TranslationOutStream::Ci86TranslationOutStream():
  m_Pos(0)
{
}

Ci86TranslationOutStream::~Ci86TranslationOutStream()
{
  Flush();
}

void Ci86TranslationOutStream::Init(ISequentialOutStream *aStream, 
    bool aTranslationMode, UINT32 aTranslationSize)
{
  m_Stream = aStream;
  m_TranslationMode = aTranslationMode;
  m_TranslationSize = aTranslationSize;
  m_ProcessedSize = 0;
  m_Pos = 0;
}

void Ci86TranslationOutStream::ReleaseStream()
{
  m_Stream.Release();
}

inline INT32 Ci86TranslationOutStream::ConvertAbsoluteToOffset(INT32 aPos, INT32 anAbsoluteValue)
{
}

static const kResidue = 6 + 4;

void Ci86TranslationOutStream::MakeTranslation()
{
  if (m_Pos <= kResidue)
    return;
  UINT32 aNumBytes = m_Pos - kResidue;
  for (UINT32 i = 0; i < aNumBytes;)
  {
    if (m_Buffer[i] == 0xE8)
    {
      i++;
      INT32 anAbsoluteValue = 0;
      for(int j = 0; j < 4; j++)
        anAbsoluteValue += UINT32(m_Buffer[i + j]) << (j * 8);

      INT32 aPos = m_ProcessedSize + i - 1;
      UINT32 anOffset;
      if (anAbsoluteValue < -aPos || anAbsoluteValue >= INT32(m_TranslationSize))
      {
      }
      else
      {
        anOffset = (anAbsoluteValue >= 0) ? 
            anAbsoluteValue - aPos :
            anAbsoluteValue + m_TranslationSize;
        for(j = 0; j < 4; j++)
        {
          m_Buffer[i + j] = BYTE(anOffset & 0xFF);
          anOffset >>= 8;
        }
      }
      i += 4;
    }
    else
      i++;
  }
}

STDMETHODIMP Ci86TranslationOutStream::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  if (!m_TranslationMode)
    return m_Stream->Write(aData, aSize, aProcessedSize);

  UINT32 aProcessedSizeReal = 0;

  while (aProcessedSizeReal < aSize)
  {
    UINT32 aWriteSize = MyMin(aSize - aProcessedSizeReal, kUncompressedBlockSize - m_Pos);
    memmove(m_Buffer + m_Pos, (const BYTE *)aData + aProcessedSizeReal, aWriteSize);
    m_Pos += aWriteSize;
    aProcessedSizeReal += aWriteSize;
    if (m_Pos == kUncompressedBlockSize)
    {
      RINOK(Flush());
    }
  }
  if (aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return S_OK;
}

STDMETHODIMP Ci86TranslationOutStream::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
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

