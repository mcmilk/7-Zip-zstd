// CabHandler.cpp

#include "StdAfx.h"

#include "../../../../C/Alloc.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "Common/UTFConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamUtils.h"

#include "../../Compress/CopyCoder.h"
#include "../../Compress/DeflateDecoder.h"
#include "../../Compress/LzxDecoder.h"
#include "../../Compress/QuantumDecoder.h"

#include "../Common/ItemNameUtils.h"

#include "CabBlockInStream.h"
#include "CabHandler.h"

using namespace NWindows;

namespace NArchive {
namespace NCab {

// #define _CAB_DETAILS

#ifdef _CAB_DETAILS
enum
{
  kpidBlockReal = kpidUserDefined
};
#endif

static STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidAttrib, VT_UI4},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidBlock, VT_I4}
  #ifdef _CAB_DETAILS
  ,
  { L"BlockReal", kpidBlockReal, VT_UI4},
  { NULL, kpidOffset, VT_UI4},
  { NULL, kpidVolume, VT_UI4}
  #endif
};

static const char *kMethods[] =
{
  "None",
  "MSZip",
  "Quantum",
  "LZX"
};

static const int kNumMethods = sizeof(kMethods) / sizeof(kMethods[0]);
static const char *kUnknownMethod = "Unknown";

