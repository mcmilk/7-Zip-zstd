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

static HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress,
    UINT64 *totalSize = NULL)
{
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  HRESULT result = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
  if (totalSize != NULL)
    *totalSize = copyCoderSpec->TotalSize;
  return result;
}

HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<NArchive::NTar::CItemInfoEx> &inputItems,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    IArchiveUpdateCallback *updateCallback)
{
  COutArchive outArchive;
  outArchive.Create(outStream);

  UINT64 complexity = 0;

  int i;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItemInfo &updateItem = updateItems[i];
    if (updateItem.NewData)
      complexity += updateItem.Size;
    else
      complexity += inputItems[updateItem.IndexInArchive].GetFullSize();
    complexity += kOneItemComplexity;
  }

  RINOK(updateCallback->SetTotal(complexity));

  complexity = 0;

  for(i = 0; i < updateItems.Size(); i++)
  {
    CItemInfoEx item;
    RINOK(updateCallback->SetCompleted(&complexity));

    CComObjectNoLock<CLocalProgress> *localProgressSpec = 
      new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
    localProgressSpec->Init(updateCallback, true);
  
    CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

    localCompressProgressSpec->Init(localProgress, &complexity, NULL);

    const CUpdateItemInfo &updateItem = updateItems[i];
    CItemInfo itemInfo;
    if (updateItem.NewProperties)
    {
      itemInfo.Mode = 0777;
      itemInfo.Name = (updateItem.Name);
      if (updateItem.IsDirectory)
      {
         itemInfo.LinkFlag = NFileHeader::NLinkFlag::kDirectory;
         itemInfo.Size = 0;
      }
      else
      {
         itemInfo.LinkFlag = NFileHeader::NLinkFlag::kNormal;
         itemInfo.Size = updateItem.Size;
      }
      itemInfo.ModificationTime = updateItem.Time;
      itemInfo.DeviceMajorDefined = false;
      itemInfo.DeviceMinorDefined = false;
      itemInfo.UID = 0;
      itemInfo.GID = 0;
      memmove(itemInfo.Magic, NFileHeader::NMagic::kEmpty, 8);
    }
    else
    {
      const CItemInfoEx &existItemInfo = inputItems[updateItem.IndexInArchive];
      itemInfo = existItemInfo;
    }
    if (updateItem.NewData)
    {
      itemInfo.Size = updateItem.Size;
    }
    else
    {
      const CItemInfoEx &existItemInfo = inputItems[updateItem.IndexInArchive];
      itemInfo.Size = existItemInfo.Size;
    }
    if (updateItem.NewData || updateItem.NewProperties)
    {
      RINOK(outArchive.WriteHeader(itemInfo));
    }

    if (updateItem.NewData)
    {
      CComPtr<IInStream> fileInStream;
      RINOK(updateCallback->GetStream(updateItem.IndexInClient, &fileInStream));
      if (!updateItem.IsDirectory)
      {
        UINT64 totalSize;
        RINOK(CopyBlock(fileInStream, outStream, compressProgress, &totalSize));
        if (totalSize != itemInfo.Size)
          return E_FAIL;
        RINOK(outArchive.FillDataResidual(itemInfo.Size));
      }
      complexity += updateItem.Size;
      RINOK(updateCallback->SetOperationResult(
          NArchive::NUpdate::NOperationResult::kOK));
    }
    else
    {
      CComObjectNoLock<CLimitedSequentialInStream> *streamSpec = new 
          CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
      const CItemInfoEx &existItemInfo = inputItems[updateItem.IndexInArchive];
      if (updateItem.NewProperties)
      {
        RINOK(inStream->Seek(existItemInfo.GetDataPosition(), 
            STREAM_SEEK_SET, NULL));
        streamSpec->Init(inStream, existItemInfo.Size);
      }
      else
      {
        RINOK(inStream->Seek(existItemInfo.HeaderPosition, 
            STREAM_SEEK_SET, NULL));
        streamSpec->Init(inStream, existItemInfo.GetFullSize());
      }
      RINOK(CopyBlock(inStreamLimited, outStream, compressProgress));
      RINOK(outArchive.FillDataResidual(existItemInfo.Size));
      complexity += existItemInfo.GetFullSize();
    }
    complexity += kOneItemComplexity;
  }
  return outArchive.WriteFinishHeader();
}

}}