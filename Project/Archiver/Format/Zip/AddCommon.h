// Zip/AddCommon.h

#pragma once

#ifndef __ZIP_ADDCOMMON_H
#define __ZIP_ADDCOMMON_H

#include "Compression/CopyCoder.h"
#include "CompressionMethod.h"
#include "Interface/ICoder.h"
#include "Interface/IProgress.h"
#include "../../../Compress/Interface/CompressInterface.h"

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
  CCompressionMethodMode m_Options; // Why not &
  CComObjectNoLock<NCompression::CCopyCoder> *m_CopyCoderSpec;
  CComPtr<ICompressCoder> m_CopyCoder;
  CComPtr<ICompressCoder> m_DeflateEncoder;
  // CComPtr<IInWindowStreamMatch> m_MatchFinder;
public:
  CAddCommon(const CCompressionMethodMode &anOptions);
  HRESULT Compress(IInStream *anInStream, IOutStream *anOutStream, 
      UINT64 anInSize, ICompressProgressInfo *aProgress, CCompressingResult &aResult);
};

}}

#endif