static STATPROPSTG kArcProps[] =
{
  { NULL, kpidMethod, VT_BSTR},
  // { NULL, kpidSolid, VT_BOOL},
  { NULL, kpidNumBlocks, VT_UI4},
  { NULL, kpidNumVolumes, VT_UI4}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidMethod:
    {
      AString resString;
      CRecordVector<Byte> ids;
      int i;
      for (int v = 0; v < m_Database.Volumes.Size(); v++)
      {
        const CDatabaseEx &de = m_Database.Volumes[v];
        for (i = 0; i < de.Folders.Size(); i++)
          ids.AddToUniqueSorted(de.Folders[i].GetCompressionMethod());
      }
      for (i = 0; i < ids.Size(); i++)
      {
        Byte id = ids[i];
        AString method = (id < kNumMethods) ? kMethods[id] : kUnknownMethod;
        if (!resString.IsEmpty())
          resString += ' ';
        resString += method;
      }
      prop = resString;
      break;
    }
    // case kpidSolid: prop = _database.IsSolid(); break;
    case kpidNumBlocks:
    {
      UInt32 numFolders = 0;
      for (int v = 0; v < m_Database.Volumes.Size(); v++)
        numFolders += m_Database.Volumes[v].Folders.Size();
      prop = numFolders;
      break;
    }
    case kpidNumVolumes:
    {
      prop = (UInt32)m_Database.Volumes.Size();
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  
  const CMvItem &mvItem = m_Database.Items[index];
  const CDatabaseEx &db = m_Database.Volumes[mvItem.VolumeIndex];
  int itemIndex = mvItem.ItemIndex;
  const CItem &item = db.Items[itemIndex];
  switch(propID)
  {
    case kpidPath:
    {
      UString unicodeName;
      if (item.IsNameUTF())
        ConvertUTF8ToUnicode(item.Name, unicodeName);
      else
        unicodeName = MultiByteToUnicodeString(item.Name, CP_ACP);
      prop = (const wchar_t *)NItemName::WinNameToOSName(unicodeName);
      break;
    }
    case kpidIsDir:  prop = item.IsDir(); break;
    case kpidSize:  prop = item.Size; break;
    case kpidAttrib:  prop = item.GetWinAttributes(); break;

    case kpidMTime:
    {
      FILETIME localFileTime, utcFileTime;
      if (NTime::DosTimeToFileTime(item.Time, localFileTime))
      {
        if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      }
      else
        utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      prop = utcFileTime;
      break;
    }

    case kpidMethod:
    {
      UInt32 realFolderIndex = item.GetFolderIndex(db.Folders.Size());
      const CFolder &folder = db.Folders[realFolderIndex];
      int methodIndex = folder.GetCompressionMethod();
      AString method = (methodIndex < kNumMethods) ? kMethods[methodIndex] : kUnknownMethod;
      if (methodIndex == NHeader::NCompressionMethodMajor::kLZX ||
        methodIndex == NHeader::NCompressionMethodMajor::kQuantum)
      {
        method += ':';
        char temp[32];
        ConvertUInt64ToString(folder.CompressionTypeMinor, temp);
        method += temp;
      }
      prop = method;
      break;
    }
    case kpidBlock:  prop = (Int32)m_Database.GetFolderIndex(&mvItem); break;
    
    #ifdef _CAB_DETAILS
    
    case kpidBlockReal:  prop = (UInt32)item.FolderIndex; break;
    case kpidOffset:  prop = (UInt32)item.Offset; break;
    case kpidVolume:  prop = (UInt32)mvItem.VolumeIndex; break;

    #endif
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

/*
class CProgressImp: public CProgressVirt
{
  CMyComPtr<IArchiveOpenCallback> m_OpenArchiveCallback;
public:
  STDMETHOD(SetTotal)(const UInt64 *numFiles);
  STDMETHOD(SetCompleted)(const UInt64 *numFiles);
  void Init(IArchiveOpenCallback *openArchiveCallback)
    { m_OpenArchiveCallback = openArchiveCallback; }
};

STDMETHODIMP CProgressImp::SetTotal(const UInt64 *numFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CProgressImp::SetCompleted(const UInt64 *numFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(numFiles, NULL);
  return S_OK;
}
*/

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  Close();
  HRESULT res = S_FALSE;
  CInArchive archive;
  CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
  callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
  
  CMyComPtr<IInStream> nextStream = inStream;
  bool prevChecked = false;
  UInt64 numItems = 0;
  try
  {
    while (nextStream != 0)
    {
      CDatabaseEx db;
      db.Stream = nextStream;
      res = archive.Open(maxCheckStartPosition, db);
      if (res == S_OK)
      {
        if (!m_Database.Volumes.IsEmpty())
        {
          const CDatabaseEx &dbPrev = m_Database.Volumes[prevChecked ? m_Database.Volumes.Size() - 1 : 0];
          if (dbPrev.ArchiveInfo.SetID != db.ArchiveInfo.SetID ||
              dbPrev.ArchiveInfo.CabinetNumber + (prevChecked ? 1: - 1) !=
              db.ArchiveInfo.CabinetNumber)
            res = S_FALSE;
        }
      }
      if (res == S_OK)
        m_Database.Volumes.Insert(prevChecked ? m_Database.Volumes.Size() : 0, db);
      else if (res != S_FALSE)
        return res;
      else
      {
        if (m_Database.Volumes.IsEmpty())
          return S_FALSE;
        if (prevChecked)
          break;
        prevChecked = true;
      }

      numItems += db.Items.Size();
      RINOK(callback->SetCompleted(&numItems, NULL));
        
      nextStream = 0;
      for (;;)
      {
        const COtherArchive *otherArchive = 0;
        if (!prevChecked)
        {
          const CInArchiveInfo &ai = m_Database.Volumes.Front().ArchiveInfo;
          if (ai.IsTherePrev())
            otherArchive = &ai.PrevArc;
          else
            prevChecked = true;
        }
        if (otherArchive == 0)
        {
          const CInArchiveInfo &ai = m_Database.Volumes.Back().ArchiveInfo;
          if (ai.IsThereNext())
            otherArchive = &ai.NextArc;
        }
        if (!otherArchive)
          break;
        const UString fullName = MultiByteToUnicodeString(otherArchive->FileName, CP_ACP);
        if (!openVolumeCallback)
          break;

        HRESULT result = openVolumeCallback->GetStream(fullName, &nextStream);
        if (result == S_OK)
          break;
        if (result != S_FALSE)
          return result;
        if (prevChecked)
          break;
        prevChecked = true;
      }
    }
    if (res == S_OK)
    {
      m_Database.FillSortAndShrink();
      if (!m_Database.Check())
        res = S_FALSE;
    }
  }
  catch(...)
  {
    res = S_FALSE;
  }
  if (res != S_OK)
  {
    Close();
    return res;
  }
  COM_TRY_END
  return S_OK;
}

STDMETHODIMP CHandler::Close()
{
  m_Database.Clear();
  return S_OK;
}

class CFolderOutStream:
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
private:
  const CMvDatabaseEx *m_Database;
  const CRecordVector<bool> *m_ExtractStatuses;
  
  Byte *TempBuf;
  UInt32 TempBufSize;
  int NumIdenticalFiles;
  bool TempBufMode;
  UInt32 m_BufStartFolderOffset;

  int m_StartIndex;
  int m_CurrentIndex;
  CMyComPtr<IArchiveExtractCallback> m_ExtractCallback;
  bool m_TestMode;

  CMyComPtr<ISequentialOutStream> m_RealOutStream;

  bool m_IsOk;
  bool m_FileIsOpen;
  UInt32 m_RemainFileSize;
  UInt64 m_FolderSize;
  UInt64 m_PosInFolder;

  void FreeTempBuf()
  {
    ::MyFree(TempBuf);
    TempBuf = NULL;
  }

  HRESULT OpenFile();
  HRESULT CloseFileWithResOp(Int32 resOp);
  HRESULT CloseFile();
  HRESULT Write2(const void *data, UInt32 size, UInt32 *processedSize, bool isOK);
public:
  HRESULT WriteEmptyFiles();

  CFolderOutStream(): TempBuf(NULL) {}
  ~CFolderOutStream() { FreeTempBuf(); }
  void Init(
      const CMvDatabaseEx *database,
      const CRecordVector<bool> *extractStatuses,
      int startIndex,
      UInt64 folderSize,
      IArchiveExtractCallback *extractCallback,
      bool testMode);
  HRESULT FlushCorrupted();
  HRESULT Unsupported();

  UInt64 GetRemain() const { return m_FolderSize - m_PosInFolder; }
  UInt64 GetPosInFolder() const { return m_PosInFolder; }
};

void CFolderOutStream::Init(
    const CMvDatabaseEx *database,
    const CRecordVector<bool> *extractStatuses,
    int startIndex,
    UInt64 folderSize,
    IArchiveExtractCallback *extractCallback,
    bool testMode)
{
  m_Database = database;
  m_ExtractStatuses = extractStatuses;
  m_StartIndex = startIndex;
  m_FolderSize = folderSize;

  m_ExtractCallback = extractCallback;
  m_TestMode = testMode;

  m_CurrentIndex = 0;
  m_PosInFolder = 0;
  m_FileIsOpen = false;
  m_IsOk = true;
  TempBufMode = false;
  NumIdenticalFiles = 0;
}

HRESULT CFolderOutStream::CloseFileWithResOp(Int32 resOp)
{
  m_RealOutStream.Release();
  m_FileIsOpen = false;
  NumIdenticalFiles--;
  return m_ExtractCallback->SetOperationResult(resOp);
}

HRESULT CFolderOutStream::CloseFile()
{
  return CloseFileWithResOp(m_IsOk ?
      NExtract::NOperationResult::kOK:
      NExtract::NOperationResult::kDataError);
}

HRESULT CFolderOutStream::OpenFile()
{
  if (NumIdenticalFiles == 0)
  {
    const CMvItem &mvItem = m_Database->Items[m_StartIndex + m_CurrentIndex];
    const CItem &item = m_Database->Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
    int numExtractItems = 0;
    int curIndex;
    for (curIndex = m_CurrentIndex; curIndex < m_ExtractStatuses->Size(); curIndex++)
    {
      const CMvItem &mvItem2 = m_Database->Items[m_StartIndex + curIndex];
      const CItem &item2 = m_Database->Volumes[mvItem2.VolumeIndex].Items[mvItem2.ItemIndex];
      if (item.Offset != item2.Offset ||
          item.Size != item2.Size ||
          item.Size == 0)
        break;
      if (!m_TestMode && (*m_ExtractStatuses)[curIndex])
        numExtractItems++;
    }
    NumIdenticalFiles = (curIndex - m_CurrentIndex);
    if (NumIdenticalFiles == 0)
      NumIdenticalFiles = 1;
    TempBufMode = false;
    if (numExtractItems > 1)
    {
      if (!TempBuf || item.Size > TempBufSize)
      {
        FreeTempBuf();
        TempBuf = (Byte *)MyAlloc(item.Size);
        TempBufSize = item.Size;
        if (TempBuf == NULL)
          return E_OUTOFMEMORY;
      }
      TempBufMode = true;
      m_BufStartFolderOffset = item.Offset;
    }
    else if (numExtractItems == 1)
    {
      while (NumIdenticalFiles && !(*m_ExtractStatuses)[m_CurrentIndex])
      {
        CMyComPtr<ISequentialOutStream> stream;
        RINOK(m_ExtractCallback->GetStream(m_StartIndex + m_CurrentIndex, &stream, NExtract::NAskMode::kSkip));
        if (stream)
          return E_FAIL;
        RINOK(m_ExtractCallback->PrepareOperation(NExtract::NAskMode::kSkip));
        m_CurrentIndex++;
        m_FileIsOpen = true;
        CloseFile();
      }
    }
  }

  Int32 askMode = (*m_ExtractStatuses)[m_CurrentIndex] ? (m_TestMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract) :
      NExtract::NAskMode::kSkip;
  RINOK(m_ExtractCallback->GetStream(m_StartIndex + m_CurrentIndex, &m_RealOutStream, askMode));
  if (!m_RealOutStream && !m_TestMode)
    askMode = NExtract::NAskMode::kSkip;
  return m_ExtractCallback->PrepareOperation(askMode);
}

HRESULT CFolderOutStream::WriteEmptyFiles()
{
  if (m_FileIsOpen)
    return S_OK;
  for (; m_CurrentIndex < m_ExtractStatuses->Size(); m_CurrentIndex++)
  {
    const CMvItem &mvItem = m_Database->Items[m_StartIndex + m_CurrentIndex];
    const CItem &item = m_Database->Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
    UInt64 fileSize = item.Size;
    if (fileSize != 0)
      return S_OK;
    HRESULT result = OpenFile();
    m_RealOutStream.Release();
    RINOK(result);
    RINOK(m_ExtractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
  }
  return S_OK;
}

// This is Write function
HRESULT CFolderOutStream::Write2(const void *data, UInt32 size, UInt32 *processedSize, bool isOK)
{
  COM_TRY_BEGIN
  UInt32 realProcessed = 0;
  if (processedSize != NULL)
   *processedSize = 0;
  while (size != 0)
  {
    if (m_FileIsOpen)
    {
      UInt32 numBytesToWrite = MyMin(m_RemainFileSize, size);
      HRESULT res = S_OK;
      if (numBytesToWrite > 0)
      {
        if (!isOK)
          m_IsOk = false;
        if (m_RealOutStream)
        {
          UInt32 processedSizeLocal = 0;
          res = m_RealOutStream->Write((const Byte *)data, numBytesToWrite, &processedSizeLocal);
          numBytesToWrite = processedSizeLocal;
        }
        if (TempBufMode && TempBuf)
          memcpy(TempBuf + (m_PosInFolder - m_BufStartFolderOffset), data, numBytesToWrite);
      }
      realProcessed += numBytesToWrite;
      if (processedSize != NULL)
        *processedSize = realProcessed;
      data = (const void *)((const Byte *)data + numBytesToWrite);
      size -= numBytesToWrite;
      m_RemainFileSize -= numBytesToWrite;
      m_PosInFolder += numBytesToWrite;
      if (res != S_OK)
        return res;
      if (m_RemainFileSize == 0)
      {
        RINOK(CloseFile());

        while (NumIdenticalFiles)
        {
          HRESULT result = OpenFile();
          m_FileIsOpen = true;
          m_CurrentIndex++;
          if (result == S_OK && m_RealOutStream && TempBuf)
            result = WriteStream(m_RealOutStream, TempBuf, (size_t)(m_PosInFolder - m_BufStartFolderOffset));
          
          if (!TempBuf && TempBufMode && m_RealOutStream)
          {
            RINOK(CloseFileWithResOp(NExtract::NOperationResult::kUnSupportedMethod));
          }
          else
          {
            RINOK(CloseFile());
          }
          RINOK(result);
        }
        TempBufMode = false;
      }
      if (realProcessed > 0)
        break; // with this break this function works as Write-Part
    }
    else
    {
      if (m_CurrentIndex >= m_ExtractStatuses->Size())
        return E_FAIL;

      const CMvItem &mvItem = m_Database->Items[m_StartIndex + m_CurrentIndex];
      const CItem &item = m_Database->Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];

      m_RemainFileSize = item.Size;

      UInt32 fileOffset = item.Offset;
      if (fileOffset < m_PosInFolder)
        return E_FAIL;
      if (fileOffset > m_PosInFolder)
      {
        UInt32 numBytesToWrite = MyMin(fileOffset - (UInt32)m_PosInFolder, size);
        realProcessed += numBytesToWrite;
        if (processedSize != NULL)
          *processedSize = realProcessed;
        data = (const void *)((const Byte *)data + numBytesToWrite);
        size -= numBytesToWrite;
        m_PosInFolder += numBytesToWrite;
      }
      if (fileOffset == m_PosInFolder)
      {
        RINOK(OpenFile());
        m_FileIsOpen = true;
        m_CurrentIndex++;
        m_IsOk = true;
      }
    }
  }
  return WriteEmptyFiles();
  COM_TRY_END
}

STDMETHODIMP CFolderOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  return Write2(data, size, processedSize, true);
}

