// Arj/Decoder2.h

#pragma once

#ifndef __COMPRESS_ARJ_DECODER2_H
#define __COMPRESS_ARJ_DECODER2_H

#include "Interface/ICoder.h"

#include "Stream/WindowOut.h"
#include "Stream/MSBFDecoder.h"
#include "Stream/InByte.h"

// {23170F69-40C1-278B-0404-020000000000}
DEFINE_GUID(CLSID_CCompressArj2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCompress {
namespace NArj {
namespace NDecoder2 {

class CCoder :
  public ICompressCoder,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressArj2Decoder>
{
  NStream::NWindow::COut m_OutWindowStream;
  NStream::NMSBF::CDecoder<NStream::CInByte> m_InBitStream;
  
  void CCoder::ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }
  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    CCoderReleaser(CCoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()
    {
      m_Coder->m_OutWindowStream.Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

public:
  CCoder();

  BEGIN_COM_MAP(CCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCoder)

  // DECLARE_NO_REGISTRY()

  DECLARE_REGISTRY(CEncoder, TEXT("Compress.Arj2Decoder.1"), 
  TEXT("Compress.Arj2Decoder"), 0, THREADFLAGS_APARTMENT)

  STDMETHOD(CodeReal)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

};

}}}

#endif