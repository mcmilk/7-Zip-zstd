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
    bool aTranslationMode, UInt32 aTranslationSize)
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

inline Int32 Ci86TranslationOutStream::ConvertAbsoluteToOffset(Int32 aPos, Int32 anAbsoluteValue)
{
}

static const int kResidue = 6 + 4;

void Ci86TranslationOutStream::MakeTranslation()
{
  if (m_Pos <= kResidue)
    return;
  UInt32 aNumBytes = m_Pos - kResidue;
  for (UInt32 i = 0; i < aNumBytes;)
  {
    if (m_Buffer[i] == 0xE8)
    {
      i++;
      Int32 anAbsoluteValue = 0;
      for(int j = 0; j < 4; j++)
        anAbsoluteValue += UInt32(m_Buffer[i + j]) << (j * 8);

      Int32 aPos = m_ProcessedSize + i - 1;
      UInt32 anOffset;
      if (anAbsoluteValue < -aPos || anAbsoluteValue >= Int32(m_TranslationSize))
      {
      }
      else
      {
        anOffset = (anAbsoluteValue >= 0) ? 
            anAbsoluteValue - aPos :
            anAbsoluteValue + m_TranslationSize;
        for(j = 0; j < 4; j++)
        {
          m_Buffer[i + j] = Byte(anOffset & 0xFF);
          anOffset >>= 8;
        }
      }
      i += 4;
    }
    else
      i++;
  }
}

STDMETHODIMP Ci86TranslationOutStream::Write(const void *aData, UInt32 aSize, UInt32 *aProcessedSize)
{
  if (!m_TranslationMode)
    return m_Stream->Write(aData, aSize, aProcessedSize);

  UInt32 aProcessedSizeReal = 0;

  while (aProcessedSizeReal < aSize)
  {
    UInt32 aWriteSize = MyMin(aSize - aProcessedSizeReal, kUncompressedBlockSize - m_Pos);
    memmove(m_Buffer + m_Pos, (const Byte *)aData + aProcessedSizeReal, aWriteSize);
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

STDMETHODIMP Ci86TranslationOutStream::WritePart(const void *aData, UInt32 aSize, UInt32 *aProcessedSize)
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

