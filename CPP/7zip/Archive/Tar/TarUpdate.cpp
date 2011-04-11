// TarUpdate.cpp

#include "StdAfx.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"

#include "../../Compress/CopyCoder.h"

#include "TarOut.h"
#include "TarUpdate.h"

namespace NArchive {
namespace NTar {

HRESULT UpdateArchive(IInStream *inStream, ISequentialOutStream *outStream,
    const CObjectVector<NArchive::NTar::CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    IArchiveUpdateCallback *updateCallback)
{
  COutArchive outArchive;
  outArchive.Create(outStream);

  UInt64 complexity = 0;

  int i;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (ui.NewData)
      complexity += ui.Size;
    else
      complexity += inputItems[ui.IndexInArchive].GetFullSize();
  }

  RINOK(updateCallback->SetTotal(complexity));

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
  streamSpec->SetStream(inStream);

  complexity = 0;

  for (i = 0; i < updateItems.Size(); i++)
  {
    lps->InSize = lps->OutSize = complexity;
    RINOK(lps->SetCur());

    const CUpdateItem &ui = updateItems[i];
    CItem item;
    if (ui.NewProps)
    {
      item.Mode = ui.Mode;
      item.Name = ui.Name;
      item.User = ui.User;
      item.Group = ui.Group;
      if (ui.IsDir)
      {
        item.LinkFlag = NFileHeader::NLinkFlag::kDirectory;
        item.PackSize = 0;
      }
      else
      {
        item.LinkFlag = NFileHeader::NLinkFlag::kNormal;
        item.PackSize = ui.Size;
      }
      item.MTime = ui.MTime;
      item.DeviceMajorDefined = false;
      item.DeviceMinorDefined = false;
      item.UID = 0;
      item.GID = 0;
      memmove(item.Magic, NFileHeader::NMagic::kEmpty, 8);
    }
    else
      item = inputItems[ui.IndexInArchive];

    if (ui.NewData)
    {
      item.PackSize = ui.Size;
      if (ui.Size == (UInt64)(Int64)-1)
        return E_INVALIDARG;
    }
    else
      item.PackSize = inputItems[ui.IndexInArchive].PackSize;
  
    if (ui.NewData)
    {
      CMyComPtr<ISequentialInStream> fileInStream;
      HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
      if (res != S_FALSE)
      {
        RINOK(res);
        RINOK(outArchive.WriteHeader(item));
        if (!ui.IsDir)
        {
          RINOK(copyCoder->Code(fileInStream, outStream, NULL, NULL, progress));
          if (copyCoderSpec->TotalSize != item.PackSize)
            return E_FAIL;
          RINOK(outArchive.FillDataResidual(item.PackSize));
        }
      }
      complexity += ui.Size;
      RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
    }
    else
    {
      const CItemEx &existItem = inputItems[ui.IndexInArchive];
      UInt64 size;
      if (ui.NewProps)
      {
        RINOK(outArchive.WriteHeader(item));
        RINOK(inStream->Seek(existItem.GetDataPosition(), STREAM_SEEK_SET, NULL));
        size = existItem.PackSize;
      }
      else
      {
        RINOK(inStream->Seek(existItem.HeaderPos, STREAM_SEEK_SET, NULL));
        size = existItem.GetFullSize();
      }
      streamSpec->Init(size);

      RINOK(copyCoder->Code(inStreamLimited, outStream, NULL, NULL, progress));
      if (copyCoderSpec->TotalSize != size)
        return E_FAIL;
      RINOK(outArchive.FillDataResidual(existItem.PackSize));
      complexity += size;
    }
  }
  lps->InSize = lps->OutSize = complexity;
  RINOK(lps->SetCur());
  return outArchive.WriteFinishHeader();
}

}}
