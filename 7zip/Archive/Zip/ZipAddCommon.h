// Zip/AddCommon.h

#pragma once

#ifndef __ZIP_ADDCOMMON_H
#define __ZIP_ADDCOMMON_H

#include "../../ICoder.h"
#include "../../IProgress.h"
#include "../../Compress/Copy/CopyCoder.h"
#include "../Common/CoderMixer.h"
#ifndef COMPRESS_DEFLATE
#include "../Common/CoderLoader.h"
#endif
#include "ZipCompressionMode.h"

namespace NArchive {
namespace NZip {

struct CCompressingResult
{
  BYTE Method;
  UINT64 PackSize;
  BYTE ExtractVersion;
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

  #ifndef CRYPTO_ZIP
  CCoderLibrary _cryptoLib;
  #endif
  CMyComPtr<ICompressCoder> _cryptoEncoder;
  CCoderMixer *_mixerCoderSpec;
  CMyComPtr<ICompressCoder> _mixerCoder;
  BYTE _mixerCoderMethod;
  // CMyComPtr<ICryptoGetTextPassword> getTextPassword;


public:
  CAddCommon(const CCompressionMethodMode &options);
  HRESULT Compress(IInStream *inStream, IOutStream *outStream, 
      UINT64 inSize, ICompressProgressInfo *progress, CCompressingResult &operationResult);
};

}}

#endif
