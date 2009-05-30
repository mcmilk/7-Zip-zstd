// InMemStream.h

#ifndef __IN_MEM_STREAM_H
#define __IN_MEM_STREAM_H

#include <stdio.h>

#include "../../../C/Alloc.h"

#include "../../Common/MyCom.h"

#include "MemBlocks.h"

class CIntListCheck
{
protected:
  int *_data;
public:
  CIntListCheck(): _data(0) {}
  ~CIntListCheck() { FreeList(); }
  
  bool AllocateList(int numItems)
  {
    FreeList();
    if (numItems == 0)
      return true;
    _data = (int *)::MyAlloc(numItems * sizeof(int));
    return (_data != 0);
  }
  
  void FreeList()
  {
    ::MyFree(_data);
    _data = 0;
  }
};


class CResourceList : public CIntListCheck
{
  int _headFree;
public:
  CResourceList(): _headFree(-1) {}
  
  bool AllocateList(int numItems)
  {
    FreeList();
    if (numItems == 0)
      return true;
    if (!CIntListCheck::AllocateList(numItems))
      return false;
    for (int i = 0; i < numItems; i++)
      _data[i] = i + 1;
    _data[numItems - 1] = -1;
    _headFree = 0;
    return true;
  }
  
  void FreeList()
  {
    CIntListCheck::FreeList();
    _headFree = -1;
  }
  
  int AllocateItem()
  {
    int res = _headFree;
    if (res >= 0)
      _headFree = _data[res];
    return res;
  }
  
  void FreeItem(int index)
  {
    if (index < 0)
      return;
    _data[index] = _headFree;
    _headFree = index;
  }
};

class CResourceListMt: public CResourceList
{
  NWindows::NSynchronization::CCriticalSection _criticalSection;
public:
  NWindows::NSynchronization::CSemaphore Semaphore;

  HRes AllocateList(int numItems)
  {
    if (!CResourceList::AllocateList(numItems))
      return E_OUTOFMEMORY;
    Semaphore.Close();
    return Semaphore.Create(numItems, numItems);
  }

  int AllocateItem()
  {
    Semaphore.Lock();
    _criticalSection.Enter();
    int res = CResourceList::AllocateItem();
    _criticalSection.Leave();
    return res;
  }
  
  void FreeItem(int index)
  {
    if (index < 0)
      return;
    _criticalSection.Enter();
    CResourceList::FreeItem(index);
    _criticalSection.Leave();
    Semaphore.Release();
  }
};

class CIntQueueMt: public CIntListCheck
{
  int _numItems;
  int _head;
  int _cur;
public:
  CIntQueueMt(): _numItems(0), _head(0),  _cur(0) {}
  NWindows::NSynchronization::CSemaphore Semaphore;

  HRes AllocateList(int numItems)
  {
    FreeList();
    if (numItems == 0)
      return S_OK;
    if (!CIntListCheck::AllocateList(numItems))
      return E_OUTOFMEMORY;
    _numItems = numItems;
    return Semaphore.Create(0, numItems);
  }

  void FreeList()
  {
    CIntListCheck::FreeList();
    _numItems = 0;
    _head = 0;
    _cur = 0;
  }

  void AddItem(int value)
  {
    _data[_head++] = value;
    if (_head == _numItems)
      _head = 0;
    Semaphore.Release();
    // printf("\nRelease prev = %d\n", previousCount);
  }
  
  int GetItem()
  {
    // Semaphore.Lock();
    int res = _data[_cur++];
    if (_cur == _numItems)
      _cur = 0;
    return res;
  }
};

struct IInMemStreamMtCallback
{
  // must be same for all calls
  virtual size_t GetBlockSize() = 0;
  
  // Out:
  //  result != S_OK stops Reading
  //   if *p = 0, result must be != S_OK;
  // Locking is allowed
  virtual HRESULT AllocateBlock(void **p) = 0;

  virtual void FreeBlock(void *p) = 0;

  // It must allow to add at least numSubStreams + 1 ,
  // where numSubStreams is value from CInMemStreamMt::Create
  // value -1 means End of stream
  // Locking is not allowed
  virtual void AddStreamIndexToQueue(int index) = 0;
};

struct CStreamInfo
{
  CRecordVector<void *> Blocks;

  int LastBlockIndex;
  size_t LastBlockPos;
  bool StreamWasFinished;

  int CurBlockIndex;
  size_t CurBlockPos;

  NWindows::NSynchronization::CCriticalSection *Cs;
  NWindows::NSynchronization::CManualResetEvent *CanReadEvent;

  HRESULT ExitResult;

  CStreamInfo(): Cs(0), CanReadEvent(0), StreamWasFinished(false) { }
  ~CStreamInfo()
  {
    delete Cs;
    delete CanReadEvent;
    // Free();
  }
  void Create()
  {
    Cs = new NWindows::NSynchronization::CCriticalSection;
    CanReadEvent = new NWindows::NSynchronization::CManualResetEvent;
  }

  void Free(IInMemStreamMtCallback *callback);
  void Init()
  {
    LastBlockIndex = CurBlockIndex = 0;
    CurBlockPos = LastBlockPos = 0;
    StreamWasFinished = false;
    ExitResult = S_OK;
  }

  // res must be != S_OK
  void Exit(HRESULT res)
  {
    ExitResult = res;
    CanReadEvent->Set();
  }
};


class CInMemStreamMt
{
  CMyComPtr<ISequentialInStream> _stream;
  NWindows::NSynchronization::CCriticalSection CS;
  CObjectVector<CStreamInfo> _streams;
  int _nextFreeStreamIndex;
  int _currentStreamIndex;
  UInt64 _subStreamSize;

  CResourceListMt _streamIndexAllocator;

  // bool _stopReading;

public:
  HRESULT Read();
  HRESULT ReadResult;
  IInMemStreamMtCallback *Callback;
  void FreeSubStream(int subStreamIndex);
  HRESULT ReadSubStream(int subStreamIndex, void *data, UInt32 size, UInt32 *processedSize, bool keepData);

  // numSubStreams: min = 1, good min = numThreads
  bool Create(int numSubStreams, UInt64 subStreamSize);
  ~CInMemStreamMt() { Free(); }
  void SetStream(ISequentialInStream *stream) { _stream = stream; }
  
  // to stop reading you must implement
  // returning Error in IInMemStreamMtCallback::AllocateBlock
  // and then you must free at least one substream
  HRes StartReadThread();

  void Free();

  // you must free at least one substream after that function to unlock waiting.
  // void StopReading() { _stopReading = true; }
};

class CInMemStream:
  public ISequentialInStream,
  public CMyUnknownImp
{
  UInt64 _size;
  bool _keepData;
public:
  int Index;
  CInMemStreamMt *mtStream;
  void Init(bool keepData = false)
  {
    _size = 0; _keepData = keepData ;
  }
  MY_UNKNOWN_IMP
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  UInt64 GetSize() const { return _size; }
};

#endif
