// Stream/OutByte.cpp

#include "StdAfx.h"

#include "Stream/OutByte.h"

namespace NStream {

COutByte::COutByte(UINT32 aBufferSize):
  m_BufferSize(aBufferSize)
{
  m_Buffer = new BYTE[m_BufferSize];
}

COutByte::~COutByte()
{
  delete []m_Buffer;
}

void COutByte::Init(ISequentialOutStream *aStream)
{
  m_Stream = aStream;
  m_ProcessedSize = 0;
  m_Pos = 0;
}

void COutByte::ReleaseStream()
{
  m_Stream.Release();
}


HRESULT COutByte::Flush()
{
  if (m_Pos == 0)
    return S_OK;
  UINT32 aProcessedSize;
  HRESULT aResult = m_Stream->Write(m_Buffer, m_Pos, &aProcessedSize);
  if (aResult != S_OK)
    return aResult;
  if (m_Pos != aProcessedSize)
    return E_FAIL;
  m_ProcessedSize += aProcessedSize;
  m_Pos = 0;
  return S_OK;
}

void COutByte::WriteBlock()
{
  HRESULT aResult = Flush();
  if (aResult != S_OK)
    throw COutByteWriteException(aResult);
}

}