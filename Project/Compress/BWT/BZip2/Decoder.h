// Compress/BZip2/Decoder.h

#pragma once

#ifndef __COMPRESS_BZIP2_DECODER_H
#define __COMPRESS_BZIP2_DECODER_H

#include "Interface/ICoder.h"
#include "../../Interface/CompressInterface.h"

// {23170F69-40C1-278B-0402-020000000000}
DEFINE_GUID(CLSID_CCompressBZip2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCompress {
namespace NBZip2 {
namespace NDecoder {

class CCoder :
  public ICompressCoder,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressBZip2Decoder>
{
  BYTE *m_InBuffer;
  BYTE *m_OutBuffer;
public:
  CCoder();
  ~CCoder();

  BEGIN_COM_MAP(CCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCoder)

  // DECLARE_NO_REGISTRY()

  DECLARE_REGISTRY(CEncoder, TEXT("Compress.BZip2Decoder.1"), 
  TEXT("Compress.BZip2Decoder"), UINT(0), THREADFLAGS_APARTMENT)

  HRESULT Flush();
  // void (ReleaseStreams)();

  STDMETHOD(CodeReal)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);
};

}}}

#endif