// ZipUpdate.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "ZipUpdate.h"
#include "ZipAddCommon.h"
#include "ZipOut.h"

#include "Common/Defs.h"
#include "Common/AutoPtr.h"
#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/Thread.h"

#include "../../Common/ProgressUtils.h"
#ifdef COMPRESS_MT
#include "../../Common/ProgressMt.h"
#endif
#include "../../Common/LimitedStreams.h"
#include "../../Common/OutMemStream.h"
#include "../../Common/CreateCoder.h"

#include "../../Compress/Copy/CopyCoder.h"

using namespace NWindows;
using namespace NSynchronization;

namespace NArchive {
namespace NZip {

class CCriticalSectionLock2
{
  CCriticalSection *_object;
  void Unlock()  { if (_object != 0) _object->Leave(); }
public:
  CCriticalSectionLock2(): _object(0) {}
  void Set(CCriticalSection &object) { _object = &object; _object->Enter(); } 
  ~CCriticalSectionLock2() { Unlock(); }
};

static const Byte kMadeByHostOS = NFileHeader::NHostOS::kFAT;
static const Byte kExtractHostOS = NFileHeader::NHostOS::kFAT;

static const Byte kMethodForDirectory = NFileHeader::NCompressionMethod::kStored;
static const Byte kExtractVersionForDirectory = NFileHeader::NCompressionMethod::kStoreExtractVersion;

static HRESULT CopyBlockToArchive(ISequentialInStream *inStream, 
    COutArchive &outArchive, ICompressProgressInfo *progress)
{
  CMyComPtr<ISequentialOutStream> outStream;
  outArchive.CreateStreamForCopying(&outStream);
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
}

static HRESULT WriteRange(IInStream *inStream, COutArchive &outArchive, 
    const CUpdateRange &range, ICompressProgressInfo *progress)
{
  UInt64 position;
  RINOK(inStream->Seek(range.Position, STREAM_SEEK_SET, &position));

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
  streamSpec->SetStream(inStream);
  streamSpec->Init(range.Size);

  RINOK(CopyBlockToArchive(inStreamLimited, outArchive, progress));
  return progress->SetRatioInfo(&range.Size, &range.Size);
}

static void SetFileHeader(
    COutArchive &archive, 
    const CCompressionMethodMode &options,
    const CUpdateItem &updateItem, 
    CItem &item)
{
  item.UnPackSize = updateItem.Size;
  bool isDirectory;

  if (updateItem.NewProperties)
  {
    isDirectory = updateItem.IsDirectory;
    item.Name = updateItem.Name; 
    item.ExternalAttributes = updateItem.Attributes;
    item.Time = updateItem.Time;
  }
  else
    isDirectory = item.IsDirectory();

  item.LocalHeaderPosition = archive.GetCurrentPosition();
  item.MadeByVersion.HostOS = kMadeByHostOS;
  item.MadeByVersion.Version = NFileHeader::NCompressionMethod::kMadeByProgramVersion;
  
  item.ExtractVersion.HostOS = kExtractHostOS;

  item.InternalAttributes = 0; // test it
  item.ClearFlags();
  item.SetEncrypted(!isDirectory && options.PasswordIsDefined);
  if (isDirectory)
  {
    item.ExtractVersion.Version = kExtractVersionForDirectory;
    item.CompressionMethod = kMethodForDirectory;
    item.PackSize = 0;
    item.FileCRC = 0; // test it
  }
}

static void SetItemInfoFromCompressingResult(const CCompressingResult &compressingResult, 
    bool isAesMode, Byte aesKeyMode, CItem &item)
{
  item.ExtractVersion.Version = compressingResult.ExtractVersion;
  item.CompressionMethod = compressingResult.Method;
  item.FileCRC = compressingResult.CRC;
  item.UnPackSize = compressingResult.UnpackSize;
  item.PackSize = compressingResult.PackSize;

  item.LocalExtra.Clear();
  item.CentralExtra.Clear();

  if (isAesMode)
  {
    CWzAesExtraField wzAesField;
    wzAesField.Strength = aesKeyMode;
    wzAesField.Method = compressingResult.Method;
    item.CompressionMethod = NFileHeader::NCompressionMethod::kWzAES;
    item.FileCRC = 0;
    CExtraSubBlock sb;
    wzAesField.SetSubBlock(sb);
    item.LocalExtra.SubBlocks.Add(sb);
    item.CentralExtra.SubBlocks.Add(sb);
  }
}

#ifdef COMPRESS_MT

static DWORD WINAPI CoderThread(void *threadCoderInfo);

struct CThreadInfo
{
  #ifdef EXTERNAL_CODECS
  CMyComPtr<ICompressCodecsInfo> _codecsInfo;
  const CObjectVector<CCodecInfoEx> *_externalCodecs;
  #endif