HRESULT CFolderOutStream::FlushCorrupted()
{
  const UInt32 kBufferSize = (1 << 10);
  Byte buffer[kBufferSize];
  for (int i = 0; i < kBufferSize; i++)
    buffer[i] = 0;
  for (;;)
  {
    UInt64 remain = GetRemain();
    if (remain == 0)
      return S_OK;
    UInt32 size = (UInt32)MyMin(remain, (UInt64)kBufferSize);
    UInt32 processedSizeLocal = 0;
    RINOK(Write2(buffer, size, &processedSizeLocal, false));
  }
}

HRESULT CFolderOutStream::Unsupported()
{
  while(m_CurrentIndex < m_ExtractStatuses->Size())
  {
    HRESULT result = OpenFile();
    if (result != S_FALSE && result != S_OK)
      return result;
    m_RealOutStream.Release();
    RINOK(m_ExtractCallback->SetOperationResult(NExtract::NOperationResult::kUnSupportedMethod));
    m_CurrentIndex++;
  }
  return S_OK;
}


STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testModeSpec, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = m_Database.Items.Size();
  if(numItems == 0)
    return S_OK;
  bool testMode = (testModeSpec != 0);
  UInt64 totalUnPacked = 0;

  UInt32 i;
  int lastFolder = -2;
  UInt64 lastFolderSize = 0;
  for(i = 0; i < numItems; i++)
  {
    int index = allFilesMode ? i : indices[i];
    const CMvItem &mvItem = m_Database.Items[index];
    const CItem &item = m_Database.Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
    if (item.IsDir())
      continue;
    int folderIndex = m_Database.GetFolderIndex(&mvItem);
    if (folderIndex != lastFolder)
      totalUnPacked += lastFolderSize;
    lastFolder = folderIndex;
    lastFolderSize = item.GetEndOffset();
  }
  totalUnPacked += lastFolderSize;

  extractCallback->SetTotal(totalUnPacked);

  totalUnPacked = 0;

  UInt64 totalPacked = 0;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  NCompress::NDeflate::NDecoder::CCOMCoder *deflateDecoderSpec = NULL;
  CMyComPtr<ICompressCoder> deflateDecoder;

  NCompress::NLzx::CDecoder *lzxDecoderSpec = NULL;
  CMyComPtr<ICompressCoder> lzxDecoder;

  NCompress::NQuantum::CDecoder *quantumDecoderSpec = NULL;
  CMyComPtr<ICompressCoder> quantumDecoder;

  CCabBlockInStream *cabBlockInStreamSpec = new CCabBlockInStream();
  CMyComPtr<ISequentialInStream> cabBlockInStream = cabBlockInStreamSpec;
  if (!cabBlockInStreamSpec->Create())
    return E_OUTOFMEMORY;

  CRecordVector<bool> extractStatuses;
  for(i = 0; i < numItems;)
  {
    int index = allFilesMode ? i : indices[i];

    const CMvItem &mvItem = m_Database.Items[index];
    const CDatabaseEx &db = m_Database.Volumes[mvItem.VolumeIndex];
    int itemIndex = mvItem.ItemIndex;
    const CItem &item = db.Items[itemIndex];

    i++;
    if (item.IsDir())
    {
      Int32 askMode = testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract;
      CMyComPtr<ISequentialOutStream> realOutStream;
      RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
      RINOK(extractCallback->PrepareOperation(askMode));
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }
    int folderIndex = m_Database.GetFolderIndex(&mvItem);
    if (folderIndex < 0)
    {
      // If we need previous archive
      Int32 askMode= testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract;
      CMyComPtr<ISequentialOutStream> realOutStream;
      RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
      RINOK(extractCallback->PrepareOperation(askMode));
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kDataError));
      continue;
    }
    int startIndex2 = m_Database.FolderStartFileIndex[folderIndex];
    int startIndex = startIndex2;
    extractStatuses.Clear();
    for (; startIndex < index; startIndex++)
      extractStatuses.Add(false);
    extractStatuses.Add(true);
    startIndex++;
    UInt64 curUnpack = item.GetEndOffset();
    for(;i < numItems; i++)
    {
      int indexNext = allFilesMode ? i : indices[i];
      const CMvItem &mvItem = m_Database.Items[indexNext];
      const CItem &item = m_Database.Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
      if (item.IsDir())
        continue;
      int newFolderIndex = m_Database.GetFolderIndex(&mvItem);

      if (newFolderIndex != folderIndex)
        break;
      for (; startIndex < indexNext; startIndex++)
        extractStatuses.Add(false);
      extractStatuses.Add(true);
      startIndex++;
      curUnpack = item.GetEndOffset();
    }

    lps->OutSize = totalUnPacked;
    lps->InSize = totalPacked;
    RINOK(lps->SetCur());

    CFolderOutStream *cabFolderOutStream = new CFolderOutStream;
    CMyComPtr<ISequentialOutStream> outStream(cabFolderOutStream);

    const CFolder &folder = db.Folders[item.GetFolderIndex(db.Folders.Size())];

    cabFolderOutStream->Init(&m_Database, &extractStatuses, startIndex2,
        curUnpack, extractCallback, testMode);

    cabBlockInStreamSpec->MsZip = false;
    switch(folder.GetCompressionMethod())
    {
      case NHeader::NCompressionMethodMajor::kNone:
        break;
      case NHeader::NCompressionMethodMajor::kMSZip:
        if(deflateDecoderSpec == NULL)
        {
          deflateDecoderSpec = new NCompress::NDeflate::NDecoder::CCOMCoder;
          deflateDecoder = deflateDecoderSpec;
        }
        cabBlockInStreamSpec->MsZip = true;
        break;
      case NHeader::NCompressionMethodMajor::kLZX:
        if(lzxDecoderSpec == NULL)
        {
          lzxDecoderSpec = new NCompress::NLzx::CDecoder;
          lzxDecoder = lzxDecoderSpec;
        }
        RINOK(lzxDecoderSpec->SetParams(folder.CompressionTypeMinor));
        break;
      case NHeader::NCompressionMethodMajor::kQuantum:
        if(quantumDecoderSpec == NULL)
        {
          quantumDecoderSpec = new NCompress::NQuantum::CDecoder;
          quantumDecoder = quantumDecoderSpec;
        }
        quantumDecoderSpec->SetParams(folder.CompressionTypeMinor);
        break;
      default:
      {
        RINOK(cabFolderOutStream->Unsupported());
        totalUnPacked += curUnpack;
        continue;
      }
    }

    cabBlockInStreamSpec->InitForNewFolder();

    HRESULT res = S_OK;

    {
      int volIndex = mvItem.VolumeIndex;
      int locFolderIndex = item.GetFolderIndex(db.Folders.Size());
      bool keepHistory = false;
      bool keepInputBuffer = false;
      for (UInt32 f = 0; cabFolderOutStream->GetRemain() != 0;)
      {
        if (volIndex >= m_Database.Volumes.Size())
        {
          res = S_FALSE;
          break;
        }

        const CDatabaseEx &db = m_Database.Volumes[volIndex];
        const CFolder &folder = db.Folders[locFolderIndex];
        if (f == 0)
        {
          cabBlockInStreamSpec->SetStream(db.Stream);
          cabBlockInStreamSpec->ReservedSize = db.ArchiveInfo.GetDataBlockReserveSize();
          RINOK(db.Stream->Seek(db.StartPosition + folder.DataStart, STREAM_SEEK_SET, NULL));
        }
        if (f == folder.NumDataBlocks)
        {
          volIndex++;
          locFolderIndex = 0;
          f = 0;
          continue;
        }
        f++;

        cabBlockInStreamSpec->DataError = false;
        
        if (!keepInputBuffer)
          cabBlockInStreamSpec->InitForNewBlock();

        UInt32 packSize, unpackSize;
        res = cabBlockInStreamSpec->PreRead(packSize, unpackSize);
        if (res == S_FALSE)
          break;
        RINOK(res);
        keepInputBuffer = (unpackSize == 0);
        if (keepInputBuffer)
          continue;

        UInt64 totalUnPacked2 = totalUnPacked + cabFolderOutStream->GetPosInFolder();
        totalPacked += packSize;

        lps->OutSize = totalUnPacked2;
        lps->InSize = totalPacked;
        RINOK(lps->SetCur());

        UInt64 unpackRemain = cabFolderOutStream->GetRemain();

        const UInt32 kBlockSizeMax = (1 << 15);
        if (unpackRemain > kBlockSizeMax)
          unpackRemain = kBlockSizeMax;
        if (unpackRemain > unpackSize)
          unpackRemain  = unpackSize;
   
        switch(folder.GetCompressionMethod())
        {
          case NHeader::NCompressionMethodMajor::kNone:
            res = copyCoder->Code(cabBlockInStream, outStream, NULL, &unpackRemain, NULL);
            break;
          case NHeader::NCompressionMethodMajor::kMSZip:
            deflateDecoderSpec->SetKeepHistory(keepHistory);
            res = deflateDecoder->Code(cabBlockInStream, outStream, NULL, &unpackRemain, NULL);
            break;
          case NHeader::NCompressionMethodMajor::kLZX:
            lzxDecoderSpec->SetKeepHistory(keepHistory);
            res = lzxDecoder->Code(cabBlockInStream, outStream, NULL, &unpackRemain, NULL);
            break;
          case NHeader::NCompressionMethodMajor::kQuantum:
            quantumDecoderSpec->SetKeepHistory(keepHistory);
            res = quantumDecoder->Code(cabBlockInStream, outStream, NULL, &unpackRemain, NULL);
          break;
        }
        if (res != S_OK)
        {
          if (res != S_FALSE)
            RINOK(res);
          break;
        }
        keepHistory = true;
      }
      if (res == S_OK)
      {
        RINOK(cabFolderOutStream->WriteEmptyFiles());
      }
    }
    if (res != S_OK || cabFolderOutStream->GetRemain() != 0)
    {
      RINOK(cabFolderOutStream->FlushCorrupted());
    }
    totalUnPacked += curUnpack;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = m_Database.Items.Size();
  return S_OK;
}

}}
