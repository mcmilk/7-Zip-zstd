// Stream/WindowIn.cpp

#include "StdAfx.h"

#include "WindowIn.h"

#include "Common/Defs.h"

#include "Windows/Defs.h"

namespace NStream {
namespace NWindow {

CIn::CIn():
  m_BufferBase(0)
{}

void CIn::Create(UINT32 aKeepSizeBefore, UINT32 aKeepSizeAfter, UINT32 aKeepSizeReserv)
{
  m_KeepSizeBefore = aKeepSizeBefore;
  m_KeepSizeAfter = aKeepSizeAfter;
  m_KeepSizeReserv = aKeepSizeReserv;
  m_BlockSize = aKeepSizeBefore + aKeepSizeAfter + aKeepSizeReserv;
  delete []m_BufferBase;
  m_BufferBase = 0;
  m_BufferBase = new BYTE[m_BlockSize];
  m_PointerToLastSafePosition = m_BufferBase + m_BlockSize - aKeepSizeAfter;
}

CIn::~CIn()
{
  delete []m_BufferBase;
}

HRESULT CIn::Init(ISequentialInStream *aStream)
{
  m_Stream = aStream;
  m_Buffer = m_BufferBase;
  m_Pos = 0;
  m_StreamPos = 0;
  m_StreamEndWasReached = false;
  return ReadBlock();
}

void CIn::ReleaseStream()
{
  m_Stream.Release();
}


///////////////////////////////////////////
// ReadBlock

// In State:
//   (m_Buffer + m_StreamPos) <= (m_BufferBase + m_BlockSize)
// Out State:
//   m_PosLimit <= m_BlockSize - m_KeepSizeAfter;
//   if(m_StreamEndWasReached == false):
//     m_StreamPos >= m_Pos + m_KeepSizeAfter
//     m_PosLimit = m_StreamPos - m_KeepSizeAfter;
//   else
//          
  
HRESULT CIn::ReadBlock()
{
  if(m_StreamEndWasReached)
    return S_OK;
  while(true)
  {
    UINT32 aSize = (m_BufferBase + m_BlockSize) - (m_Buffer + m_StreamPos);
    if(aSize == 0)
      return S_OK;
    UINT32 aNumReadBytes;
    RETURN_IF_NOT_S_OK(m_Stream->ReadPart(m_Buffer + m_StreamPos, 
        aSize, &aNumReadBytes));
    if(aNumReadBytes == 0)
    {
      m_PosLimit = m_StreamPos;
      const BYTE *aPointerToPostion = m_Buffer + m_PosLimit;
      if(aPointerToPostion > m_PointerToLastSafePosition)
        m_PosLimit = m_PointerToLastSafePosition - m_Buffer;
      m_StreamEndWasReached = true;
      return S_OK;
    }
    m_StreamPos += aNumReadBytes;
    if(m_StreamPos >= m_Pos + m_KeepSizeAfter)
    {
      m_PosLimit = m_StreamPos - m_KeepSizeAfter;
      return S_OK;
    }
  }
}

// UINT32 CIn::GetMatchLen(UINT32 aIndex, UINT32 aBack, UINT32 aLimit)const

void CIn::MoveBlock(UINT32 anOffset)
{
  UINT32 aNumBytes = (m_Buffer + m_StreamPos) -  (m_BufferBase + anOffset);
  memmove(m_BufferBase, m_BufferBase + anOffset, aNumBytes);
  m_Buffer -= anOffset;
}


}}
