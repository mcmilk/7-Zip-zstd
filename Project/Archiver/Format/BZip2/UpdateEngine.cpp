// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Windows/Defs.h"

#include "UpdateEngine.h"

#include "Interface/ProgressUtils.h"

#ifdef COMPRESS_BZIP2
#include "../../../Compress/BWT/BZip2/Encoder.h"
#else
// {23170F69-40C1-278B-0402-020000000100}
DEFINE_GUID(CLSID_CCompressBZip2Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif

namespace NArchive {
namespace NBZip2 {

static const kOneItemComplexity = 30;

// -----------------------------------------------------
// Main Function UpdateArchiveStd


HRESULT UpdateArchive(UINT64 unpackSize,
    IOutStream *outStream,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback)
{
  RINOK(updateCallback->SetTotal(unpackSize));
  
  UINT64 complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));

  CComPtr<IInStream> fileInStream;

  RINOK(updateCallback->GetStream(indexInClient, &fileInStream));

  CComObjectNoLock<CLocalProgress> *localProgressSpec = 
    new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);
  
  CComPtr<ICompressCoder> encoder;
  #ifdef COMPRESS_BZIP2
  encoder = new CComObjectNoLock<NCompress::NBZip2::NEncoder::CCoder>;
  #else
  RINOK(encoder.CoCreateInstance(CLSID_CCompressBZip2Encoder));
  #endif
  
  RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, localProgress));
  
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

}}
