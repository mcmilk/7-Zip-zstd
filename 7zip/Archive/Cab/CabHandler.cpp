// Cab/Handler.cpp

#include "stdafx.h"

#include "Common/StringConvert.h"
#include "Common/Defs.h"
#include "Common/UTFConvert.h"
#include "Common/ComTry.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "CabCopyDecoder.h"
#include "LZXDecoder.h"
#include "MSZIPDecoder.h"

#include "CabHandler.h"

#include "../../Common/ProgressUtils.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NCab {

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},

  { NULL, kpidMethod, VT_BSTR},
  // { NULL, kpidDictionarySize, VT_UI4},

  { NULL, kpidBlock, VT_UI4}
};

static const int kNumProperties = sizeof(kProperties) / sizeof(kProperties[0]);

static const wchar_t *kMethods[] = 
{
  L"None",
  L"MSZip",
  L"Quantum",
  L"LZX"
};

static const int kNumMethods = sizeof(kMethods) / sizeof(kMethods[0]);
static const wchar_t *kUnknownMethod = L"Unknown";

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  value->vt = VT_EMPTY;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfProperties(UINT32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CHandler::GetPropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  if(index >= sizeof(kProperties) / sizeof(kProperties[0]))
    return E_INVALIDARG;
  const STATPROPSTG &srcItem = kProperties[index];
  *propID = srcItem.propid;
  *varType = srcItem.vt;
  *name = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfArchiveProperties(UINT32 *numProperties)
{
  *numProperties = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetArchivePropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return E_INVALIDARG;
}

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const CItem &fileInfo = m_Files[index];
  switch(propID)
  {
    case kpidPath:
      if (fileInfo.IsNameUTF())
      {
        UString unicodeName;
        if (!ConvertUTF8ToUnicode(fileInfo.Name, unicodeName))
          propVariant = L"";
        else
          propVariant = unicodeName;
      }
      else
        propVariant = MultiByteToUnicodeString(fileInfo.Name, CP_ACP);
      break;
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidSize:
      propVariant = fileInfo.UnPackSize;
      break;
    case kpidLastWriteTime:
    {
      FILETIME localFileTime, utcFileTime;
      if (DosTimeToFileTime(fileInfo.Time, localFileTime))
      {
        if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      }
      else
        utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      propVariant = utcFileTime;
      break;
    }
    case kpidAttributes:
      propVariant = fileInfo.GetWinAttributes();
      break;

    case kpidMethod:
    {
      UINT16 realFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
          m_Folders.Size(), fileInfo.FolderIndex);
      const NHeader::CFolder &folder = m_Folders[realFolderIndex];
      UString method;
      if (folder.CompressionTypeMajor < kNumMethods)
        method = kMethods[folder.CompressionTypeMajor];
      else
        method = kUnknownMethod;
      if (folder.CompressionTypeMajor == NHeader::NCompressionMethodMajor::kLZX)
      {
        method += L":";
        wchar_t temp[32];
        _itow (folder.CompressionTypeMinor, temp, 10);
        method += temp;
      }
      propVariant = method;
      break;
    }
    case kpidBlock:
      propVariant = UINT32(fileInfo.FolderIndex);
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CPropgressImp: public CProgressVirt
{
  CMyComPtr<IArchiveOpenCallback> m_OpenArchiveCallback;
public:
  STDMETHOD(SetTotal)(const UINT64 *numFiles);
  STDMETHOD(SetCompleted)(const UINT64 *numFiles);
  void Init(IArchiveOpenCallback *openArchiveCallback)
    { m_OpenArchiveCallback = openArchiveCallback; }
};

STDMETHODIMP CPropgressImp::SetTotal(const UINT64 *numFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CPropgressImp::SetCompleted(const UINT64 *numFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  m_Stream.Release();
  // try
  {
    CInArchive archive;
    m_Files.Clear();
    CPropgressImp progressImp;
    progressImp.Init(openArchiveCallback);
    RINOK(archive.Open(inStream, maxCheckStartPosition, 
        m_ArchiveInfo, m_Folders, m_Files, &progressImp));
    m_Stream = inStream;
  }
  /*
  catch(...)
  {
    return S_FALSE;
  }
  */
  COM_TRY_END
  return S_OK;
}

STDMETHODIMP CHandler::Close()
{
  m_Stream.Release();
  return S_OK;
}

class CCabFolderOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
private:
  const CObjectVector<NHeader::CFolder> *m_Folders;
  const CObjectVector<CItem> *m_Files;
  const CRecordVector<int> *m_FileIndexes;
  const CRecordVector<bool> *m_ExtractStatuses;
  int m_StartIndex;
  int m_CurrentIndex;
  int m_NumFiles;
  UINT64 m_CurrentDataPos;
  CMyComPtr<IArchiveExtractCallback> m_ExtractCallback;
  bool m_TestMode;

  bool m_FileIsOpen;
  CMyComPtr<ISequentialOutStream> realOutStream;
  UINT64 m_FilePos;

  HRESULT OpenFile(int indexIndex, ISequentialOutStream **realOutStream);
  HRESULT WriteEmptyFiles();
  UINT64 m_StartImportantTotalUnPacked;
public:
  void Init(
      const CObjectVector<NHeader::CFolder> *folders,
      const CObjectVector<CItem> *files, 
      const CRecordVector<int> *fileIndices, 
      const CRecordVector<bool> *extractStatuses, 
      int startIndex, 
      int numFiles, 
      IArchiveExtractCallback *extractCallback,
      UINT64 startImportantTotalUnPacked,
      bool testMode);
  STDMETHOD(FlushCorrupted)();
};

void CCabFolderOutStream::Init(
    const CObjectVector<NHeader::CFolder> *folders,
    const CObjectVector<CItem> *files, 
    const CRecordVector<int> *fileIndices,
    const CRecordVector<bool> *extractStatuses, 
    int startIndex, 
    int numFiles,
    IArchiveExtractCallback *extractCallback,
    UINT64 startImportantTotalUnPacked,
    bool testMode)
{
  m_Folders = folders;
  m_Files = files;
  m_FileIndexes = fileIndices;
  m_ExtractStatuses = extractStatuses;
  m_StartIndex = startIndex;
  m_NumFiles = numFiles;
  m_ExtractCallback = extractCallback;
  m_StartImportantTotalUnPacked = startImportantTotalUnPacked;
  m_TestMode = testMode;

  m_CurrentIndex = 0;
  m_FileIsOpen = false;
}

HRESULT CCabFolderOutStream::OpenFile(int indexIndex, ISequentialOutStream **realOutStream)
{
  // RINOK(m_ExtractCallback->SetCompleted(&m_StartImportantTotalUnPacked));
  
  int fullIndex = m_StartIndex + indexIndex;

  INT32 askMode;
  if((*m_ExtractStatuses)[fullIndex])
    askMode = m_TestMode ? 
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
  else
    askMode = NArchive::NExtract::NAskMode::kSkip;
  
  int index = (*m_FileIndexes)[fullIndex];
  const CItem &fileInfo = (*m_Files)[index];
  UINT16 realFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
      m_Folders->Size(), fileInfo.FolderIndex);

  RINOK(m_ExtractCallback->GetStream(index, realOutStream, askMode));
  
  UINT64 currentUnPackSize = fileInfo.UnPackSize;
  
  bool mustBeProcessedAnywhere = (indexIndex < m_NumFiles - 1);
    
  if (realOutStream || mustBeProcessedAnywhere)
  {
    if (!realOutStream && !m_TestMode)
      askMode = NArchive::NExtract::NAskMode::kSkip;
    RINOK(m_ExtractCallback->PrepareOperation(askMode));
    return S_OK;
  }
  else
    return S_FALSE;
}