  NWindows::CThread Thread;
  CAutoResetEvent *CompressEvent;
  CAutoResetEvent *CompressionCompletedEvent;
  bool ExitThread;

  CMtCompressProgress *ProgressSpec;
  CMyComPtr<ICompressProgressInfo> Progress;

  COutMemStream *OutStreamSpec;
  CMyComPtr<IOutStream> OutStream;
  CMyComPtr<ISequentialInStream> InStream;

  CAddCommon Coder;
  HRESULT Result;
  CCompressingResult CompressingResult;

  bool IsFree;
  UInt32 UpdateIndex;

  CThreadInfo(const CCompressionMethodMode &options):
      CompressEvent(NULL), 
      CompressionCompletedEvent(NULL),
      ExitThread(false),
      ProgressSpec(0),
      OutStreamSpec(0),
      Coder(options)
  {}
  
  void CreateEvents()
  {
    CompressEvent = new CAutoResetEvent(false);
    CompressionCompletedEvent = new CAutoResetEvent(false);
  }
  bool CreateThread() { return Thread.Create(CoderThread, this); }
  ~CThreadInfo();

  void WaitAndCode();
  void StopWaitClose()
  {
    ExitThread = true;
    if (OutStreamSpec != 0)
      OutStreamSpec->StopWriting(E_ABORT);
    if (CompressEvent != NULL)
      CompressEvent->Set();
    Thread.Wait();
    Thread.Close();
  }

};

CThreadInfo::~CThreadInfo()
{
  if (CompressEvent != NULL)
    delete CompressEvent;
  if (CompressionCompletedEvent != NULL)
    delete CompressionCompletedEvent;
}

void CThreadInfo::WaitAndCode()
{
  for (;;)
  {
    CompressEvent->Lock();
    if (ExitThread)
      return;
    Result = Coder.Compress(
        #ifdef EXTERNAL_CODECS
        _codecsInfo, _externalCodecs, 
        #endif
        InStream, OutStream, Progress, CompressingResult);
    if (Result == S_OK && Progress)
      Result = Progress->SetRatioInfo(&CompressingResult.UnpackSize, &CompressingResult.PackSize);
    CompressionCompletedEvent->Set();
  }
}

static DWORD WINAPI CoderThread(void *threadCoderInfo)
{
  ((CThreadInfo *)threadCoderInfo)->WaitAndCode();
  return 0;
}

class CThreads
{
public:
  CObjectVector<CThreadInfo> Threads;
  ~CThreads()
  {
    for (int i = 0; i < Threads.Size(); i++)
      Threads[i].StopWaitClose();
  }
};

struct CMemBlocks2: public CMemLockBlocks
{
  CCompressingResult CompressingResult;
  bool Defined;
  bool Skip;
  CMemBlocks2(): Defined(false), Skip(false) {}
};

class CMemRefs
{
public:
  CMemBlockManagerMt *Manager;
  CObjectVector<CMemBlocks2> Refs;
  CMemRefs(CMemBlockManagerMt *manager): Manager(manager) {} ;
  ~CMemRefs()
  {
    for (int i = 0; i < Refs.Size(); i++)
      Refs[i].FreeOpt(Manager);
  }
};

#endif


static HRESULT UpdateItemOldData(COutArchive &archive, 
    IInStream *inStream,
    const CUpdateItem &updateItem, CItemEx &item, ICompressProgressInfo *progress, 
    UInt64 &complexity)
{
  if (updateItem.NewProperties)
  {
    if (item.HasDescriptor())
      return E_NOTIMPL;
    
    // use old name size.
    // CUpdateRange range(item.GetLocalExtraPosition(), item.LocalExtraSize + item.PackSize);
    CUpdateRange range(item.GetDataPosition(), item.PackSize);
    
    // item.ExternalAttributes = updateItem.Attributes;
    // Test it
    item.Name = updateItem.Name; 
    item.Time = updateItem.Time;
    item.CentralExtra.RemoveUnknownSubBlocks();
    item.LocalExtra.RemoveUnknownSubBlocks();
    
    archive.PrepareWriteCompressedData2((UInt16)item.Name.Length(), item.UnPackSize, item.PackSize, item.LocalExtra.HasWzAesField());
    item.LocalHeaderPosition = archive.GetCurrentPosition();
    archive.SeekToPackedDataPosition();
    RINOK(WriteRange(inStream, archive, range, progress));
    complexity += range.Size;
    archive.WriteLocalHeader(item);
  }
  else
  {
    CUpdateRange range(item.LocalHeaderPosition, item.GetLocalFullSize());
    
    // set new header position
    item.LocalHeaderPosition = archive.GetCurrentPosition(); 
    
    RINOK(WriteRange(inStream, archive, range, progress));
    complexity += range.Size;
    archive.MoveBasePosition(range.Size);
  }
  return S_OK;
}

static HRESULT WriteDirHeader(COutArchive &archive, const CCompressionMethodMode *options,
    const CUpdateItem &updateItem, CItemEx &item)
{
  SetFileHeader(archive, *options, updateItem, item);
  archive.PrepareWriteCompressedData((UInt16)item.Name.Length(), updateItem.Size, options->IsAesMode);
  return archive.WriteLocalHeader(item);
}

static HRESULT Update2St(
    DECL_EXTERNAL_CODECS_LOC_VARS
    COutArchive &archive, 
    CInArchive *inArchive,
    IInStream *inStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    const CCompressionMethodMode *options, 
    const CByteBuffer &comment,
    IArchiveUpdateCallback *updateCallback)
{
  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);

