// BZip2Update.cpp

#include "StdAfx.h"

#include "../../Common/ProgressUtils.h"
#include "Windows/PropVariant.h"

#include "BZip2Update.h"

#ifdef COMPRESS_BZIP2
#include "../../Compress/BZip2/BZip2Encoder.h"
#else
// {23170F69-40C1-278B-0402-020000000100}
DEFINE_GUID(CLSID_CCompressBZip2Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00);
#include "../Common/CoderLoader.h"
extern CSysString GetBZip2CodecPath();
#endif

namespace NArchive {
namespace NBZip2 {

HRESULT UpdateArchive(UInt64 unpackSize,
    ISequentialOutStream *outStream,
    int indexInClient,
    UInt32 numPasses,
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
  
  #ifndef COMPRESS_BZIP2
  CCoderLibrary lib;
  #endif
  CMyComPtr<ICompressCoder> encoder;
  #ifdef COMPRESS_BZIP2
  encoder = new NCompress::NBZip2::CEncoder;
  #else
  RINOK(lib.LoadAndCreateCoder(GetBZip2CodecPath(),
      CLSID_CCompressBZip2Encoder, &encoder));
  #endif

  CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
  encoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProperties);
  if (setCoderProperties)
  {
    /*
    NWindows::NCOM::CPropVariant properties[2] = 
    {
      dictionary, numPasses
    };
    PROPID propIDs[2] = 
    {
      NCoderPropID::kDictionarySize,
      NCoderPropID::kNumPasses,
    };
    RINOK(setCoderProperties->SetCoderProperties(propIDs, properties, 2));
    */
    NWindows::NCOM::CPropVariant property = numPasses;
    PROPID propID = NCoderPropID::kNumPasses;
    RINOK(setCoderProperties->SetCoderProperties(&propID, &property, 1));
  }
  
  RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, localProgress));
  
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

}}