HRESULT CCabFolderOutStream::WriteEmptyFiles()
{
  for(;m_CurrentIndex < m_NumFiles; m_CurrentIndex++)
  {
    int index = (*m_FileIndexes)[m_StartIndex + m_CurrentIndex];
    const CItem &fileInfo = (*m_Files)[index];
    if (fileInfo.UnPackSize != 0)
      return S_OK;
    realOutStream.Release();
    HRESULT result = OpenFile(m_CurrentIndex, &realOutStream);
    realOutStream.Release();
    if (result == S_FALSE)
    {
    }
    else if (result == S_OK)
    {
      RINOK(m_ExtractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
    }
    else
      return result;
  }
  return S_OK;
}

STDMETHODIMP CCabFolderOutStream::Write(const void *data, 
    UINT32 size, UINT32 *processedSize)
{
  UINT32 processedSizeReal = 0;
  while(m_CurrentIndex < m_NumFiles)
  {
    if (m_FileIsOpen)
    {
      int index = (*m_FileIndexes)[m_StartIndex + m_CurrentIndex];
      const CItem &fileInfo = (*m_Files)[index];
      UINT64 fileSize = fileInfo.UnPackSize;
      
      UINT32 numBytesToWrite = (UINT32)MyMin(fileSize - m_FilePos, 
          UINT64(size - processedSizeReal));
      
      UINT32 processedSizeLocal;
      if (!realOutStream)
      {
        processedSizeLocal = numBytesToWrite;
      }
      else
      {
        RINOK(realOutStream->Write((const BYTE *)data + processedSizeReal, numBytesToWrite, &processedSizeLocal));
      }
      m_FilePos += processedSizeLocal;
      processedSizeReal += processedSizeLocal;
      if (m_FilePos == fileInfo.UnPackSize)
      {
        realOutStream.Release();
        RINOK(m_ExtractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
        m_FileIsOpen = false;
        m_CurrentIndex++;
      }
      if (processedSizeReal == size)
      {
        RINOK(WriteEmptyFiles());
        if (processedSize != NULL)
          *processedSize = processedSizeReal;
        return S_OK;
      }
    }
    else
    {
      HRESULT result = OpenFile(m_CurrentIndex, &realOutStream);
      if (result != S_FALSE && result != S_OK)
        return result;
      m_FileIsOpen = true;
      m_FilePos = 0;
    }
  }
  if (processedSize != NULL)
    *processedSize = size;
  return S_OK;
}

STDMETHODIMP CCabFolderOutStream::FlushCorrupted()
{
  // UINT32 processedSizeReal = 0;
  while(m_CurrentIndex < m_NumFiles)
  {
    if (m_FileIsOpen)
    {
      int index = (*m_FileIndexes)[m_StartIndex + m_CurrentIndex];
      const CItem &fileInfo = (*m_Files)[index];
      UINT64 fileSize = fileInfo.UnPackSize;
      
      realOutStream.Release();
      RINOK(m_ExtractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kCRCError));
      m_FileIsOpen = false;
      m_CurrentIndex++;
    }
    else
    {
      HRESULT result = OpenFile(m_CurrentIndex, &realOutStream);
      if (result != S_FALSE && result != S_OK)
        return result;
      m_FileIsOpen = true;
    }
  }
  return S_OK;
}

STDMETHODIMP CCabFolderOutStream::WritePart(const void *data, 
    UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}


STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UINT32(-1));
  if (allFilesMode)
    numItems = m_Files.Size();
  if(numItems == 0)
    return S_OK;
  bool testMode = (_aTestMode != 0);
  UINT64 censoredTotalUnPacked = 0, importantTotalUnPacked = 0;
  int lastIndex = 0;
  CRecordVector<int> folderIndexes;
  CRecordVector<int> importantIndices;
  CRecordVector<bool> extractStatuses;

  UINT32 i;
  for(i = 0; i < numItems; i++)
  {
    int index = allFilesMode ? i : indices[i];
    const CItem &fileInfo = m_Files[index];
    censoredTotalUnPacked += fileInfo.UnPackSize;

    int folderIndex = fileInfo.FolderIndex;
    if (folderIndexes.IsEmpty())
      folderIndexes.Add(folderIndex);
    else
    {
      if (folderIndex != folderIndexes.Back())
        folderIndexes.Add(folderIndex);
    }

    for(int j = index - 1; j >= lastIndex; j--)
      if(m_Files[j].FolderIndex != folderIndex)
        break;
    for(j++; j <= index; j++)
    {
      const CItem &fileInfo = m_Files[j];
      importantTotalUnPacked += fileInfo.UnPackSize;
      importantIndices.Add(j);
      extractStatuses.Add(j == index);
    }
    lastIndex = index + 1;
  }

  extractCallback->SetTotal(importantTotalUnPacked);
  UINT64 currentImportantTotalUnPacked = 0;
  UINT64 currentImportantTotalPacked = 0;

  CCopyDecoder *storeDecoderSpec = NULL;
  CMyComPtr<ICompressCoder> storeDecoder;

  NMSZip::CDecoder *msZipDecoderSpec = NULL;
  CMyComPtr<ICompressCoder> msZipDecoder;

  NLZX::CDecoder *lzxDecoderSpec = NULL;
  CMyComPtr<ICompressCoder> lzxDecoder;


  int curImportantIndexIndex = 0;
  UINT64 totalFolderUnPacked;
  for(i = 0; i < (UINT32)folderIndexes.Size(); i++, currentImportantTotalUnPacked += totalFolderUnPacked)
  {
    int folderIndex = folderIndexes[i];
    UINT16 realFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
        m_Folders.Size(), folderIndex);

    RINOK(extractCallback->SetCompleted(&currentImportantTotalUnPacked));
    totalFolderUnPacked = 0;
    for (int j = curImportantIndexIndex; j < importantIndices.Size(); j++)
    {
      const CItem &fileInfo = m_Files[importantIndices[j]];
      if (fileInfo.FolderIndex != folderIndex)
        break;
      totalFolderUnPacked += fileInfo.UnPackSize;
    }
    
    CCabFolderOutStream *cabFolderOutStream =  new CCabFolderOutStream;
    CMyComPtr<ISequentialOutStream> outStream(cabFolderOutStream);

    cabFolderOutStream->Init(&m_Folders, &m_Files, &importantIndices, 
        &extractStatuses, curImportantIndexIndex, j - curImportantIndexIndex, 
        extractCallback, currentImportantTotalUnPacked, testMode);

    curImportantIndexIndex = j;
  
    const NHeader::CFolder &folder = m_Folders[realFolderIndex];
    UINT64 pos = folder.DataStart; // test it (+ archiveStart)
    RINOK(m_Stream->Seek(pos, STREAM_SEEK_SET, NULL));

    CLocalProgress *localProgressSpec = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);
   
    CLocalCompressProgressInfo *localCompressProgressSpec = 
        new CLocalCompressProgressInfo;
    CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(progress, 
        NULL, &currentImportantTotalUnPacked);

    BYTE reservedSize = m_ArchiveInfo.ReserveBlockPresent() ? 
      m_ArchiveInfo.PerDataSizes.PerDatablockAreaSize : 0;

    switch(folder.CompressionTypeMajor)
    {
      case NHeader::NCompressionMethodMajor::kNone:
      {
        if(storeDecoderSpec == NULL)
        {
          storeDecoderSpec = new CCopyDecoder;
          storeDecoder = storeDecoderSpec;
        }
        try
        {
          storeDecoderSpec->SetParams(reservedSize, folder.NumDataBlocks);
          RINOK(storeDecoder->Code(m_Stream, outStream,
              NULL, &totalFolderUnPacked, compressProgress));
        }
        catch(...)
        {
          RINOK(cabFolderOutStream->FlushCorrupted());
          continue;
        }
        break;
      }
      case NHeader::NCompressionMethodMajor::kMSZip:
      {
        if(lzxDecoderSpec == NULL)
        {
          msZipDecoderSpec = new NMSZip::CDecoder;
          msZipDecoder = msZipDecoderSpec;
        }
        try
        {
          msZipDecoderSpec->SetParams(reservedSize, folder.NumDataBlocks);
          RINOK(msZipDecoder->Code(m_Stream, outStream,
            NULL, &totalFolderUnPacked, compressProgress));
        }
        catch(...)
        {
          RINOK(cabFolderOutStream->FlushCorrupted());
          continue;
        }
        break;
      }
      case NHeader::NCompressionMethodMajor::kLZX:
      {
        if(lzxDecoderSpec == NULL)
        {
          lzxDecoderSpec = new NLZX::CDecoder;
          lzxDecoder = lzxDecoderSpec;
        }
        try
        {
          lzxDecoderSpec->SetParams(reservedSize, folder.NumDataBlocks, 
              folder.CompressionTypeMinor);
          RINOK(lzxDecoder->Code(m_Stream, outStream,
            NULL, &totalFolderUnPacked, compressProgress));
        }
        catch(...)
        {
          RINOK(cabFolderOutStream->FlushCorrupted());
          continue;
        }
        break;
      }
    default:
      return E_FAIL;
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  COM_TRY_BEGIN
  *numItems = m_Files.Size();
  return S_OK;
  COM_TRY_END
}

}}