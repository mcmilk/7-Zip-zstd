// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "UpdateEngine.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "AddCommon.h"
#include "Handler.h"

#include "../Common/InStreamWithCRC.h"

namespace NArchive {
namespace NGZip {

static const BYTE kHostOS = NFileHeader::NHostOS::kFAT;

HRESULT UpdateArchive(IInStream *inStream, 
    // const CItemInfoEx *existingItemInfo,
    UINT64 unpackSize,
    IOutStream *outStream,
    const CItemInfo &newItemInfo,
    const CCompressionMethodMode &compressionMethod,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback)
{
  UINT64 complexity = 0;

  complexity += unpackSize;

  RINOK(updateCallback->SetTotal(complexity));

  CAddCommon compressor(compressionMethod);
  
  complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));


  CComPtr<IInStream> fileInStream;

  RINOK(updateCallback->GetStream(indexInClient, &fileInStream));

  CComObjectNoLock<CInStreamWithCRC> *inStreamSpec = 
      new CComObjectNoLock<CInStreamWithCRC>;
  CComPtr<ISequentialInStream> crcStream(inStreamSpec);
  inStreamSpec->Init(fileInStream);

  CComObjectNoLock<CLocalProgress> *localProgressSpec = 
    new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);
  
  CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
    new  CComObjectNoLock<CLocalCompressProgressInfo>;
  CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
  
  COutArchive outArchive;
  outArchive.Create(outStream);

  CItemInfo itemInfo = newItemInfo;
  itemInfo.CompressionMethod = NFileHeader::NCompressionMethod::kDefalate;
  itemInfo.ExtraFlags = 0;
  itemInfo.HostOS = kHostOS;

  RINOK(outArchive.WriteHeader(itemInfo));

  localCompressProgressSpec->Init(localProgress, &complexity, NULL);

  RINOK(compressor.Compress(crcStream, outStream, compressProgress));

  RINOK(outArchive.WritePostInfo(inStreamSpec->GetCRC(), 
      (UINT32)inStreamSpec->GetSize()));
  return updateCallback->SetOperationResult(
      NArchive::NUpdate::NOperationResult::kOK);
}

}}
