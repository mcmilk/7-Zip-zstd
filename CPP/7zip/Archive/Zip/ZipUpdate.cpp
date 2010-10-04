// ZipUpdate.cpp

#include "StdAfx.h"

#include "../../../../C/Alloc.h"

#include "Common/AutoPtr.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/Thread.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/OutMemStream.h"
#include "../../Common/ProgressUtils.h"
#ifndef _7ZIP_ST
#include "../../Common/ProgressMt.h"
#endif
#include "../../Common/StreamUtils.h"

#include "../../Compress/CopyCoder.h"

#include "ZipAddCommon.h"
#include "ZipOut.h"
#include "ZipUpdate.h"

using namespace NWindows;
using namespace NSynchronization;

namespace NArchive {
namespace NZip {

static const Byte kHostOS =
  #ifdef _WIN32
  NFileHeader::NHostOS::kFAT;
  #else
  NFileHeader::NHostOS::kUnix;
  #endif

static const Byte kMadeByHostOS = kHostOS;
static const Byte kExtractHostOS = kHostOS;

static const Byte kMethodForDirectory = NFileHeader::NCompressionMethod::kStored;

static HRESULT CopyBlockToArchive(ISequentialInStream *inStream,
    COutArchive &outArchive, ICompressProgressInfo *progress)
{
  CMyComPtr<ISequentialOutStream> outStream;
  outArchive.CreateStreamForCopying(&outStream);
  return NCompress::CopyStream(inStream, outStream, progress);
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
    const CUpdateItem &ui,
    CItem &item)
{
  item.UnPackSize = ui.Size;
  bool isDir;

  item.ClearFlags();

  if (ui.NewProperties)
  {
    isDir = ui.IsDir;
    item.Name = ui.Name;
    item.SetUtf8(ui.IsUtf8);
    item.ExternalAttributes = ui.Attributes;
    item.Time = ui.Time;
    item.NtfsMTime = ui.NtfsMTime;
    item.NtfsATime = ui.NtfsATime;
    item.NtfsCTime = ui.NtfsCTime;
    item.NtfsTimeIsDefined = ui.NtfsTimeIsDefined;
  }
  else
    isDir = item.IsDir();

  item.LocalHeaderPosition = archive.GetCurrentPosition();
  item.MadeByVersion.HostOS = kMadeByHostOS;
  item.MadeByVersion.Version = NFileHeader::NCompressionMethod::kMadeByProgramVersion;
  
  item.ExtractVersion.HostOS = kExtractHostOS;

  item.InternalAttributes = 0; // test it
  item.SetEncrypted(!isDir && options.PasswordIsDefined);
  if (isDir)
  {
    item.ExtractVersion.Version = NFileHeader::NCompressionMethod::kExtractVersion_Dir;
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

#ifndef _7ZIP_ST

static THREAD_FUNC_DECL CoderThread(void *threadCoderInfo);

struct CThreadInfo
{
  #ifdef EXTERNAL_CODECS
  CMyComPtr<ICompressCodecsInfo> _codecsInfo;
  const CObjectVector<CCodecInfoEx> *_externalCodecs;
  #endif

  NWindows::CThread Thread;
  NWindows::NSynchronization::CAutoResetEvent CompressEvent;
  NWindows::NSynchronization::CAutoResetEvent CompressionCompletedEvent;
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
      ExitThread(false),
      ProgressSpec(0),
      OutStreamSpec(0),
      Coder(options)
  {}
  
  HRESULT CreateEvents()
  {
    RINOK(CompressEvent.CreateIfNotCreated());
    return CompressionCompletedEvent.CreateIfNotCreated();
  }
  HRes CreateThread() { return Thread.Create(CoderThread, this); }

  void WaitAndCode();
  void StopWaitClose()
  {
    ExitThread = true;
    if (OutStreamSpec != 0)
      OutStreamSpec->StopWriting(E_ABORT);
    if (CompressEvent.IsCreated())
      CompressEvent.Set();
    Thread.Wait();
    Thread.Close();
  }

};

void CThreadInfo::WaitAndCode()
{
  for (;;)
  {
    CompressEvent.Lock();
    if (ExitThread)
      return;
    Result = Coder.Compress(
        #ifdef EXTERNAL_CODECS
        _codecsInfo, _externalCodecs,
        #endif
        InStream, OutStream, Progress, CompressingResult);
    if (Result == S_OK && Progress)
      Result = Progress->SetRatioInfo(&CompressingResult.UnpackSize, &CompressingResult.PackSize);
    CompressionCompletedEvent.Set();
  }
}

static THREAD_FUNC_DECL CoderThread(void *threadCoderInfo)
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

class CMtProgressMixer2:
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  UInt64 ProgressOffset;
  UInt64 InSizes[2];
  UInt64 OutSizes[2];
  CMyComPtr<IProgress> Progress;
  CMyComPtr<ICompressProgressInfo> RatioProgress;
  bool _inSizeIsMain;
public:
  NWindows::NSynchronization::CCriticalSection CriticalSection;
  MY_UNKNOWN_IMP
  void Create(IProgress *progress, bool inSizeIsMain);
  void SetProgressOffset(UInt64 progressOffset);
  HRESULT SetRatioInfo(int index, const UInt64 *inSize, const UInt64 *outSize);
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);
};

void CMtProgressMixer2::Create(IProgress *progress, bool inSizeIsMain)
{
  Progress = progress;
  Progress.QueryInterface(IID_ICompressProgressInfo, &RatioProgress);
  _inSizeIsMain = inSizeIsMain;
  ProgressOffset = InSizes[0] = InSizes[1] = OutSizes[0] = OutSizes[1] = 0;
}

void CMtProgressMixer2::SetProgressOffset(UInt64 progressOffset)
{
  CriticalSection.Enter();
  InSizes[1] = OutSizes[1] = 0;
  ProgressOffset = progressOffset;
  CriticalSection.Leave();
}

HRESULT CMtProgressMixer2::SetRatioInfo(int index, const UInt64 *inSize, const UInt64 *outSize)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
  if (index == 0 && RatioProgress)
  {
    RINOK(RatioProgress->SetRatioInfo(inSize, outSize));
  }
  if (inSize != 0)
    InSizes[index] = *inSize;
  if (outSize != 0)
    OutSizes[index] = *outSize;
  UInt64 v = ProgressOffset + (_inSizeIsMain  ?
      (InSizes[0] + InSizes[1]) :
      (OutSizes[0] + OutSizes[1]));
  return Progress->SetCompleted(&v);
}

STDMETHODIMP CMtProgressMixer2::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  return SetRatioInfo(0, inSize, outSize);
}

