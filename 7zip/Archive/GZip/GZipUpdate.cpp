// GZipUpdate.cpp

#include "StdAfx.h"

#include "GZipUpdate.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "../../ICoder.h"
#include "../../Common/ProgressUtils.h"
#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/InStreamWithCRC.h"

#ifdef COMPRESS_DEFLATE
#include "../../Compress/Deflate/DeflateEncoder.h"
#else
// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);
extern CSysString GetBaseFolderPrefix();
#include "../Common/CoderLoader.h"
extern CSysString GetDeflateCodecPath();
#endif

namespace NArchive {
namespace NGZip {

static const Byte kHostOS = NFileHeader::NHostOS::kFAT;

HRESULT UpdateArchive(IInStream *inStream, 
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    const CItem &newItem,
    const CCompressionMethodMode &compressionMethod,
    int indexInClient,
    IArchiveUpdateCallback *updateCallback)
{
  UInt64 complexity = 0;

  complexity += unpackSize;

  RINOK(updateCallback->SetTotal(complexity));

  #ifndef COMPRESS_DEFLATE
  CCoderLibrary lib;
  #endif
  CMyComPtr<ICompressCoder> deflateEncoder;
  
  complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));

  CMyComPtr<ISequentialInStream> fileInStream;

  RINOK(updateCallback->GetStream(indexInClient, &fileInStream));

  CSequentialInStreamWithCRC *inStreamSpec = new CSequentialInStreamWithCRC;
  CMyComPtr<ISequentialInStream> crcStream(inStreamSpec);
  inStreamSpec->Init(fileInStream);

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);
  
  CLocalCompressProgressInfo *localCompressProgressSpec = 
      new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
  
  COutArchive outArchive;
  outArchive.Create(outStream);

  CItem item = newItem;
  item.CompressionMethod = NFileHeader::NCompressionMethod::kDeflate;
  item.ExtraFlags = 0;
  item.HostOS = kHostOS;

  RINOK(outArchive.WriteHeader(item));

  localCompressProgressSpec->Init(localProgress, &complexity, NULL);

  {
    #ifdef COMPRESS_DEFLATE
    deflateEncoder = new NCompress::NDeflate::NEncoder::CCOMCoder;
    #else
    RINOK(lib.LoadAndCreateCoder(GetDeflateCodecPath(),
        CLSID_CCompressDeflateEncoder, &deflateEncoder));
    #endif

    NWindows::NCOM::CPropVariant properties[2] = 
      { compressionMethod.NumPasses, compressionMethod.NumFastBytes };
    PROPID propIDs[2] = 
      { NCoderPropID::kNumPasses, NCoderPropID::kNumFastBytes };
    CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
    RINOK(deflateEncoder.QueryInterface(
        IID_ICompressSetCoderProperties, &setCoderProperties));
    RINOK(setCoderProperties->SetCoderProperties(propIDs, properties, 2));
  }
  RINOK(deflateEncoder->Code(crcStream, outStream, NULL, NULL, compressProgress));

  item.FileCRC = inStreamSpec->GetCRC();
  item.UnPackSize32 = (UInt32)inStreamSpec->GetSize();
  RINOK(outArchive.WritePostHeader(item));
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

}}
