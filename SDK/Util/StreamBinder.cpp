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

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
private:
  CStreamBinder *m_StreamBinder;
public:
  ~CSequentialInStreamForBinder() { m_StreamBinder->CloseRead(); }
  void SetBinder(CStreamBinder *streamBinder) { m_StreamBinder = streamBinder; }
};

STDMETHODIMP CSequentialInStreamForBinder::Read(void *data, UINT32 size, UINT32 *processedSize)
  { return m_StreamBinder->Read(data, size, processedSize); }
  
STDMETHODIMP CSequentialInStreamForBinder::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
  { return m_StreamBinder->ReadPart(data, size, processedSize); }

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

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);

private:
  CStreamBinder *m_StreamBinder;
public:
  ~CSequentialOutStreamForBinder() {  m_StreamBinder->CloseWrite(); }
  void SetBinder(CStreamBinder *streamBinder) { m_StreamBinder = streamBinder; }
};

STDMETHODIMP CSequentialOutStreamForBinder::Write(const void *data, UINT32 size, UINT32 *processedSize)
  { return m_StreamBinder->Write(data, size, processedSize); }

STDMETHODIMP CSequentialOutStreamForBinder::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
  { return m_StreamBinder->WritePart(data, size, processedSize); }


//////////////////////////
// CStreamBinder
// (_thereAreBytesToReadEvent && _bufferSize == 0) means that stream is finished.

void CStreamBinder::CreateEvents()
{
  _allBytesAreWritenEvent = new CManualResetEvent(true);
  _thereAreBytesToReadEvent = new CManualResetEvent(false);
  _readStreamIsClosedEvent = new CManualResetEvent(false);
}

void CStreamBinder::ReInit()
{
  _thereAreBytesToReadEvent->Reset();
  _readStreamIsClosedEvent->Reset();
  ProcessedSize = 0;
}

CStreamBinder::~CStreamBinder()
{
  if (_allBytesAreWritenEvent != NULL)
    delete _allBytesAreWritenEvent;
  if (_thereAreBytesToReadEvent != NULL)
    delete _thereAreBytesToReadEvent;
  if (_readStreamIsClosedEvent != NULL)
    delete _readStreamIsClosedEvent;
}



  
void CStreamBinder::CreateStreams(ISequentialInStream **inStream, 
      ISequentialOutStream **outStream)
{
  CComObjectNoLock<CSequentialInStreamForBinder> *inStreamSpec = new 
    CComObjectNoLock<CSequentialInStreamForBinder>;
  CComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
  inStreamSpec->SetBinder(this);
  *inStream = inStreamLoc.Detach();

  CComObjectNoLock<CSequentialOutStreamForBinder> *outStreamSpec = new 
    CComObjectNoLock<CSequentialOutStreamForBinder>;
  CComPtr<ISequentialOutStream> outStreamLoc(outStreamSpec);
  outStreamSpec->SetBinder(this);
  *outStream = outStreamLoc.Detach();

  _buffer = NULL;
  _bufferSize= 0;
  ProcessedSize = 0;
}

STDMETHODIMP CStreamBinder::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 sizeToRead = size;
  if (size > 0)
  {
    if(!_thereAreBytesToReadEvent->Lock())
      return E_FAIL;
    sizeToRead = MyMin(_bufferSize, size);
    if (_bufferSize > 0)
    {
      MoveMemory(data, _buffer, sizeToRead);
      _buffer = ((const BYTE *)_buffer) + sizeToRead;
      _bufferSize -= sizeToRead;
      if (_bufferSize == 0)
      {
        _thereAreBytesToReadEvent->Reset();
        _allBytesAreWritenEvent->Set();
      }
    }
  }
  if (processedSize != NULL)
    *processedSize = sizeToRead;
  ProcessedSize += sizeToRead;
  return S_OK;
}

STDMETHODIMP CStreamBinder::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 fullProcessedSize = 0;
  UINT32 realProcessedSize;
  HRESULT result = S_OK;
  while(size > 0)
  {
    result = ReadPart(data, size, &realProcessedSize);
    size -= realProcessedSize;
    data = (void *)((BYTE *)data + realProcessedSize);
    fullProcessedSize += realProcessedSize;
    if (result != S_OK)
      break;
    if (realProcessedSize == 0)
      break;
  }
  if (processedSize != NULL)
    *processedSize = fullProcessedSize;
  return result;
}

void CStreamBinder::CloseRead()
{
  _readStreamIsClosedEvent->Set();
}

STDMETHODIMP CStreamBinder::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
  if (size > 0)
  {
    _buffer = data;
    _bufferSize = size;
    _allBytesAreWritenEvent->Reset();
    _thereAreBytesToReadEvent->Set();

    HANDLE events[2]; 
    events[0] = *_allBytesAreWritenEvent;
    events[1] = *_readStreamIsClosedEvent; 
    DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
    if (waitResult != WAIT_OBJECT_0 + 0)
    {
      // ReadingWasClosed = true;
      return E_FAIL;
    }
    // if(!_allBytesAreWritenEvent.Lock())
    //   return E_FAIL;
  }
  if (processedSize != NULL)
    *processedSize = size;
  return S_OK;
}

STDMETHODIMP CStreamBinder::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}

void CStreamBinder::CloseWrite()
{
  // _bufferSize must be = 0
  _thereAreBytesToReadEvent->Set();
}
 
