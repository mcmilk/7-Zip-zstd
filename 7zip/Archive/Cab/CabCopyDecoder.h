// CabCopyDecoder.h

#ifndef __ARCHIVE_CAB_COPY_DECODER_H
#define __ARCHIVE_CAB_COPY_DECODER_H

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
  Byte m_ReservedSize;
  UInt32 m_NumInDataBlocks;
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  void ReleaseStreams();
  HRESULT Flush() { return m_OutStream.Flush(); }
  void SetParams(Byte reservedSize, UInt32 numInDataBlocks) 
  { 
    m_ReservedSize = reservedSize;
    m_NumInDataBlocks = numInDataBlocks;
  }
  
};

}}

#endif