// InOutTempBuffer.cpp

#include "StdAfx.h"

#include "InOutTempBuffer.h"
#include "Common/Defs.h"
#include "Windows/Defs.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static UINT32 kTmpBufferMemorySize = (1 << 20);

static LPCTSTR kTempFilePrefixString = _T("iot");

CInOutTempBuffer::CInOutTempBuffer():
  m_Buffer(NULL)
{
}

void CInOutTempBuffer::Create()
{
  m_Buffer = new BYTE[kTmpBufferMemorySize];
}

CInOutTempBuffer::~CInOutTempBuffer()
{
  delete []m_Buffer;
}
void CInOutTempBuffer::InitWriting()
{
  m_BufferPosition = 0;
  m_TmpFileCreated = false;
  m_FileSize = 0;
}

bool CInOutTempBuffer::WriteToFile(const void *aPointer, UINT32 aSize)
{
  if (aSize == 0)
    return true;
  if(!m_TmpFileCreated)
  {
    CSysString aTempDirPath;
    if(!MyGetTempPath(aTempDirPath))
      return false;
    if (m_TempFile.Create(aTempDirPath, kTempFilePrefixString, m_TmpFileName) == 0)
      return false;
    m_OutFile.SetOpenCreationDispositionCreateAlways();
    if(!m_OutFile.Open(m_TmpFileName))
      return false;
    m_TmpFileCreated = true;
  }
  UINT32 aProcessedSize;
  if(!m_OutFile.Write(aPointer, aSize, aProcessedSize))
    return false;
  m_FileSize += aProcessedSize;
  return (aProcessedSize == aSize);
}

bool CInOutTempBuffer::FlushWrite()
{
  return m_OutFile.Close();
}

bool CInOutTempBuffer::Write(const void *aPointer, UINT32 aSize)
{
  UINT32 aNumBytes = 0;
  if(m_BufferPosition < kTmpBufferMemorySize)
  {
    UINT32 aCurSize = MyMin(kTmpBufferMemorySize - m_BufferPosition, aSize);
    memmove(m_Buffer + m_BufferPosition, (const BYTE *)aPointer, aCurSize);
    m_BufferPosition += aCurSize;
    aSize -= aCurSize;
    aPointer = ((const BYTE *)aPointer) + aCurSize;
    m_FileSize += aCurSize;
  }
  return WriteToFile(aPointer, aSize);
}

bool CInOutTempBuffer::InitReading()
{
  m_CurrentPositionInBuffer = 0;
  if(m_TmpFileCreated)
    return m_InFile.Open(m_TmpFileName);
  return true;
}

/*
bool CInOutTempBuffer::Read(void *aPointer, UINT32 aMaxSize, UINT32 &aProcessedSize)
{
  aProcessedSize = 0;
  if (m_CurrentPositionInBuffer < m_BufferPosition)
  {
    UINT32 aSizeToRead = MyMin(m_BufferPosition - m_CurrentPositionInBuffer, aMaxSize);
    memmove(aPointer, m_Buffer + m_CurrentPositionInBuffer, aSizeToRead);
    aPointer = ((BYTE *)aPointer) + aSizeToRead;
    m_CurrentPositionInBuffer += aSizeToRead;
    aProcessedSize += aSizeToRead;
    aMaxSize -= aSizeToRead;
  }
  if (aMaxSize == 0 || !m_TmpFileCreated)
    return true;
  UINT32 aProcessedSizeLoc;
  bool aResult = m_InFile.Read(aPointer, aMaxSize, aProcessedSizeLoc);
  aProcessedSize += aProcessedSizeLoc;
  return aResult;
}
*/

HRESULT CInOutTempBuffer::WriteToStream(ISequentialOutStream *aStream)
{
  if (m_CurrentPositionInBuffer < m_BufferPosition)
  {
    UINT32 aSizeToWrite = m_BufferPosition - m_CurrentPositionInBuffer;
    RETURN_IF_NOT_S_OK(aStream->Write(m_Buffer + m_CurrentPositionInBuffer, aSizeToWrite, NULL));
    m_CurrentPositionInBuffer += aSizeToWrite;
  }
  if (!m_TmpFileCreated)
    return true;
  while(true)
  {
    UINT32 aProcessedSizeLoc;
    if (!m_InFile.Read(m_Buffer, kTmpBufferMemorySize, aProcessedSizeLoc))
      return E_FAIL;
    if (aProcessedSizeLoc == 0)
      return S_OK;
    RETURN_IF_NOT_S_OK(aStream->Write(m_Buffer, aProcessedSizeLoc, NULL));
  }
}

STDMETHODIMP CSequentialOutTempBufferImp::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  if (!m_Buffer->Write(aData, aSize))
  {
    if (aProcessedSize != NULL)
      *aProcessedSize = 0;
    return E_FAIL;
  }
  if (aProcessedSize != NULL)
    *aProcessedSize = aSize;
  return S_OK;
}

STDMETHODIMP CSequentialOutTempBufferImp::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
}
