// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "UpdateEngine.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"

#include "Windows/Defs.h"

#include "AddCommon.h"
#include "Handler.h"

#include "../Common/InStreamWithCRC.h"

#include "Interface/LimitedStreams.h"

using namespace std;

namespace NArchive {
namespace NZip {

static const kOneItemComplexity = 30;

static const BYTE kMadeByHostOS = NFileHeader::NHostOS::kFAT;
static const BYTE kExtractHostOS = NFileHeader::NHostOS::kFAT;

static const BYTE kMethodForDirectory = NFileHeader::NCompressionMethod::kStored;
static const BYTE kExtractVersionForDirectory = NFileHeader::NCompressionMethod::kStoreExtractVersion;

HRESULT CopyBlock(ISequentialInStream *anInStream, 
    ISequentialOutStream *anOutStream, ICompressProgressInfo *aProgress)
{
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> aCopyCoder = aCopyCoderSpec;

  return aCopyCoder->Code(anInStream, anOutStream, NULL, NULL, aProgress);
}

HRESULT CopyBlockToArchive(ISequentialInStream *anInStream, 
    COutArchive &anOutArchive, ICompressProgressInfo *aProgress)
{
  CComPtr<ISequentialOutStream> anOutStream;
  anOutArchive.CreateStreamForCopying(&anOutStream);
  return CopyBlock(anInStream, anOutStream, aProgress);
}

static HRESULT WriteRange(IInStream *anInStream, 
    COutArchive &anOutArchive, 
    const CUpdateRange &aRange, 
    IProgress *aProgress,
    UINT64 &aCurrentComplexity)
{
  UINT64 aPosition;
  anInStream->Seek(aRange.Position, STREAM_SEEK_SET, &aPosition);

  CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
      CComObjectNoLock<CLimitedSequentialInStream>;
  CComPtr<CLimitedSequentialInStream> aInStreamLimited(aStreamSpec);
  aStreamSpec->Init(anInStream, aRange.Size);


  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(aProgress, true);
  
  CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
  CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;

  aLocalCompressProgressSpec->Init(aLocalProgress, &aCurrentComplexity, &aCurrentComplexity);

  HRESULT aResult = CopyBlockToArchive(aInStreamLimited, anOutArchive, aCompressProgress);
  aCurrentComplexity += aRange.Size;
  return aResult;
}


HRESULT UpdateOneFile(IInStream *anInStream, 
    COutArchive &anArchive, 
    const CCompressionMethodMode &anOptions,
    CAddCommon &aCompressor, 
    const CUpdateItemInfo &anUpdateItem, 
    UINT64 &aCurrentComplexity,
    IUpdateCallBack *anUpdateCallBack,
    CItemInfoEx &aFileHeaderInfo)
{
  CComPtr<IInStream> aFileInStream;
  RETURN_IF_NOT_S_OK(anUpdateCallBack->CompressOperation(
      anUpdateItem.IndexInClient, &aFileInStream));

  UINT32 aFileSize = anUpdateItem.Size;
  anArchive.PrepareWriteCompressedData(anUpdateItem.Name.Length());

  bool anIsDirectory = anUpdateItem.IsDirectory();
  aFileHeaderInfo.LocalHeaderPosition = anArchive.GetCurrentPosition();
  aFileHeaderInfo.MadeByVersion.HostOS = kMadeByHostOS;
  aFileHeaderInfo.MadeByVersion.Version = NFileHeader::NCompressionMethod::kMadeByProgramVersion;
  
  aFileHeaderInfo.ExtractVersion.HostOS = kExtractHostOS;

  aFileHeaderInfo.InternalAttributes = 0; // test it
  aFileHeaderInfo.ClearFlags();
  aFileHeaderInfo.Name = anUpdateItem.Name; 
  if(anIsDirectory)
  {
    aFileHeaderInfo.ExtractVersion.Version = kExtractVersionForDirectory;
    aFileHeaderInfo.CompressionMethod = kMethodForDirectory;

    aFileHeaderInfo.PackSize = 0;
    aFileHeaderInfo.FileCRC = 0; // test it
  }
  else
  {
    {
      CComObjectNoLock<CInStreamWithCRC> *anInStreamSpec = 
        new CComObjectNoLock<CInStreamWithCRC>;
      CComPtr<IInStream> anInStream(anInStreamSpec);
      anInStreamSpec->Init(aFileInStream);
      CCompressingResult aCompressingResult;
      CComPtr<IOutStream> anOutStream;
      anArchive.CreateStreamForCompressing(&anOutStream);

      CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = 
          new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
      aLocalProgressSpec->Init(anUpdateCallBack, true);
  
      CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;

      aLocalCompressProgressSpec->Init(aLocalProgress, &aCurrentComplexity, NULL);

      RETURN_IF_NOT_S_OK(aCompressor.Compress(anInStream, anOutStream, 
          aFileSize, aCompressProgress, aCompressingResult));

      aFileHeaderInfo.PackSize = aCompressingResult.PackSize;
      aFileHeaderInfo.CompressionMethod = aCompressingResult.Method;
      aFileHeaderInfo.ExtractVersion.Version = aCompressingResult.ExtractVersion;
      aFileHeaderInfo.FileCRC = anInStreamSpec->GetCRC();
    }
  }
  aFileHeaderInfo.UnPackSize = aFileSize;
  aFileHeaderInfo.SetEncrypted(anOptions.PasswordIsDefined);
  aFileHeaderInfo.CommentSize = (anUpdateItem.Commented) ? 
      WORD(anUpdateItem.CommentRange.Size) : 0;

  aFileHeaderInfo.LocalExtraSize = 0;
  aFileHeaderInfo.CentralExtraSize = 0;

  aFileHeaderInfo.ExternalAttributes = anUpdateItem.Attributes;
  aFileHeaderInfo.Time = anUpdateItem.Time;

  anArchive.WriteLocalHeader(aFileHeaderInfo);
  aCurrentComplexity += aFileSize;
  return anUpdateCallBack->OperationResult(
      NArchiveHandler::NUpdate::NOperationResult::kOK);
}

HRESULT UpdateArchiveStd(COutArchive &anArchive, 
    IInStream *anInStream,
    const NArchive::NZip::CItemInfoExVector &anInputItems,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    const CCompressionMethodMode *anOptions, 
    bool aCommentRangeAssigned,
    const CUpdateRange &aCommentRange,
    IUpdateCallBack *anUpdateCallBack)
{
  UINT64 aComplexity = 0; // = aCommandIsCompressFileList.Count() * kOneItemComplexity;

  // TTCopyComplexityConverter aCompressConverter;
  // TTDivideComplexityConverter aCopyistConverter(kCopyRangeComplexityDivider);
  
  UINT32 aCompressIndex = 0, aCopyIndexIndex = 0;
  int i;
  for(i = 0; i < aCompressStatuses.Size(); i++)
  {
    if (aCompressStatuses[i])
    {
      const CUpdateItemInfo &anUpdateItem = anUpdateItems[aCompressIndex++];
      aComplexity += anUpdateItem.Size;
      if (anUpdateItem.Commented)
        aComplexity += anUpdateItem.CommentRange.Size;
    }
    else
    {
      const CItemInfoEx &anInputItem = anInputItems[aCopyIndexes[aCopyIndexIndex++]];
      aComplexity += anInputItem.GetLocalFullSize();
      aComplexity += anInputItem.GetCentralExtraPlusCommentSize();
    }
    aComplexity += kOneItemComplexity * 2;
  }
  aCompressIndex = aCopyIndexIndex = 0;
  if (aCommentRangeAssigned)
    aComplexity += aCommentRange.Size;

  aComplexity++; // end of central
  
  anUpdateCallBack->SetTotal(aComplexity);
  auto_ptr<CAddCommon> aCompressor;
  
  aComplexity = 0;
  
  CItemInfoExVector anItems;

  for(i = 0; i < aCompressStatuses.Size(); i++)
  {
    CItemInfoEx anItem;
    // TTConsoleCloseException::TestBreakSignal();
    RETURN_IF_NOT_S_OK(anUpdateCallBack->SetCompleted(&aComplexity));
    if (aCompressStatuses[i])
    {
      if(aCompressor.get() == NULL)
      {
        aCompressor = auto_ptr<CAddCommon>(new CAddCommon(*anOptions));
      }
      const CUpdateItemInfo &anUpdateItemInfo = anUpdateItems[aCompressIndex++];
      RETURN_IF_NOT_S_OK(UpdateOneFile(anInStream, anArchive, *anOptions, 
          *aCompressor, anUpdateItemInfo, aComplexity, anUpdateCallBack, anItem));
    }
    else
    {
      anItem = anInputItems[aCopyIndexes[aCopyIndexIndex++]];
      CUpdateRange aRange(anItem.LocalHeaderPosition, anItem.GetLocalFullSize());
      
      // set new header position
      anItem.LocalHeaderPosition = anArchive.GetCurrentPosition(); 
      
      RETURN_IF_NOT_S_OK(WriteRange(anInStream, anArchive, aRange, 
          anUpdateCallBack, aComplexity));
      anArchive.MoveBasePosition(aRange.Size);
    }
    anItems.Add(anItem);
    aComplexity += kOneItemComplexity;
  }
  DWORD aCentralDirStartPosition = anArchive.GetCurrentPosition();
  aCompressIndex = aCopyIndexIndex = 0;
  for(i = 0; i < anItems.Size(); i++)
  {
    anArchive.WriteCentralHeader(anItems[i]);
    if (aCompressStatuses[i])
    {
      const CUpdateItemInfo &anUpdateItem = anUpdateItems[aCompressIndex++];
      if (anUpdateItem.Commented)
      {
        const CUpdateRange aRange = anUpdateItem.CommentRange;
        RETURN_IF_NOT_S_OK(WriteRange(anInStream, anArchive, aRange, 
            anUpdateCallBack, aComplexity));
        anArchive.MoveBasePosition(aRange.Size);
      }
    }
    else
    {
      const CItemInfoEx anItem = anItems[i];
      CUpdateRange aRange(anItem.CentralExtraPosition, 
        anItem.GetCentralExtraPlusCommentSize());
      RETURN_IF_NOT_S_OK(WriteRange(anInStream, anArchive, aRange, 
          anUpdateCallBack, aComplexity));
      anArchive.MoveBasePosition(aRange.Size);
    }
    aComplexity += kOneItemComplexity;
  }
  COutArchiveInfo anArchiveInfo;
  anArchiveInfo.NumEntriesInCentaralDirectory = anItems.Size();
  anArchiveInfo.CentralDirectorySize = anArchive.GetCurrentPosition() - 
      aCentralDirStartPosition;
  anArchiveInfo.CentralDirectoryStartOffset = aCentralDirStartPosition;
  anArchiveInfo.CommentSize = aCommentRangeAssigned ? WORD(aCommentRange.Size) : 0;
  anArchive.WriteEndOfCentralDir(anArchiveInfo);
  if (aCommentRangeAssigned)
     RETURN_IF_NOT_S_OK(WriteRange(anInStream, anArchive, aCommentRange,
        anUpdateCallBack, aComplexity));
  aComplexity++; // end of central
  return S_OK;
}

}}
