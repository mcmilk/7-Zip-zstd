// StreamBinder.cpp

#include "StdAfx.h"

#include "StreamBinder.h"
#include "Common/Defs.h"

using namespace NWindows;
using namespace NSynchronization;

class CSequentialInStreamForBinder: 
  public ISequentialInStream,
  public CComObjectRoot
{
public:
  BEGIN_COM_MAP(CSequentialInStreamForBinder)
    COM_INTERFACE_ENTRY(ISequentialInStream)
  END_COM_MAP()
  DECLARE_NOT_AGGREGATABLE(CSequentialInStreamForBinder)
  DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
private:
  CStreamBinder *m_StreamBinder;
public:
  ~CSequentialInStreamForBinder() { m_StreamBinder->CloseRead(); }
  void SetBinder(CStreamBinder *aStreamBinder) { m_StreamBinder = aStreamBinder; }
};

STDMETHODIMP CSequentialInStreamForBinder::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
  { return m_StreamBinder->Read(aData, aSize, aProcessedSize); }
  
STDMETHODIMP CSequentialInStreamForBinder::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
  { return m_StreamBinder->ReadPart(aData, aSize, aProcessedSize); }

class CSequentialOutStreamForBinder: 
  public ISequentialOutStream,
  public CComObjectRoot
{
public:
  BEGIN_COM_MAP(CSequentialOutStreamForBinder)
    COM_INTERFACE_ENTRY(ISequentialOutStream)
  END_COM_MAP()
  DECLARE_NOT_AGGREGATABLE(CSequentialOutStreamForBinder)
  DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);

private:
  CStreamBinder *m_StreamBinder;
public:
  ~CSequentialOutStreamForBinder() {  m_StreamBinder->CloseWrite(); }
  void SetBinder(CStreamBinder *aStreamBinder) { m_StreamBinder = aStreamBinder; }
};

STDMETHODIMP CSequentialOutStreamForBinder::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
  { return m_StreamBinder->Write(aData, aSize, aProcessedSize); }

STDMETHODIMP CSequentialOutStreamForBinder::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
  { return m_StreamBinder->WritePart(aData, aSize, aProcessedSize); }


//////////////////////////
// CStreamBinder
// (m_ThereAreBytesToReadEvent && m_BufferSize == 0) means that stream is finished.

void CStreamBinder::CreateEvents()
{
  m_AllBytesAreWritenEvent = new CManualResetEvent(true);
  m_ThereAreBytesToReadEvent = new CManualResetEvent(false);
  m_ReadStreamIsClosedEvent = new CManualResetEvent(false);
}

void CStreamBinder::ReInit()
{
  m_ThereAreBytesToReadEvent->Reset();
  m_ReadStreamIsClosedEvent->Reset();
  m_ProcessedSize = 0;
}

CStreamBinder::~CStreamBinder()
{
  if (m_AllBytesAreWritenEvent != NULL)
    delete m_AllBytesAreWritenEvent;
  if (m_ThereAreBytesToReadEvent != NULL)
    delete m_ThereAreBytesToReadEvent;
  if (m_ReadStreamIsClosedEvent != NULL)
    delete m_ReadStreamIsClosedEvent;
}



  
void CStreamBinder::CreateStreams(ISequentialInStream **anInStream, 
      ISequentialOutStream **anOutStream)
{
  CComObjectNoLock<CSequentialInStreamForBinder> *anInStreamSpec = new 
    CComObjectNoLock<CSequentialInStreamForBinder>;
  CComPtr<ISequentialInStream> anInStreamLoc(anInStreamSpec);
  anInStreamSpec->SetBinder(this);
  *anInStream = anInStreamLoc.Detach();

  CComObjectNoLock<CSequentialOutStreamForBinder> *anOutStreamSpec = new 
    CComObjectNoLock<CSequentialOutStreamForBinder>;
  CComPtr<ISequentialOutStream> anOutStreamLoc(anOutStreamSpec);
  anOutStreamSpec->SetBinder(this);
  *anOutStream = anOutStreamLoc.Detach();

  m_Buffer = NULL;
  m_BufferSize= 0;
  m_ProcessedSize = 0;
}

STDMETHODIMP CStreamBinder::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aSizeToRead = aSize;
  if (aSize > 0)
  {
    if(!m_ThereAreBytesToReadEvent->Lock())
      return E_FAIL;
    aSizeToRead = MyMin(m_BufferSize, aSize);
    if (m_BufferSize > 0)
    {
      MoveMemory(aData, m_Buffer, aSizeToRead);
      m_Buffer = ((const BYTE *)m_Buffer) + aSizeToRead;
      m_BufferSize -= aSizeToRead;
      if (m_BufferSize == 0)
      {
        m_ThereAreBytesToReadEvent->Reset();
        m_AllBytesAreWritenEvent->Set();
      }
    }
  }
  if (aProcessedSize != NULL)
    *aProcessedSize = aSizeToRead;
  m_ProcessedSize += aSizeToRead;
  return S_OK;
}

STDMETHODIMP CStreamBinder::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeFull = 0;
  UINT32 aProcessedSizeReal;
  HRESULT aResult = S_OK;
  while(aSize > 0)
  {
    aResult = ReadPart(aData, aSize, &aProcessedSizeReal);
    aSize -= aProcessedSizeReal;
    aData = (void *)((BYTE *)aData + aProcessedSizeReal);
    aProcessedSizeFull += aProcessedSizeReal;
    if (aResult != S_OK)
      break;
    if (aProcessedSizeReal == 0)
      break;
  }
  if (aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeFull;
  return aResult;
}

void CStreamBinder::CloseRead()
{
  m_ReadStreamIsClosedEvent->Set();
}

STDMETHODIMP CStreamBinder::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  if (aSize > 0)
  {
    m_Buffer = aData;
    m_BufferSize = aSize;
    m_AllBytesAreWritenEvent->Reset();
    m_ThereAreBytesToReadEvent->Set();

    HANDLE anEvents[2]; 
    anEvents[0] = *m_AllBytesAreWritenEvent;
    anEvents[1] = *m_ReadStreamIsClosedEvent; 
    DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
    if (anWaitResult != WAIT_OBJECT_0 + 0)
    {
      // ReadingWasClosed = true;
      return E_FAIL;
    }
    // if(!m_AllBytesAreWritenEvent.Lock())
    //   return E_FAIL;
  }
  if (aProcessedSize != NULL)
    *aProcessedSize = aSize;
  return S_OK;
}

STDMETHODIMP CStreamBinder::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
}

void CStreamBinder::CloseWrite()
{
  // m_BufferSize must be = 0
  m_ThereAreBytesToReadEvent->Set();
}
 
