// BZip2Update.cpp

#include "StdAfx.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/CreateCoder.h"
#include "Windows/PropVariant.h"

#include "BZip2Update.h"

namespace NArchive {
namespace NBZip2 {

static const CMethodId kMethodId_BZip2 = 0x040202;

HRESULT UpdateArchive(
    DECL_EXTERNAL_CODECS_LOC_VARS
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    int indexInClient,
    UInt32 dictionary,
    UInt32 numPasses,
    #ifdef COMPRESS_MT
    UInt32 numThreads,
    #endif
    IArchiveUpdateCallback *updateCallback)
{
  RINOK(updateCallback->SetTotal(unpackSize));
  UInt64 complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));

  CMyComPtr<ISequentialInStream> fileInStream;

  RINOK(updateCallback->GetStream(indexInClient, &fileInStream));

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);
  
  CMyComPtr<ICompressCoder> encoder;
  RINOK(CreateCoder(
      EXTERNAL_CODECS_LOC_VARS
      kMethodId_BZip2, encoder, true));
  if (!encoder)
    return E_NOTIMPL;
  CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
  encoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProperties);
  if (setCoderProperties)
  {
    NWindows::NCOM::CPropVariant properties[] = 
    {
      dictionary, 
      numPasses
      #ifdef COMPRESS_MT
      , numThreads
      #endif
    };
    PROPID propIDs[] = 
    {
      NCoderPropID::kDictionarySize,
      NCoderPropID::kNumPasses
      #ifdef COMPRESS_MT
      , NCoderPropID::kNumThreads
      #endif
    };
    RINOK(setCoderProperties->SetCoderProperties(propIDs, properties, sizeof(propIDs) / sizeof(propIDs[0])));
  }
  
  RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, localProgress));
  
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

}}
