// Zip/AddCommon.h

#pragma once

#ifndef __ZIP_ADDCOMMON_H
#define __ZIP_ADDCOMMON_H

#include "Compression/CopyCoder.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Interface/IProgress.h"
#include "../../../Compress/Interface/CompressInterface.h"
#include "../Common/CoderMixer.h"

namespace NArchive {
namespace NZip {

struct CCompressingResult
{
  BYTE Method;
  UINT32 PackSize;
  BYTE ExtractVersion;
};

class CAddCommon
{
  CCompressionMethodMode _options;
  CComObjectNoLock<NCompression::CCopyCoder> *_copyCoderSpec;
  CComPtr<ICompressCoder> _copyCoder;
  CComPtr<ICompressCoder> _compressEncoder;

  CComPtr<ICompressCoder> _cryptoEncoder;
  CComObjectNoLock<CCoderMixer> *_mixerCoderSpec;
  CComPtr<ICompressCoder> _mixerCoder;
  BYTE _mixerCoderMethod;
  // CComPtr<ICryptoGetTextPassword> getTextPassword;


public:
  CAddCommon(const CCompressionMethodMode &options);
  HRESULT Compress(IInStream *inStream, IOutStream *outStream, 
      UINT64 inSize, ICompressProgressInfo *progress, CCompressingResult &operationResult);
};

}}

#endif