  CLocalCompressProgressInfo *localCompressProgressSpec = new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  CAddCommon compressor(*options);
  
  CObjectVector<CItem> items;
  UInt64 complexity = 0;

  for (int itemIndex = 0; itemIndex < updateItems.Size(); itemIndex++)
  {
    RINOK(updateCallback->SetCompleted(&complexity));
    const CUpdateItem &updateItem = updateItems[itemIndex];
    CItemEx item;
    if (!updateItem.NewProperties || !updateItem.NewData)
    {
      item = inputItems[updateItem.IndexInArchive];
      if (inArchive->ReadLocalItemAfterCdItemFull(item) != S_OK)
        return E_NOTIMPL;
    }

    if (updateItem.NewData)
    {
      bool isDirectory = ((updateItem.NewProperties) ? updateItem.IsDirectory : item.IsDirectory());
      if (isDirectory)
      {
        RINOK(WriteDirHeader(archive, options, updateItem, item));
      }
      else
      {
        CMyComPtr<ISequentialInStream> fileInStream;
        HRESULT res = updateCallback->GetStream(updateItem.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          complexity += updateItem.Size + NFileHeader::kLocalBlockSize;
          RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
          continue;
        }
        RINOK(res);

        // file Size can be 64-bit !!!
        SetFileHeader(archive, *options, updateItem, item);
        archive.PrepareWriteCompressedData((UInt16)item.Name.Length(), updateItem.Size, options->IsAesMode);
        CCompressingResult compressingResult;
        CMyComPtr<IOutStream> outStream;
        archive.CreateStreamForCompressing(&outStream);
        localCompressProgressSpec->Init(localProgress, &complexity, NULL);
        RINOK(compressor.Compress(
            EXTERNAL_CODECS_LOC_VARS
            fileInStream, outStream, compressProgress, compressingResult));
        SetItemInfoFromCompressingResult(compressingResult, options->IsAesMode, options->AesKeyMode, item);
        RINOK(archive.WriteLocalHeader(item));
        RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
        complexity += item.UnPackSize;
      }
    }
    else
    {
      localCompressProgressSpec->Init(localProgress, &complexity, NULL);
      RINOK(UpdateItemOldData(archive, inStream, updateItem, item, compressProgress, complexity));
    }
    items.Add(item);
    complexity += NFileHeader::kLocalBlockSize;
  }
  archive.WriteCentralDir(items, comment);
  return S_OK;
}

static HRESULT Update2(
    DECL_EXTERNAL_CODECS_LOC_VARS
    COutArchive &archive, 
    CInArchive *inArchive,
    IInStream *inStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    const CCompressionMethodMode *options, 
    const CByteBuffer &comment,
    IArchiveUpdateCallback *updateCallback)
{
  UInt64 complexity = 0;
  UInt64 numFilesToCompress = 0;
  UInt64 numBytesToCompress = 0;
 
  int i;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &updateItem = updateItems[i];
    if (updateItem.NewData)
    {
      complexity += updateItem.Size;
      numBytesToCompress += updateItem.Size;
      numFilesToCompress++;
      /*
      if (updateItem.Commented)
        complexity += updateItem.CommentRange.Size;
      */
    }
    else
    {
      CItemEx inputItem = inputItems[updateItem.IndexInArchive];
      if (inArchive->ReadLocalItemAfterCdItemFull(inputItem) != S_OK)
        return E_NOTIMPL;
      complexity += inputItem.GetLocalFullSize();
      // complexity += inputItem.GetCentralExtraPlusCommentSize();
    }
    complexity += NFileHeader::kLocalBlockSize;
    complexity += NFileHeader::kCentralBlockSize;
  }

