// GZipUpdate.cpp

#include "StdAfx.h"

#include "GZipUpdate.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "../../ICoder.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/CreateCoder.h"
#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/InStreamWithCRC.h"

namespace NArchive {
namespace NGZip {

static const CMethodId kMethodId_Deflate = 0x040108;

static const Byte kHostOS = NFileHeader::NHostOS::kFAT;

HRESULT UpdateArchive(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream * /* inStream */, 
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    const CItem &newItem,
    const CCompressionMethodMode &compressionMethod,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback)
{
  UInt64 complexity = unpackSize;

  RINOK(updateCallback->SetTotal(complexity));

  CMyComPtr<ICompressCoder> deflateEncoder;
  
  complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));

  CMyComPtr<ISequentialInStream> fileInStream;

  RINOK(updateCallback->GetStream(indexInClient, &fileInStream));

  CSequentialInStreamWithCRC *inStreamSpec = new CSequentialInStreamWithCRC;
  CMyComPtr<ISequentialInStream> crcStream(inStreamSpec);
  inStreamSpec->SetStream(fileInStream);
  inStreamSpec->Init();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);
  
  COutArchive outArchive;
  outArchive.Create(outStream);

  CItem item = newItem;
  item.CompressionMethod = NFileHeader::NCompressionMethod::kDeflate;
  item.ExtraFlags = 0;
  item.HostOS = kHostOS;

  RINOK(outArchive.WriteHeader(item));

  {
    RINOK(CreateCoder(
        EXTERNAL_CODECS_LOC_VARS
        kMethodId_Deflate, deflateEncoder, true));
    if (!deflateEncoder)
      return E_NOTIMPL;

    NWindows::NCOM::CPropVariant properties[] = 
    { 
      compressionMethod.Algo, 
      compressionMethod.NumPasses, 
      compressionMethod.NumFastBytes,
      compressionMethod.NumMatchFinderCycles
    };
    PROPID propIDs[] = 
    { 
      NCoderPropID::kAlgorithm,
      NCoderPropID::kNumPasses, 
      NCoderPropID::kNumFastBytes,
      NCoderPropID::kMatchFinderCycles
    };
    int numProps = sizeof(propIDs) / sizeof(propIDs[0]);
    if (!compressionMethod.NumMatchFinderCyclesDefined)
      numProps--;
    CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
    RINOK(deflateEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProperties));
    RINOK(setCoderProperties->SetCoderProperties(propIDs, properties, numProps));
  }
  RINOK(deflateEncoder->Code(crcStream, outStream, NULL, NULL, progress));

  item.FileCRC = inStreamSpec->GetCRC();
  item.UnPackSize32 = (UInt32)inStreamSpec->GetSize();
  RINOK(outArchive.WritePostHeader(item));
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

}}
