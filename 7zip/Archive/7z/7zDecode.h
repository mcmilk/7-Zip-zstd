// 7zDecode.h

#pragma once

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "../../IStream.h"
#include "../../IPassword.h"

#include "../Common/CoderMixer2.h"
#ifndef EXCLUDE_COM
#include "../Common/CoderLoader.h"
#endif

#include "7zItem.h"

namespace NArchive {
namespace N7z {

struct CBindInfoEx: public NCoderMixer2::CBindInfo
{
  CRecordVector<CMethodID> CoderMethodIDs;
};

class CDecoder
{
  #ifndef EXCLUDE_COM
  CCoderLibraries _libraries;
  #endif

  bool _bindInfoExPrevIsDefinded;
  CBindInfoEx _bindInfoExPrev;
  NCoderMixer2::CCoderMixer2 *_mixerCoderSpec;
  CMyComPtr<ICompressCoder2> _mixerCoder;
  CObjectVector<CMyComPtr<IUnknown> > _decoders;
  // CObjectVector<CMyComPtr<ICompressCoder2> > _decoders2;
public:
  CDecoder();
  HRESULT Decode(IInStream *inStream,
      UINT64 startPos,
      const UINT64 *packSizes,
      const CFolderItemInfo &folderInfo, 
      ISequentialOutStream *outStream,
      ICompressProgressInfo *compressProgress
      #ifndef _NO_CRYPTO
      , ICryptoGetTextPassword *getTextPasswordSpec
      #endif
      );
};

}}

#endif
