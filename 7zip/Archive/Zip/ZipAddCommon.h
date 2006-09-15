// Zip/AddCommon.h

#ifndef __ZIP_ADDCOMMON_H
#define __ZIP_ADDCOMMON_H

#include "../../ICoder.h"
#include "../../IProgress.h"
#include "../../Compress/Copy/CopyCoder.h"
#ifndef COMPRESS_DEFLATE
#include "../Common/CoderLoader.h"
#endif
#include "../Common/FilterCoder.h"
#include "ZipCompressionMode.h"
#include "../../Crypto/Zip/ZipCipher.h"
#include "../../Crypto/WzAES/WzAES.h"

namespace NArchive {
namespace NZip {

struct CCompressingResult
{
  UInt64 UnpackSize;
  UInt64 PackSize;
  UInt32 CRC;
  UInt16 Method;
  Byte ExtractVersion;
};

class CAddCommon
{
  CCompressionMethodMode _options;
  NCompress::CCopyCoder *_copyCoderSpec;
  CMyComPtr<ICompressCoder> _copyCoder;

  #ifndef COMPRESS_DEFLATE
  CCoderLibrary _compressLib;
  #endif
  CMyComPtr<ICompressCoder> _compressEncoder;

  CFilterCoder *_cryptoStreamSpec;
  CMyComPtr<ISequentialOutStream> _cryptoStream;

  NCrypto::NZip::CEncoder *_filterSpec;
  NCrypto::NWzAES::CEncoder *_filterAesSpec;

  CMyComPtr<ICompressFilter> _zipCryptoFilter;
  CMyComPtr<ICompressFilter> _aesFilter;


public:
  CAddCommon(const CCompressionMethodMode &options);
  HRESULT Compress(ISequentialInStream *inStream, IOutStream *outStream, 
      ICompressProgressInfo *progress, CCompressingResult &operationResult);
};

}}

#endif
