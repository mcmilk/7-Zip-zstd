// 7zEncode.h

#ifndef __7Z_ENCODE_H
#define __7Z_ENCODE_H

// #include "../../Common/StreamObjects.h"

#include "7zCompressionMode.h"

#include "../Common/CoderMixer2.h"
#include "../Common/CoderMixer2MT.h"
#include "../Common/CoderMixer2ST.h"
#ifndef EXCLUDE_COM
#include "../Common/CoderLoader.h"
#endif
#include "7zMethods.h"
#include "7zItem.h"

namespace NArchive {
namespace N7z {

class CEncoder
{
  #ifndef EXCLUDE_COM
  // CMethodMap _methodMap;
  // it must be in top of objects
  CCoderLibraries _libraries;
  #endif

  NCoderMixer2::CCoderMixer2MT *_mixerCoderSpec;
  CMyComPtr<ICompressCoder2> _mixerCoder;

  CObjectVector<CCoderInfo> _codersInfo;

  CCompressionMethodMode _options;
  NCoderMixer2::CBindInfo _bindInfo;
  NCoderMixer2::CBindInfo _decompressBindInfo;
  NCoderMixer2::CBindReverseConverter *_bindReverseConverter;
  CRecordVector<CMethodID> _decompressionMethods;

  HRESULT CreateMixerCoder();

public:
  CEncoder(const CCompressionMethodMode &options);
  ~CEncoder();
  HRESULT Encode(ISequentialInStream *inStream,
      const UInt64 *inStreamSize,
      CFolder &folderItem,
      ISequentialOutStream *outStream,
      CRecordVector<UInt64> &packSizes,
      ICompressProgressInfo *compressProgress);
};

}}

#endif
