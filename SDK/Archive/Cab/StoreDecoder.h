// Archive/Cab/StoreDecoder.h

#ifndef __ARCHIVE_CAB_STOREDECODER_H
#define __ARCHIVE_CAB_STOREDECODER_H

#pragma once

#include "Interface/ICoder.h"
#include "Archive/Cab/DataInByte.h"
#include "Stream/OutByte.h"

namespace NArchive {
namespace NCab {

class CStoreDecoder:
  public ICompressCoder,
  public CComObjectRoot
{
  CInByte m_InStream;
  NStream::COutByte m_OutStream;

  BYTE m_ReservedSize;
  UINT32 m_NumInDataBlocks;

public:
  CStoreDecoder();
  ~CStoreDecoder();

  BEGIN_COM_MAP(CStoreDecoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CStoreDecoder)

  DECLARE_NO_REGISTRY()

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  void ReleaseStreams();
  STDMETHOD(Flush)();

  void SetParams(BYTE aReservedSize, UINT32 aNumInDataBlocks) 
  { 
    m_ReservedSize = aReservedSize;
    m_NumInDataBlocks = aNumInDataBlocks;
  }
  
};

}}

#endif