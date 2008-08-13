// InMemStream.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "Windows/Thread.h"

#include "InMemStream.h"
#include "../../Common/Defs.h"

void CStreamInfo::Free(IInMemStreamMtCallback *callback)
{
  for (int i = 0; i < Blocks.Size(); i++)
  {
    callback->FreeBlock(Blocks[i]);
    Blocks[i] = 0;
  }
}

bool CInMemStreamMt::Create(int numSubStreams, UInt64 subStreamSize)
{
  Free();
  _subStreamSize = subStreamSize;
  size_t blockSize = Callback->GetBlockSize();
  for (int i = 0; i < numSubStreams; i++)
  {
    _streams.Add(CStreamInfo());
    CStreamInfo &blocks = _streams.Back();
    blocks.Create();
    for (UInt64 j = 0; (UInt64)j * blockSize < _subStreamSize; j++)
      blocks.Blocks.Add(0);
  }
  if (!_streamIndexAllocator.AllocateList(numSubStreams))
    return false;
  return true;
}

void CInMemStreamMt::Free()
{
  while(_streams.Size() > 0)
  {
    _streams.Back().Free(Callback);
    _streams.DeleteBack();
  }
}

HRESULT CInMemStreamMt::Read()
{
  for (;;)
  {
    // printf("\n_streamIndexAllocator.AllocateItem\n");
    int index = _streamIndexAllocator.AllocateItem();
    /*
    if (_stopReading)
      return E_ABORT;
    */
    // printf("\nread Index = %d\n", index);
    CStreamInfo &blocks = _streams[index];
    blocks.Init();
    Callback->AddStreamIndexToQueue(index);

    for (;;)
    {
      const Byte *p = (const Byte *)blocks.Blocks[blocks.LastBlockIndex];
      if (p == 0)
      {
        void **pp = &blocks.Blocks[blocks.LastBlockIndex];
        HRESULT res = Callback->AllocateBlock(pp);
        p = (const Byte *)*pp;
        RINOK(res);
        if (p == 0)
          return E_FAIL;
      }
      size_t blockSize = Callback->GetBlockSize();
      UInt32 curSize = (UInt32)(blockSize - blocks.LastBlockPos);
      UInt32 realProcessedSize;
      UInt64 pos64 = (UInt64)blocks.LastBlockIndex * blockSize + blocks.LastBlockPos;
      if (curSize > _subStreamSize - pos64)
        curSize = (UInt32)(_subStreamSize - pos64);
      RINOK(_stream->Read((void *)(p + blocks.LastBlockPos), curSize, &realProcessedSize));

      blocks.Cs->Enter();
      if (realProcessedSize == 0)
      {
        blocks.StreamWasFinished = true;
        blocks.CanReadEvent->Set();
        blocks.Cs->Leave();

        Callback->AddStreamIndexToQueue(-1);
        return S_OK;
      }
      
      blocks.LastBlockPos += realProcessedSize;
      if (blocks.LastBlockPos == blockSize)
      {
        blocks.LastBlockPos = 0;
        blocks.LastBlockIndex++;
      }
      pos64 += realProcessedSize;
      if (pos64 >= _subStreamSize)
        blocks.StreamWasFinished = true;
      blocks.CanReadEvent->Set();
      blocks.Cs->Leave();
      if (pos64 >= _subStreamSize)
        break;
    }
  }
}

static THREAD_FUNC_DECL CoderThread(void *threadCoderInfo)
{
  ((CInMemStreamMt *)threadCoderInfo)->ReadResult = ((CInMemStreamMt *)threadCoderInfo)->Read();
  return 0;
}

HRes CInMemStreamMt::StartReadThread()
{
  // _stopReading = false;
  NWindows::CThread Thread;
  return Thread.Create(CoderThread, this);
}

void CInMemStreamMt::FreeSubStream(int subStreamIndex)
{
  // printf("\nFreeSubStream\n");
  _streams[subStreamIndex].Free(Callback);
  _streamIndexAllocator.FreeItem(subStreamIndex);
  // printf("\nFreeSubStream end\n");
}

HRESULT CInMemStreamMt::ReadSubStream(int subStreamIndex, void *data, UInt32 size, UInt32 *processedSize, bool keepData)
{
  if (processedSize != NULL)
    *processedSize = 0;
  CStreamInfo &blocks = _streams[subStreamIndex];
  while (size > 0)
  {
    if (blocks.CurBlockPos == Callback->GetBlockSize())
    {
      blocks.CurBlockPos = 0;
      blocks.CurBlockIndex++;
    }
    UInt32 curSize;
    UInt32 curPos = blocks.CurBlockPos;
    
    blocks.Cs->Enter();
    if (blocks.CurBlockIndex == blocks.LastBlockIndex)
    {
      curSize = blocks.LastBlockPos - curPos;
      if (curSize == 0)
      {
        if (blocks.StreamWasFinished)
        {
          blocks.Cs->Leave();
          void *p = blocks.Blocks[blocks.CurBlockIndex];
          if (p != 0 && !keepData)
          {
            Callback->FreeBlock(p);
            blocks.Blocks[blocks.CurBlockIndex] = 0;
          }
          return S_OK;
        }
        blocks.CanReadEvent->Reset();
        blocks.Cs->Leave();
        // printf("\nBlock Lock\n");
        blocks.CanReadEvent->Lock();
        // printf("\nAfter Lock\n");
        if (blocks.ExitResult != S_OK)
          return blocks.ExitResult;
        continue;
      }
    }
    else
      curSize = Callback->GetBlockSize() - curPos;
    blocks.Cs->Leave();
    
    if (curSize > size)
      curSize = size;
    void *p = blocks.Blocks[blocks.CurBlockIndex];
    memmove(data, (const Byte *)p + curPos, curSize);
    data = (void *)((Byte *)data + curSize);
    size -= curSize;
    if (processedSize != NULL)
      *processedSize += curSize;
    curPos += curSize;
    
    bool needFree = false;
    blocks.CurBlockPos = curPos;

    if (curPos == Callback->GetBlockSize())
      needFree = true;
    blocks.Cs->Enter();
    if (blocks.CurBlockIndex == blocks.LastBlockIndex &&
        blocks.CurBlockPos == blocks.LastBlockPos &&
        blocks.StreamWasFinished)
      needFree = true;
    blocks.Cs->Leave();

    if (needFree && !keepData)
    {
      Callback->FreeBlock(p);
      blocks.Blocks[blocks.CurBlockIndex] = 0;
    }
    return S_OK;
  }
  return S_OK;
}

STDMETHODIMP CInMemStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = mtStream->ReadSubStream(Index, data, size, &realProcessedSize, _keepData);
  if (processedSize != NULL)
    *processedSize = realProcessedSize;
  if (realProcessedSize != 0)
  {
    // printf("\ns = %d\n", Index);
  }
  _size += realProcessedSize;
  return result;
}