  if (comment != 0)
    complexity += comment.GetCapacity();
  complexity++; // end of central
  updateCallback->SetTotal(complexity);

  CAddCommon compressor(*options);
  
  complexity = 0;
  
  #ifdef COMPRESS_MT

  const size_t kNumMaxThreads = (1 << 10);
  UInt32 numThreads = options->NumThreads;
  if (numThreads > kNumMaxThreads)
    numThreads = kNumMaxThreads;
  
  const size_t kMemPerThread = (1 << 25);
  const size_t kBlockSize = 1 << 16;

  CCompressionMethodMode options2;
  if (options != 0)
    options2 = *options;

  bool mtMode = ((options != 0) && (numThreads > 1));

  if (numFilesToCompress <= 1)
    mtMode = false;

  if (mtMode)
  {
    Byte method = options->MethodSequence.Front();
    if (method == NFileHeader::NCompressionMethod::kStored && !options->PasswordIsDefined)
      mtMode = false;
    if (method == NFileHeader::NCompressionMethod::kBZip2)
    {
      UInt64 averageSize = numBytesToCompress / numFilesToCompress;
      UInt32 blockSize = options->DicSize;
      if (blockSize == 0)
        blockSize = 1;
      UInt64 averageNumberOfBlocks = averageSize / blockSize;
      UInt32 numBZip2Threads = 32;
      if (averageNumberOfBlocks < numBZip2Threads)
        numBZip2Threads = (UInt32)averageNumberOfBlocks;
      if (numBZip2Threads < 1)
        numBZip2Threads = 1;
      numThreads = numThreads / numBZip2Threads;
      options2.NumThreads = numBZip2Threads;
      if (numThreads <= 1)
        mtMode = false;
    }
  }

  if (!mtMode)
  #endif
    return Update2St(
        EXTERNAL_CODECS_LOC_VARS
        archive, inArchive,inStream,
        inputItems, updateItems, options, comment, updateCallback);


  #ifdef COMPRESS_MT

