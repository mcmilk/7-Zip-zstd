// Stream/Window/Out.cpp

#include "StdAfx.h"

#include "Stream/WindowOut.h"

namespace NStream {
namespace NWindow {

void COut::Create(UINT32 aKeepSizeBefore, UINT32 aKeepSizeAfter, UINT32 aKeepSizeReserv)
{
  m_Pos = 0;
  m_PosLimit = aKeepSizeReserv + aKeepSizeBefore;
  m_KeepSizeBefore = aKeepSizeBefore;
  m_KeepSizeAfter = aKeepSizeAfter;
  m_KeepSizeReserv = aKeepSizeReserv;
  m_StreamPos = 0;
  m_MoveFrom = m_KeepSizeReserv;
  m_WindowSize = aKeepSizeBefore;
  UINT32 aBlockSize = m_KeepSizeBefore + m_KeepSizeAfter + m_KeepSizeReserv;
  delete []m_Buffer;
  m_Buffer = new BYTE[aBlockSize];
}

COut::~COut()
{
  delete []m_Buffer;
  ReleaseStream();
}

void COut::SetWindowSize(UINT32 aWindowSize)
{
  m_WindowSize = aWindowSize;
  m_MoveFrom = m_KeepSizeReserv + m_KeepSizeBefore - aWindowSize;
}

void COut::Init(ISequentialOutStream *aStream, bool aSolid)
{
  ReleaseStream();
  m_Stream = aStream;
  m_Stream->AddRef();

  if(aSolid)
    m_StreamPos = m_Pos;
  else
  {
    m_Pos = 0;
    m_PosLimit = m_KeepSizeReserv + m_KeepSizeBefore;
    m_StreamPos = 0;
  }
}

void COut::ReleaseStream()
{
  if(m_Stream != 0)
  {
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
  m_StreamPos = m_Pos;
  return S_OK;
}

void COut::MoveBlockBackward()
{
  HRESULT aResult = Flush();
  if (aResult != S_OK)
    throw COutWriteException(aResult);
  memmove(m_Buffer, m_Buffer + m_MoveFrom, m_WindowSize + m_KeepSizeAfter);
  m_Pos -= m_MoveFrom;
  m_StreamPos -= m_MoveFrom;
}

}}
