// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "UpdateEngine.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"

#include "Interface/ProgressUtils.h"

#include "AddCommon.h"
#include "Handler.h"

#include "../Common/InStreamWithCRC.h"

#include "Interface/LimitedStreams.h"

namespace NArchive {
namespace NGZip {

static const kOneItemComplexity = 30;

static const BYTE kHostOS = NFileHeader::NHostOS::kFAT;

HRESULT UpdateArchive(IInStream *anInStream, 
    const CItemInfoEx *anItemInfoExist,
    UINT64 anUnpackSize,
    IOutStream *anOutStream,
    const CItemInfo &aNewItemInfo,
    const CCompressionMethodMode &aCompressionMethod,
    int anIndexInClient,
    IUpdateCallBack *anUpdateCallBack)
{
  UINT64 aComplexity = 0;

  aComplexity += anUnpackSize;

  RETURN_IF_NOT_S_OK(anUpdateCallBack->SetTotal(aComplexity));

  CAddCommon aCompressor(aCompressionMethod);
  
  aComplexity = 0;
  RETURN_IF_NOT_S_OK(anUpdateCallBack->SetCompleted(&aComplexity));


  CComPtr<IInStream> aFileInStream;

  RETURN_IF_NOT_S_OK(anUpdateCallBack->CompressOperation(
      anIndexInClient, &aFileInStream));

  CComObjectNoLock<CInStreamWithCRC> *anInStreamSpec = 
  new CComObjectNoLock<CInStreamWithCRC>;
  CComPtr<ISequentialInStream> aCRCStream(anInStreamSpec);
  anInStreamSpec->Init(aFileInStream);

  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = 
    new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(anUpdateCallBack, true);
  
  CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
    new  CComObjectNoLock<CLocalCompressProgressInfo>;
  CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
  
  COutArchive anOutArchive;
  anOutArchive.Create(anOutStream);

  CItemInfo anItemInfo = aNewItemInfo;
  anItemInfo.CompressionMethod = NFileHeader::NCompressionMethod::kDefalate;
  anItemInfo.ExtraFlags = 0;
  anItemInfo.HostOS = kHostOS;

  RETURN_IF_NOT_S_OK(anOutArchive.WriteHeader(anItemInfo));

  aLocalCompressProgressSpec->Init(aLocalProgress, &aComplexity, NULL);

  RETURN_IF_NOT_S_OK(aCompressor.Compress(aCRCStream, anOutStream, aCompressProgress));

  RETURN_IF_NOT_S_OK(anOutArchive.WritePostInfo(anInStreamSpec->GetCRC(), anItemInfo.UnPackSize32));
  RETURN_IF_NOT_S_OK(anUpdateCallBack->OperationResult(
      NArchiveHandler::NUpdate::NOperationResult::kOK));
  return S_OK;
}

}}
