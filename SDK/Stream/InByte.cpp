// Stream/InByte.cpp

#include "stdafx.h"

#include "Stream/InByte.h"

namespace NStream{

CInByte::CInByte(UINT32 aBufferSize):
  m_BufferSize(aBufferSize),
  m_BufferBase(0)
{
  m_BufferBase = new BYTE[m_BufferSize];
}

CInByte::~CInByte()
{
  delete []m_BufferBase;
}

void CInByte::Init(ISequentialInStream *aStream)
{
  m_Stream = aStream;
  m_ProcessedSize = 0;
  m_Buffer = m_BufferBase;
  m_BufferLimit = m_Buffer;
  m_StreamWasExhausted = false;
}

bool CInByte::ReadBlock()
{
  if (m_StreamWasExhausted)
    return false;
  m_ProcessedSize += (m_Buffer - m_BufferBase);
  UINT32 aNumProcessedBytes;
  HRESULT aResult = m_Stream->ReadPart(m_BufferBase, m_BufferSize, &aNumProcessedBytes);
  if (aResult != S_OK)
    throw CInByteReadException(aResult);
  m_Buffer = m_BufferBase;
  m_BufferLimit = m_Buffer + aNumProcessedBytes;
  m_StreamWasExhausted = (aNumProcessedBytes == 0);
  return (!m_StreamWasExhausted);
}

}
