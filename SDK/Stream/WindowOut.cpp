// Stream/Window/Out.cpp

#include "StdAfx.h"

#include "Stream/WindowOut.h"

namespace NStream {
namespace NWindow {

void COut::Create(UINT32 aWindowSize)
{
  m_Pos = 0;
  m_StreamPos = 0;
  UINT32 aNewBlockSize = aWindowSize;
  const UINT32 kMinBlockSize = 1;
  if (aNewBlockSize < kMinBlockSize)
    aNewBlockSize = kMinBlockSize;
  if (m_Buffer != 0 && m_WindowSize == aNewBlockSize)
    return;
  delete []m_Buffer;
  m_Buffer = 0;
  m_WindowSize = aNewBlockSize;
  m_Buffer = new BYTE[m_WindowSize];
}

COut::~COut()
{
  ReleaseStream();
  delete []m_Buffer;
}

/*
void COut::SetWindowSize(UINT32 aWindowSize)
{
  m_WindowSize = aWindowSize;
}
*/

void COut::Init(ISequentialOutStream *aStream, bool aSolid)
{
  ReleaseStream();
  m_Stream = aStream;
  m_Stream->AddRef();

  if(!aSolid)
  {
    m_StreamPos = 0;
    m_Pos = 0;
  }
}

void COut::ReleaseStream()
{
  if(m_Stream != 0)
  {
    Flush();
    m_Stream->Release();
    m_Stream = 0;
  }
}

HRESULT COut::Flush()
{
  UINT32 aSize = m_Pos - m_StreamPos;
  if(aSize == 0)
    return S_OK;
  UINT32 aProcessedSize;
  HRESULT aResult = m_Stream->Write(m_Buffer + m_StreamPos, aSize, &aProcessedSize);
  if (aResult != S_OK)
    return aResult;
  if (aSize != aProcessedSize)
    return E_FAIL;
  if (m_Pos >= m_WindowSize)
    m_Pos = 0;
  m_StreamPos = m_Pos;
  return S_OK;
}

}}
