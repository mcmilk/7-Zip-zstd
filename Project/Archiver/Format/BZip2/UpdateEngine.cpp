// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "Windows/Defs.h"

#include "UpdateEngine.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"

#include "Handler.h"

#include "Interface/LimitedStreams.h"

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


HRESULT UpdateArchive(UINT64 anUnpackSize,
    IOutStream *anOutStream,
    int anIndexInClient,
    IUpdateCallBack *anUpdateCallBack)
{
  RETURN_IF_NOT_S_OK(anUpdateCallBack->SetTotal(anUnpackSize));
  
  UINT64 aComplexity = 0;
  RETURN_IF_NOT_S_OK(anUpdateCallBack->SetCompleted(&aComplexity));

  CComPtr<IInStream> aFileInStream;

  RETURN_IF_NOT_S_OK(anUpdateCallBack->CompressOperation(
      anIndexInClient, &aFileInStream));

  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = 
    new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(anUpdateCallBack, true);
  
  CComPtr<ICompressCoder> anEncoder;
  #ifdef COMPRESS_BZIP2
  anEncoder = new CComObjectNoLock<NCompress::NBZip2::NEncoder::CCoder>;
  #else
  RETURN_IF_NOT_S_OK(anEncoder.CoCreateInstance(CLSID_CCompressBZip2Encoder));
  #endif
  
  /*
  NWindows::NCOM::CPropVariant aProperties[2] = 
  {
  m_Options.MaximizeRatio ? UINT32(kNumPassesMX) : UINT32(kNumPassesNormal),
  m_Options.MaximizeRatio ? UINT32(kMatchFastLenMX) : UINT32(kMatchFastLenNormal)
  };
  CComPtr<ICompressSetEncoderProperties> aSetEncoderProperties;
  RETURN_IF_NOT_S_OK(m_DeflateEncoder.QueryInterface(&aSetEncoderProperties));
  aSetEncoderProperties->SetEncoderProperties(aProperties, 2);
  */
  RETURN_IF_NOT_S_OK(anEncoder->Code(aFileInStream, anOutStream, NULL, NULL, aLocalProgress));
  
  RETURN_IF_NOT_S_OK(anUpdateCallBack->OperationResult(
      NArchiveHandler::NUpdate::NOperationResult::kOK));

  return S_OK;
}

}}
