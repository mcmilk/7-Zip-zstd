// Encode.h

#pragma once

#ifndef __7Z_ENCODE_H
#define __7Z_ENCODE_H

#include "Interface/IInOutStreams.h"
#include "Interface/StreamObjects.h"

#include "CompressionMethod.h"

#include "../Common/CoderMixer2.h"
#include "RegistryInfo.h"
#include "ItemInfo.h"

namespace NArchive {
namespace N7z {

class CEncoder
{
  CComObjectNoLock<NCoderMixer2::CCoderMixer2> *_mixerCoderSpec;
  CComPtr<ICompressCoder2> _mixerCoder;

  CObjectVector<CComPtr<ICompressCoder> > _encoders;
  CObjectVector<CComPtr<ICompressCoder2> > _encoders2;
  
  CObjectVector<NArchive::N7z::CCoderInfo> _codersInfo;

  CCompressionMethodMode _options;
  NCoderMixer2::CBindInfo _bindInfo;
  NCoderMixer2::CBindInfo _decompressBindInfo;
  NCoderMixer2::CBindReverseConverter *_bindReverseConverter;
  CRecordVector<CMethodID> _decompressionMethods;

  HRESULT CreateMixerCoder();

public:
  CEncoder(const CCompressionMethodMode *options);
  ~CEncoder();
  HRESULT Encode(ISequentialInStream *inStream,
      const UINT64 *inStreamSize,
      NArchive::N7z::CFolderItemInfo &folderItem,
      ISequentialOutStream *outStream,
      CRecordVector<UINT64> &packSizes,
      ICompressProgressInfo *compressProgress);
};

}}

#endif
