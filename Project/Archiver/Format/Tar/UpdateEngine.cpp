// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "UpdateEngine.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Compression/CopyCoder.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "Archive/Tar/OutEngine.h"

#include "Windows/Defs.h"

static const kOneItemComplexity = 30;

namespace NArchive {
namespace NTar {

HRESULT CopyBlock(ISequentialInStream *anInStream, 
    ISequentialOutStream *anOutStream, ICompressProgressInfo *aProgress)
{
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> aCopyCoder = aCopyCoderSpec;

  return aCopyCoder->Code(anInStream, anOutStream, NULL, NULL, aProgress);
}

HRESULT UpdateArchive(IInStream *anInStream, ISequentialOutStream *anOutStream,
    const NArchive::NTar::CItemInfoExVector &anInputItems,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IUpdateCallBack *anUpdateCallBack)
{
  COutArchive anOutArchive;

  anOutArchive.Create(anOutStream);

  UINT64 aComplexity = 0;
  UINT32 aCompressIndex = 0, aCopyIndexIndex = 0;

  int i;
  for(i = 0; i < aCompressStatuses.Size(); i++)
  {
    if (aCompressStatuses[i])
      aComplexity += anUpdateItems[aCompressIndex++].Size;
    else
      aComplexity += anInputItems[aCopyIndexes[aCopyIndexIndex++]].GetFullSize();
    aComplexity += kOneItemComplexity;
  }
  aCompressIndex = aCopyIndexIndex = 0;

  RETURN_IF_NOT_S_OK(anUpdateCallBack->SetTotal(aComplexity));

  aComplexity = 0;

  for(i = 0; i < aCompressStatuses.Size(); i++)
  {
    CItemInfoEx anItem;
    RETURN_IF_NOT_S_OK(anUpdateCallBack->SetCompleted(&aComplexity));

    CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = 
      new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
    aLocalProgressSpec->Init(anUpdateCallBack, true);
  
    CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;

    aLocalCompressProgressSpec->Init(aLocalProgress, &aComplexity, NULL);

    if (aCompressStatuses[i])
    {
      const CUpdateItemInfo &anUpdateItemInfo = anUpdateItems[aCompressIndex++];

      CComPtr<IInStream> aFileInStream;
      RETURN_IF_NOT_S_OK(anUpdateCallBack->CompressOperation(
          anUpdateItemInfo.IndexInClient, &aFileInStream));

      CItemInfo anItemInfo;

      anItemInfo.Mode = 0777;
      anItemInfo.Name = (anUpdateItemInfo.Name);
      if (anUpdateItemInfo.IsDirectory)
      {
         anItemInfo.LinkFlag = NFileHeader::NLinkFlag::kDirectory;
         anItemInfo.Size = 0;
      }
      else
      {
         anItemInfo.LinkFlag = NFileHeader::NLinkFlag::kNormal;
         anItemInfo.Size = anUpdateItemInfo.Size;
      }
      anItemInfo.ModificationTime = anUpdateItemInfo.Time;
      anItemInfo.DeviceMajorDefined = false;
      anItemInfo.DeviceMinorDefined = false;
      anItemInfo.UID = 0;
      anItemInfo.GID = 0;
      memmove(anItemInfo.Magic, NFileHeader::NMagic::kEmpty, 8);

      RETURN_IF_NOT_S_OK(anOutArchive.WriteHeader(anItemInfo));

      if (!anUpdateItemInfo.IsDirectory)
      {
        RETURN_IF_NOT_S_OK(CopyBlock(aFileInStream, anOutStream, aCompressProgress));
        RETURN_IF_NOT_S_OK(anOutArchive.FillDataResidual(anItemInfo.Size));
      }
      aComplexity += anUpdateItemInfo.Size;
      RETURN_IF_NOT_S_OK(anUpdateCallBack->OperationResult(
          NArchiveHandler::NUpdate::NOperationResult::kOK));
    }
    else
    {
      CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
          CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<CLimitedSequentialInStream> anInStreamLimited(aStreamSpec);
      const CItemInfoEx &anExistItemInfo = anInputItems[aCopyIndexes[aCopyIndexIndex++]];
      RETURN_IF_NOT_S_OK(anInStream->Seek(anExistItemInfo.HeaderPosition, 
          STREAM_SEEK_SET, NULL));
      aStreamSpec->Init(anInStream, anExistItemInfo.GetFullSize());
      RETURN_IF_NOT_S_OK(CopyBlock(anInStreamLimited, anOutStream, aCompressProgress));
      RETURN_IF_NOT_S_OK(anOutArchive.FillDataResidual(anExistItemInfo.Size));
      aComplexity += anExistItemInfo.GetFullSize();
    }
    aComplexity += kOneItemComplexity;
  }
  return anOutArchive.WriteFinishHeader();
}

}}