  CObjectVector<CItem> items;

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);

  CMtCompressProgressMixer mtCompressProgressMixer;
  mtCompressProgressMixer.Init(numThreads + 1, localProgress); 

  // we need one item for main stream
  CMtCompressProgress *progressMainSpec  = new CMtCompressProgress();
  CMyComPtr<ICompressProgressInfo> progressMain = progressMainSpec;
  progressMainSpec->Init(&mtCompressProgressMixer, (int)numThreads);

  CMemBlockManagerMt memManager(kBlockSize);
  CMemRefs refs(&memManager);

  CThreads threads;
  CRecordVector<HANDLE> compressingCompletedEvents;
  CRecordVector<int> threadIndices;  // list threads in order of updateItems

  {
    if (!memManager.AllocateSpaceAlways((size_t)numThreads * (kMemPerThread / kBlockSize)))
      return E_OUTOFMEMORY;
    for(i = 0; i < updateItems.Size(); i++)
      refs.Refs.Add(CMemBlocks2());

    UInt32 i;
    for (i = 0; i < numThreads; i++)
      threads.Threads.Add(CThreadInfo(options2));

    for (i = 0; i < numThreads; i++)
    {
      CThreadInfo &threadInfo = threads.Threads[i];
      #ifdef EXTERNAL_CODECS
      threadInfo._codecsInfo = codecsInfo;
      threadInfo._externalCodecs = externalCodecs;
      #endif
      threadInfo.CreateEvents();
      threadInfo.OutStreamSpec = new COutMemStream(&memManager);
      threadInfo.OutStream = threadInfo.OutStreamSpec;
      threadInfo.IsFree = true;
      threadInfo.ProgressSpec = new CMtCompressProgress();
      threadInfo.Progress = threadInfo.ProgressSpec;
      threadInfo.ProgressSpec->Init(&mtCompressProgressMixer, (int)i);
      if (!threadInfo.CreateThread())
        return ::GetLastError();
    }
  }
  int mtItemIndex = 0;

  int itemIndex = 0;
  int lastRealStreamItemIndex = -1;

  while (itemIndex < updateItems.Size())
  {
    if ((UInt32)threadIndices.Size() < numThreads && mtItemIndex < updateItems.Size())
    {
      const CUpdateItem &updateItem = updateItems[mtItemIndex++];
      if (!updateItem.NewData)
        continue;
      CItemEx item;
      if (updateItem.NewProperties)
      {
        if (updateItem.IsDirectory)
          continue;
      }
      else
      {
        item = inputItems[updateItem.IndexInArchive];
        if (inArchive->ReadLocalItemAfterCdItemFull(item) != S_OK)
          return E_NOTIMPL;
        if (item.IsDirectory())
          continue;
      }
      CMyComPtr<ISequentialInStream> fileInStream;
      {
        NWindows::NSynchronization::CCriticalSectionLock lock(mtCompressProgressMixer.CriticalSection);
        HRESULT res = updateCallback->GetStream(updateItem.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          complexity += updateItem.Size;
          complexity++;
          RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
          refs.Refs[mtItemIndex - 1].Skip = true;
          continue;
        }
        RINOK(res);
        RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
      }

      for (UInt32 i = 0; i < numThreads; i++)
      {
        CThreadInfo &threadInfo = threads.Threads[i];
        if (threadInfo.IsFree)
        {
          threadInfo.IsFree = false;
          threadInfo.InStream = fileInStream;

          // !!!!! we must release ref before sending event 
          // BUG was here in v4.43 and v4.44. It could change ref counter in two threads in same time
          fileInStream.Release(); 

          threadInfo.OutStreamSpec->Init();
          threadInfo.ProgressSpec->Reinit();
          threadInfo.CompressEvent->Set();
          threadInfo.UpdateIndex = mtItemIndex - 1;

          compressingCompletedEvents.Add(*threadInfo.CompressionCompletedEvent);
          threadIndices.Add(i);
          break;
        }
      }
      continue;
    }
    
    if (refs.Refs[itemIndex].Skip)
    {
      itemIndex++;
      continue;
    }

    const CUpdateItem &updateItem = updateItems[itemIndex];

    CItemEx item;
    if (!updateItem.NewProperties || !updateItem.NewData)
    {
      item = inputItems[updateItem.IndexInArchive];
      if (inArchive->ReadLocalItemAfterCdItemFull(item) != S_OK)
        return E_NOTIMPL;
    }

    if (updateItem.NewData)
    {
      bool isDirectory = ((updateItem.NewProperties) ? updateItem.IsDirectory : item.IsDirectory());
      if (isDirectory)
      {
        RINOK(WriteDirHeader(archive, options, updateItem, item));
      }
      else
      {
        if (lastRealStreamItemIndex < itemIndex)
        {
          lastRealStreamItemIndex = itemIndex;
          SetFileHeader(archive, *options, updateItem, item);
          // file Size can be 64-bit !!!
          archive.PrepareWriteCompressedData((UInt16)item.Name.Length(), updateItem.Size, options->IsAesMode);
        }

        CMemBlocks2 &memRef = refs.Refs[itemIndex];
        if (memRef.Defined)
        {
          CMyComPtr<IOutStream> outStream;
          archive.CreateStreamForCompressing(&outStream);
          memRef.WriteToStream(memManager.GetBlockSize(), outStream);
          SetItemInfoFromCompressingResult(memRef.CompressingResult,
              options->IsAesMode, options->AesKeyMode, item);
          SetFileHeader(archive, *options, updateItem, item);
          RINOK(archive.WriteLocalHeader(item));
          complexity += item.UnPackSize;
          // RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
          memRef.FreeOpt(&memManager);
        }
        else
        {
          {
            CThreadInfo &thread = threads.Threads[threadIndices.Front()];
            if (!thread.OutStreamSpec->WasUnlockEventSent())
            {
              CMyComPtr<IOutStream> outStream;
              archive.CreateStreamForCompressing(&outStream);
              thread.OutStreamSpec->SetOutStream(outStream);
              thread.OutStreamSpec->SetRealStreamMode();
            }
          }

          DWORD result = ::WaitForMultipleObjects(compressingCompletedEvents.Size(), 
              &compressingCompletedEvents.Front(), FALSE, INFINITE);
          int t = (int)(result - WAIT_OBJECT_0);
          CThreadInfo &threadInfo = threads.Threads[threadIndices[t]];
          threadInfo.InStream.Release();
          threadInfo.IsFree = true;
          RINOK(threadInfo.Result);
          threadIndices.Delete(t);
          compressingCompletedEvents.Delete(t);
          if (t == 0)
          {
            RINOK(threadInfo.OutStreamSpec->WriteToRealStream());
            threadInfo.OutStreamSpec->ReleaseOutStream();
            SetItemInfoFromCompressingResult(threadInfo.CompressingResult, 
                options->IsAesMode, options->AesKeyMode, item);
            SetFileHeader(archive, *options, updateItem, item);
            RINOK(archive.WriteLocalHeader(item));
            complexity += item.UnPackSize;
          }
          else
          {
            CMemBlocks2 &memRef = refs.Refs[threadInfo.UpdateIndex];
            threadInfo.OutStreamSpec->DetachData(memRef);
            memRef.CompressingResult = threadInfo.CompressingResult;
            memRef.Defined = true;
            continue;
          }
        }
      }
    }
    else
    {
      progressMainSpec->Reinit();
      RINOK(UpdateItemOldData(archive, inStream, updateItem, item, progressMain, complexity));
    }
    items.Add(item);
    complexity += NFileHeader::kLocalBlockSize;
    itemIndex++;
  }
  archive.WriteCentralDir(items, comment);
  return S_OK;
  #endif  
}

