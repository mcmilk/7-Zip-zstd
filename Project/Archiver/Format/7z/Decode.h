// Decode.h

#pragma once

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "Interface/IInOutStreams.h"
#include "Interface/CryptoInterface.h"

#include "../Common/CoderMixer2.h"

#include "RegistryInfo.h"
#include "ItemInfo.h"

namespace NArchive {
namespace N7z {

struct CBindInfoEx: public NCoderMixer2::CBindInfo
{
  CRecordVector<CMethodID> CoderMethodIDs;
};

class CDecoder
{
  bool _bindInfoExPrevIsDefinded;
  CBindInfoEx _bindInfoExPrev;
  #ifndef COMPRESS_LZMA
  NRegistryInfo::CMethodToCLSIDMap _methodMap;
  #endif
  CComObjectNoLock<NCoderMixer2::CCoderMixer2> *_mixerCoderSpec;
  CComPtr<ICompressCoder2> _mixerCoder;
  CObjectVector<CComPtr<ICompressCoder> > _decoders;
  CObjectVector<CComPtr<ICompressCoder2> > _decoders2;
public:
  CDecoder();
  HRESULT Decode(IInStream *inStream,
      UINT64 startPos,
      const UINT64 *packSizes,
      const CFolderItemInfo &folderInfo, 
      ISequentialOutStream *outStream,
      ICompressProgressInfo *compressProgress,
      ICryptoGetTextPassword *getTextPasswordSpec);
};

}}

#endif
