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
  CComObjectNoLock<NCoderMixer2::CCoderMixer2> *aMixerCoderSpec;
  CComPtr<ICompressCoder2> aMixerCoder;

  CObjectVector<CComPtr<ICompressCoder> > m_Encoders;
  CObjectVector<CComPtr<ICompressCoder2> > m_Encoders2;
  
  CObjectVector<NArchive::N7z::CCoderInfo> m_CodersInfo;

  CCompressionMethodMode m_Options;
  NCoderMixer2::CBindInfo m_BindInfo;
  NCoderMixer2::CBindInfo m_DecompressBindInfo;
  NCoderMixer2::CBindReverseConverter *m_BindReverseConverter;
  CRecordVector<CMethodID> m_DecompressionMethods;

public:
  CEncoder(const CCompressionMethodMode *anOptions);
  HRESULT Encode(ISequentialInStream *anInStream,
      const UINT64 *anInStreamSize,
      NArchive::N7z::CFolderItemInfo &aFolderItem,
      ISequentialOutStream *anOutStream,
      CRecordVector<UINT64> &aPackSizes,
      ICompressProgressInfo *aCompressProgress);
};

}}

#endif
