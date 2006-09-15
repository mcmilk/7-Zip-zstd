// TarUpdate.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Windows/Defs.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"
#include "../../Compress/Copy/CopyCoder.h"

#include "TarOut.h"
#include "TarUpdate.h"

static const UInt64 kOneItemComplexity = 512;

namespace NArchive {
namespace NTar {

static HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress,
    UInt64 *totalSize = NULL)
{
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  HRESULT result = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
  if (totalSize != NULL)
    *totalSize = copyCoderSpec->TotalSize;
  return result;
}

HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<NArchive::NTar::CItemEx> &inputItems,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    IArchiveUpdateCallback *updateCallback)
{
  COutArchive outArchive;
  outArchive.Create(outStream);

  UInt64 complexity = 0;

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
    RINOK(updateCallback->SetCompleted(&complexity));

    CLocalProgress *localProgressSpec = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
    localProgressSpec->Init(updateCallback, true);
  
    CLocalCompressProgressInfo *localCompressProgressSpec = new CLocalCompressProgressInfo;
    CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

    localCompressProgressSpec->Init(localProgress, &complexity, NULL);

    const CUpdateItemInfo &updateItem = updateItems[i];
    CItem item;
    if (updateItem.NewProperties)
    {
      item.Mode = 0777;
      item.Name = (updateItem.Name);
      if (updateItem.IsDirectory)
      {
         item.LinkFlag = NFileHeader::NLinkFlag::kDirectory;
         item.Size = 0;
      }
      else
      {
         item.LinkFlag = NFileHeader::NLinkFlag::kNormal;
         item.Size = updateItem.Size;
      }
      item.ModificationTime = updateItem.Time;
      item.DeviceMajorDefined = false;
      item.DeviceMinorDefined = false;
      item.UID = 0;
      item.GID = 0;
      memmove(item.Magic, NFileHeader::NMagic::kEmpty, 8);
    }
    else
    {
      const CItemEx &existItemInfo = inputItems[updateItem.IndexInArchive];
      item = existItemInfo;
    }
    if (updateItem.NewData)
    {
      item.Size = updateItem.Size;
      if (item.Size == UInt64(Int64(-1)))
        return E_INVALIDARG;
    }
    else
    {
      const CItemEx &existItemInfo = inputItems[updateItem.IndexInArchive];
      item.Size = existItemInfo.Size;
    }
  
    if (updateItem.NewData)
    {
      CMyComPtr<ISequentialInStream> fileInStream;
      HRESULT res = updateCallback->GetStream(updateItem.IndexInClient, &fileInStream);
      if (res != S_FALSE)
      {
        RINOK(res);
        RINOK(outArchive.WriteHeader(item));
        if (!updateItem.IsDirectory)
        {
          UInt64 totalSize;
          RINOK(CopyBlock(fileInStream, outStream, compressProgress, &totalSize));
          if (totalSize != item.Size)
            return E_FAIL;
          RINOK(outArchive.FillDataResidual(item.Size));
        }
      }
      complexity += updateItem.Size;
      RINOK(updateCallback->SetOperationResult(
          NArchive::NUpdate::NOperationResult::kOK));
    }
    else
    {
      CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
      CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
      const CItemEx &existItemInfo = inputItems[updateItem.IndexInArchive];
      if (updateItem.NewProperties)
      {
        RINOK(outArchive.WriteHeader(item));
        RINOK(inStream->Seek(existItemInfo.GetDataPosition(), 
            STREAM_SEEK_SET, NULL));
        streamSpec->SetStream(inStream);
        streamSpec->Init(existItemInfo.Size);
      }
      else
      {
        RINOK(inStream->Seek(existItemInfo.HeaderPosition, 
            STREAM_SEEK_SET, NULL));
        streamSpec->SetStream(inStream);
        streamSpec->Init(existItemInfo.GetFullSize());
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

