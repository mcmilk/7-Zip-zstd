// CabCopyDecoder.h

#ifndef __ARCHIVE_CAB_COPY_DECODER_H
#define __ARCHIVE_CAB_COPY_DECODER_H

#pragma once

#include "Common/MyCom.h"
#include "../../ICoder.h"
#include "../../Common/OutBuffer.h"
#include "CabInBuffer.h"

namespace NArchive {
namespace NCab {

class CCopyDecoder:
  public ICompressCoder,
  public CMyUnknownImp
{
  CInBuffer m_InStream;
  COutBuffer m_OutStream;
  BYTE m_ReservedSize;
  UINT32 m_NumInDataBlocks;
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // void ReleaseStreams();
  HRESULT Flush() { return m_OutStream.Flush(); }
  void SetParams(BYTE reservedSize, UINT32 numInDataBlocks) 
  { 
    m_ReservedSize = reservedSize;
    m_NumInDataBlocks = numInDataBlocks;
  }
  
};

}}

#endif