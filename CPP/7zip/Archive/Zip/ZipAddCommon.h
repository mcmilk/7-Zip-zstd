// Zip/AddCommon.h

#ifndef __ZIP_ADDCOMMON_H
#define __ZIP_ADDCOMMON_H

#include "../../ICoder.h"
#include "../../IProgress.h"
#include "../../Compress/Copy/CopyCoder.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/FilterCoder.h"
#include "../../Crypto/Zip/ZipCipher.h"
#include "../../Crypto/WzAES/WzAES.h"

#include "ZipCompressionMode.h"

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

  CMyComPtr<ICompressCoder> _compressEncoder;

  CFilterCoder *_cryptoStreamSpec;
  CMyComPtr<ISequentialOutStream> _cryptoStream;

  NCrypto::NZip::CEncoder *_filterSpec;
  NCrypto::NWzAES::CEncoder *_filterAesSpec;

  CMyComPtr<ICompressFilter> _zipCryptoFilter;
  CMyComPtr<ICompressFilter> _aesFilter;


public:
  CAddCommon(const CCompressionMethodMode &options);
  HRESULT Compress(
      DECL_EXTERNAL_CODECS_LOC_VARS
      ISequentialInStream *inStream, IOutStream *outStream, 
      ICompressProgressInfo *progress, CCompressingResult &operationResult);
};

}}

#endif