class CMtProgressMixer:
  public ICompressProgressInfo,
  public CMyUnknownImp
{
public:
  CMtProgressMixer2 *Mixer2;
  CMyComPtr<ICompressProgressInfo> RatioProgress;
  void Create(IProgress *progress, bool inSizeIsMain);
  MY_UNKNOWN_IMP
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);
};

void CMtProgressMixer::Create(IProgress *progress, bool inSizeIsMain)
{
  Mixer2 = new CMtProgressMixer2;
  RatioProgress = Mixer2;
  Mixer2->Create(progress, inSizeIsMain);
}

STDMETHODIMP CMtProgressMixer::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  return Mixer2->SetRatioInfo(1, inSize, outSize);
}


#endif


static HRESULT UpdateItemOldData(COutArchive &archive,
    IInStream *inStream,
    const CUpdateItem &ui, CItemEx &item,
    /* bool izZip64, */
    ICompressProgressInfo *progress,
    UInt64 &complexity)
{
  if (ui.NewProperties)
  {
    if (item.HasDescriptor())
      return E_NOTIMPL;
    
    // use old name size.
    // CUpdateRange range(item.GetLocalExtraPosition(), item.LocalExtraSize + item.PackSize);
    CUpdateRange range(item.GetDataPosition(), item.PackSize);
    
    // item.ExternalAttributes = ui.Attributes;
    // Test it
    item.Name = ui.Name;
    item.SetUtf8(ui.IsUtf8);
    item.Time = ui.Time;
    item.NtfsMTime = ui.NtfsMTime;
    item.NtfsATime = ui.NtfsATime;
    item.NtfsCTime = ui.NtfsCTime;
    item.NtfsTimeIsDefined = ui.NtfsTimeIsDefined;

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

static void WriteDirHeader(COutArchive &archive, const CCompressionMethodMode *options,
    const CUpdateItem &ui, CItemEx &item)
{
  SetFileHeader(archive, *options, ui, item);
  archive.PrepareWriteCompressedData((UInt16)item.Name.Length(), ui.Size, options->IsAesMode);
  archive.WriteLocalHeader(item);
}

static HRESULT Update2St(
    DECL_EXTERNAL_CODECS_LOC_VARS
    COutArchive &archive,
    CInArchive *inArchive,
    IInStream *inStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    const CCompressionMethodMode *options,
    const CByteBuffer *comment,
    IArchiveUpdateCallback *updateCallback)
{
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);

  CAddCommon compressor(*options);
  
  CObjectVector<CItem> items;
  UInt64 unpackSizeTotal = 0, packSizeTotal = 0;

  for (int itemIndex = 0; itemIndex < updateItems.Size(); itemIndex++)
  {
    lps->InSize = unpackSizeTotal;
    lps->OutSize = packSizeTotal;
    RINOK(lps->SetCur());
    const CUpdateItem &ui = updateItems[itemIndex];
    CItemEx item;
    if (!ui.NewProperties || !ui.NewData)
    {
      item = inputItems[ui.IndexInArchive];
      if (inArchive->ReadLocalItemAfterCdItemFull(item) != S_OK)
        return E_NOTIMPL;
    }

    if (ui.NewData)
    {
      bool isDir = ((ui.NewProperties) ? ui.IsDir : item.IsDir());
      if (isDir)
      {
        WriteDirHeader(archive, options, ui, item);
      }
      else
      {
        CMyComPtr<ISequentialInStream> fileInStream;
        HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          lps->ProgressOffset += ui.Size;
          RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
          continue;
        }
        RINOK(res);

        // file Size can be 64-bit !!!
        SetFileHeader(archive, *options, ui, item);
        archive.PrepareWriteCompressedData((UInt16)item.Name.Length(), ui.Size, options->IsAesMode);
        CCompressingResult compressingResult;
        CMyComPtr<IOutStream> outStream;
        archive.CreateStreamForCompressing(&outStream);
        RINOK(compressor.Compress(
            EXTERNAL_CODECS_LOC_VARS
            fileInStream, outStream, progress, compressingResult));
        SetItemInfoFromCompressingResult(compressingResult, options->IsAesMode, options->AesKeyMode, item);
        archive.WriteLocalHeader(item);
        RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
        unpackSizeTotal += item.UnPackSize;
        packSizeTotal += item.PackSize;
      }
    }
    else
    {
      UInt64 complexity = 0;
      lps->SendRatio = false;
      RINOK(UpdateItemOldData(archive, inStream, ui, item, progress, complexity));
      lps->SendRatio = true;
      lps->ProgressOffset += complexity;
    }
    items.Add(item);
    lps->ProgressOffset += NFileHeader::kLocalBlockSize;
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
    const CByteBuffer *comment,
    IArchiveUpdateCallback *updateCallback)
{
  UInt64 complexity = 0;
  UInt64 numFilesToCompress = 0;
  UInt64 numBytesToCompress = 0;
 
  int i;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (ui.NewData)
    {
      complexity += ui.Size;
      numBytesToCompress += ui.Size;
      numFilesToCompress++;
      /*
      if (ui.Commented)
        complexity += ui.CommentRange.Size;
      */
    }
    else
    {
      CItemEx inputItem = inputItems[ui.IndexInArchive];
      if (inArchive->ReadLocalItemAfterCdItemFull(inputItem) != S_OK)
        return E_NOTIMPL;
      complexity += inputItem.GetLocalFullSize();
      // complexity += inputItem.GetCentralExtraPlusCommentSize();
    }
    complexity += NFileHeader::kLocalBlockSize;
    complexity += NFileHeader::kCentralBlockSize;
  }

  if (comment)
    complexity += comment->GetCapacity();
  complexity++; // end of central
  updateCallback->SetTotal(complexity);

  CAddCommon compressor(*options);
  
  complexity = 0;
  
  #ifndef _7ZIP_ST

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
    if (method == NFileHeader::NCompressionMethod::kLZMA)
    {
      UInt32 numLZMAThreads = (options->Algo > 0 ? 2 : 1);
      numThreads /= numLZMAThreads;
      options2.NumThreads = numLZMAThreads;
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


  #ifndef _7ZIP_ST

  CObjectVector<CItem> items;

  CMtProgressMixer *mtProgressMixerSpec = new CMtProgressMixer;
  CMyComPtr<ICompressProgressInfo> progress = mtProgressMixerSpec;
  mtProgressMixerSpec->Create(updateCallback, true);

  CMtCompressProgressMixer mtCompressProgressMixer;
  mtCompressProgressMixer.Init(numThreads, mtProgressMixerSpec->RatioProgress);

  CMemBlockManagerMt memManager(kBlockSize);
  CMemRefs refs(&memManager);

  CThreads threads;
  CRecordVector<HANDLE> compressingCompletedEvents;
  CRecordVector<int> threadIndices;  // list threads in order of updateItems

  {
    RINOK(memManager.AllocateSpaceAlways((size_t)numThreads * (kMemPerThread / kBlockSize)));
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
      RINOK(threadInfo.CreateEvents());
      threadInfo.OutStreamSpec = new COutMemStream(&memManager);
      RINOK(threadInfo.OutStreamSpec->CreateEvents());
      threadInfo.OutStream = threadInfo.OutStreamSpec;
      threadInfo.IsFree = true;
      threadInfo.ProgressSpec = new CMtCompressProgress();
      threadInfo.Progress = threadInfo.ProgressSpec;
      threadInfo.ProgressSpec->Init(&mtCompressProgressMixer, (int)i);
      RINOK(threadInfo.CreateThread());
    }
  }
  int mtItemIndex = 0;

  int itemIndex = 0;
  int lastRealStreamItemIndex = -1;

  while (itemIndex < updateItems.Size())
  {
    if ((UInt32)threadIndices.Size() < numThreads && mtItemIndex < updateItems.Size())
    {
      const CUpdateItem &ui = updateItems[mtItemIndex++];
      if (!ui.NewData)
        continue;
      CItemEx item;
      if (ui.NewProperties)
      {
        if (ui.IsDir)
          continue;
      }
      else
      {
        item = inputItems[ui.IndexInArchive];
        if (inArchive->ReadLocalItemAfterCdItemFull(item) != S_OK)
          return E_NOTIMPL;
        if (item.IsDir())
          continue;
      }
      CMyComPtr<ISequentialInStream> fileInStream;
      {
        NWindows::NSynchronization::CCriticalSectionLock lock(mtProgressMixerSpec->Mixer2->CriticalSection);
        HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          complexity += ui.Size;
          complexity += NFileHeader::kLocalBlockSize;
          mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
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
          threadInfo.CompressEvent.Set();
          threadInfo.UpdateIndex = mtItemIndex - 1;

          compressingCompletedEvents.Add(threadInfo.CompressionCompletedEvent);
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

    const CUpdateItem &ui = updateItems[itemIndex];

    CItemEx item;
    if (!ui.NewProperties || !ui.NewData)
    {
      item = inputItems[ui.IndexInArchive];
      if (inArchive->ReadLocalItemAfterCdItemFull(item) != S_OK)
        return E_NOTIMPL;
    }

    if (ui.NewData)
    {
      bool isDir = ((ui.NewProperties) ? ui.IsDir : item.IsDir());
      if (isDir)
      {
        WriteDirHeader(archive, options, ui, item);
      }
      else
      {
        if (lastRealStreamItemIndex < itemIndex)
        {
          lastRealStreamItemIndex = itemIndex;
          SetFileHeader(archive, *options, ui, item);
          // file Size can be 64-bit !!!
          archive.PrepareWriteCompressedData((UInt16)item.Name.Length(), ui.Size, options->IsAesMode);
        }

        CMemBlocks2 &memRef = refs.Refs[itemIndex];
        if (memRef.Defined)
        {
          CMyComPtr<IOutStream> outStream;
          archive.CreateStreamForCompressing(&outStream);
          memRef.WriteToStream(memManager.GetBlockSize(), outStream);
          SetItemInfoFromCompressingResult(memRef.CompressingResult,
              options->IsAesMode, options->AesKeyMode, item);
          SetFileHeader(archive, *options, ui, item);
          archive.WriteLocalHeader(item);
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
            SetFileHeader(archive, *options, ui, item);
            archive.WriteLocalHeader(item);
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
      RINOK(UpdateItemOldData(archive, inStream, ui, item, progress, complexity));
    }
    items.Add(item);
    complexity += NFileHeader::kLocalBlockSize;
    mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
    itemIndex++;
  }
  archive.WriteCentralDir(items, comment);
  return S_OK;
  #endif
}

static const size_t kCacheBlockSize = (1 << 20);
static const size_t kCacheSize = (kCacheBlockSize << 2);
static const size_t kCacheMask = (kCacheSize - 1);

class CCacheOutStream:
  public IOutStream,
  public CMyUnknownImp
{
  CMyComPtr<IOutStream> _stream;
  Byte *_cache;
  UInt64 _virtPos;
  UInt64 _virtSize;
  UInt64 _phyPos;
  UInt64 _phySize; // <= _virtSize
  UInt64 _cachedPos; // (_cachedPos + _cachedSize) <= _virtSize
  size_t _cachedSize;

  HRESULT MyWrite(size_t size);
  HRESULT MyWriteBlock()
  {
    return MyWrite(kCacheBlockSize - ((size_t)_cachedPos & (kCacheBlockSize - 1)));
  }
  HRESULT FlushCache();
public:
  CCacheOutStream(): _cache(0) {}
  ~CCacheOutStream();
  bool Allocate();
  HRESULT Init(IOutStream *stream);
  
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(UInt64 newSize);
};

bool CCacheOutStream::Allocate()
{
  if (!_cache)
    _cache = (Byte *)::MidAlloc(kCacheSize);
  return (_cache != NULL);
}

HRESULT CCacheOutStream::Init(IOutStream *stream)
{
  _virtPos = _phyPos = 0;
  _stream = stream;
  RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &_virtPos));
  RINOK(_stream->Seek(0, STREAM_SEEK_END, &_virtSize));
  RINOK(_stream->Seek(_virtPos, STREAM_SEEK_SET, &_virtPos));
  _phyPos = _virtPos;
  _phySize = _virtSize;
  _cachedPos = 0;
  _cachedSize = 0;
  return S_OK;
}

HRESULT CCacheOutStream::MyWrite(size_t size)
{
  while (size != 0 && _cachedSize != 0)
  {
    if (_phyPos != _cachedPos)
    {
      RINOK(_stream->Seek(_cachedPos, STREAM_SEEK_SET, &_phyPos));
    }
    size_t pos = (size_t)_cachedPos & kCacheMask;
    size_t curSize = MyMin(kCacheSize - pos, _cachedSize);
    curSize = MyMin(curSize, size);
    RINOK(WriteStream(_stream, _cache + pos, curSize));
    _phyPos += curSize;
    if (_phySize < _phyPos)
      _phySize = _phyPos;
    _cachedPos += curSize;
    _cachedSize -= curSize;
    size -= curSize;
  }
  return S_OK;
}

HRESULT CCacheOutStream::FlushCache()
{
  return MyWrite(_cachedSize);
}

CCacheOutStream::~CCacheOutStream()
{
  FlushCache();
  if (_virtSize != _phySize)
    _stream->SetSize(_virtSize);
  if (_virtPos != _phyPos)
    _stream->Seek(_virtPos, STREAM_SEEK_SET, NULL);
  ::MidFree(_cache);
}

STDMETHODIMP CCacheOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;

  UInt64 zerosStart = _virtPos;
  if (_cachedSize != 0)
  {
    if (_virtPos < _cachedPos)
    {
      RINOK(FlushCache());
    }
    else
    {
      UInt64 cachedEnd = _cachedPos + _cachedSize;
      if (cachedEnd < _virtPos)
      {
        if (cachedEnd < _phySize)
        {
          RINOK(FlushCache());
        }
        else
          zerosStart = cachedEnd;
      }
    }
  }

  if (_cachedSize == 0 && _phySize < _virtPos)
    _cachedPos = zerosStart = _phySize;

  if (zerosStart != _virtPos)
  {
    // write zeros to [cachedEnd ... _virtPos)
    
    for (;;)
    {
      UInt64 cachedEnd = _cachedPos + _cachedSize;
      size_t endPos = (size_t)cachedEnd & kCacheMask;
      size_t curSize = kCacheSize - endPos;
      if (curSize > _virtPos - cachedEnd)
        curSize = (size_t)(_virtPos - cachedEnd);
      if (curSize == 0)
        break;
      while (curSize > (kCacheSize - _cachedSize))
      {
        RINOK(MyWriteBlock());
      }
      memset(_cache + endPos, 0, curSize);
      _cachedSize += curSize;
    }
  }

  if (_cachedSize == 0)
    _cachedPos = _virtPos;

  size_t pos = (size_t)_virtPos & kCacheMask;
  size = (UInt32)MyMin((size_t)size, kCacheSize - pos);
  UInt64 cachedEnd = _cachedPos + _cachedSize;
  if (_virtPos != cachedEnd) // _virtPos < cachedEnd
    size = (UInt32)MyMin((size_t)size, (size_t)(cachedEnd - _virtPos));
  else
  {
    // _virtPos == cachedEnd
    if (_cachedSize == kCacheSize)
    {
      RINOK(MyWriteBlock());
    }
    size_t startPos = (size_t)_cachedPos & kCacheMask;
    if (startPos > pos)
      size = (UInt32)MyMin((size_t)size, (size_t)(startPos - pos));
    _cachedSize += size;
  }
  memcpy(_cache + pos, data, size);
  if (processedSize)
    *processedSize = size;
  _virtPos += size;
  if (_virtSize < _virtPos)
    _virtSize = _virtPos;
  return S_OK;
}

STDMETHODIMP CCacheOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch(seekOrigin)
  {
    case STREAM_SEEK_SET: _virtPos = offset; break;
    case STREAM_SEEK_CUR: _virtPos += offset; break;
    case STREAM_SEEK_END: _virtPos = _virtSize + offset; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

STDMETHODIMP CCacheOutStream::SetSize(UInt64 newSize)
{
  _virtSize = newSize;
  if (newSize < _phySize)
  {
    RINOK(_stream->SetSize(newSize));
    _phySize = newSize;
  }
  if (newSize <= _cachedPos)
  {
    _cachedSize = 0;
    _cachedPos = newSize;
  }
  if (newSize < _cachedPos + _cachedSize)
    _cachedSize = (size_t)(newSize - _cachedPos);
  return S_OK;
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
  {
    CMyComPtr<IOutStream> outStreamReal;
    seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStreamReal);
    if (!outStreamReal)
      return E_NOTIMPL;
    CCacheOutStream *cacheStream = new CCacheOutStream();
    outStream = cacheStream;
    if (!cacheStream->Allocate())
      return E_OUTOFMEMORY;
    RINOK(cacheStream->Init(outStreamReal));
  }

  if (inArchive)
  {
    if (inArchive->ArcInfo.Base != 0 ||
        inArchive->ArcInfo.StartPosition != 0 ||
        !inArchive->IsOkHeaders)
      return E_NOTIMPL;
  }
  
  COutArchive outArchive;
  outArchive.Create(outStream);
  /*
  if (inArchive && inArchive->ArcInfo.StartPosition > 0)
  {
    CMyComPtr<ISequentialInStream> inStream;
    inStream.Attach(inArchive->CreateLimitedStream(0, inArchive->ArcInfo.StartPosition));
    RINOK(CopyBlockToArchive(inStream, outArchive, NULL));
    outArchive.MoveBasePosition(inArchive->ArcInfo.StartPosition);
  }
  */
  CMyComPtr<IInStream> inStream;
  if (inArchive)
    inStream.Attach(inArchive->CreateStream());

  return Update2(
      EXTERNAL_CODECS_LOC_VARS
      outArchive, inArchive, inStream,
      inputItems, updateItems,
      compressionMethodMode,
      inArchive ? &inArchive->ArcInfo.Comment : NULL,
      updateCallback);
}

}}
