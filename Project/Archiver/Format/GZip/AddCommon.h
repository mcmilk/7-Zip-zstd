// GZip/AddCommon.h

#pragma once

#ifndef __GZIP_ADDCOMMON_H
#define __GZIP_ADDCOMMON_H

#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Interface/IProgress.h"
#include "../../../Compress/Interface/CompressInterface.h"

namespace NArchive {
namespace NGZip {

class CAddCommon
{
  CCompressionMethodMode m_Options;
  CComPtr<ICompressCoder> m_DeflateEncoder;
  // CComPtr<IInWindowStreamMatch> m_MatchFinder;
public:
  CAddCommon(const CCompressionMethodMode &anOptions) : m_Options(anOptions) {};
  HRESULT Compress(ISequentialInStream *anInStream, 
      ISequentialOutStream *anOutStream, 
      ICompressProgressInfo *aProgress);
};

}}

#endif
