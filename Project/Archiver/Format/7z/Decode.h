// Decode.h

#pragma once

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "Interface/IInOutStreams.h"

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
  bool BindInfoExPrevIsDefinded;
  CBindInfoEx BindInfoExPrev;
  #ifndef COMPRESS_LZMA
  NRegistryInfo::CMethodToCLSIDMap aMethodMap;
  #endif
  CComObjectNoLock<NCoderMixer2::CCoderMixer2> *MixerCoderSpec;
  CComPtr<ICompressCoder2> MixerCoder;
  CObjectVector<CComPtr<ICompressCoder> > aDecoders;
  CObjectVector<CComPtr<ICompressCoder2> > aDecoders2;
public:
  CDecoder();
  HRESULT Decode(IInStream *anInStream,
      UINT64 aStartPos,
      const UINT64 *aPackSizes,
      const CFolderItemInfo &aFolderInfo, 
      ISequentialOutStream *anOutStream,
      ICompressProgressInfo *aCompressProgress);
};

}}

#endif