HRESULT Update(    
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    CInArchive *inArchive,
    CCompressionMethodMode *compressionMethodMode,
    IArchiveUpdateCallback *updateCallback)
{
  CMyComPtr<IOutStream> outStream;
  RINOK(seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStream));
  if (!outStream)
    return E_NOTIMPL;

  CInArchiveInfo archiveInfo;
  if(inArchive != 0)
  {
    inArchive->GetArchiveInfo(archiveInfo);
    if (archiveInfo.Base != 0)
      return E_NOTIMPL;
  }
  else
    archiveInfo.StartPosition = 0;
  
  COutArchive outArchive;
  outArchive.Create(outStream);
  if (archiveInfo.StartPosition > 0)
  {
    CMyComPtr<ISequentialInStream> inStream;
    inStream.Attach(inArchive->CreateLimitedStream(0, archiveInfo.StartPosition));
    RINOK(CopyBlockToArchive(inStream, outArchive, NULL));
    outArchive.MoveBasePosition(archiveInfo.StartPosition);
  }
  CMyComPtr<IInStream> inStream;
  if(inArchive != 0)
    inStream.Attach(inArchive->CreateStream());

  return Update2(
      EXTERNAL_CODECS_LOC_VARS
      outArchive, inArchive, inStream, 
      inputItems, updateItems, 
      compressionMethodMode, 
      archiveInfo.Comment, updateCallback);
}

}}
