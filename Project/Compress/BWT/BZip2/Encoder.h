// Compress/BZip2/Encoder.h

#pragma once

#ifndef __COMPRESS_BZIP2_ENCODER_H
#define __COMPRESS_BZIP2_ENCODER_H

#include "../../Interface/CompressInterface.h"

// {23170F69-40C1-278B-0402-020000000100}
DEFINE_GUID(CLSID_CCompressBZip2Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00);


namespace NCompress {
namespace NBZip2 {
namespace NEncoder {

class CCoder :
  public ICompressCoder,
//  public ICompressSetEncoderProperties,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressBZip2Encoder>
{
  BYTE *m_InBuffer;
  BYTE *m_OutBuffer;

public:
  CCoder();
  ~CCoder();

  BEGIN_COM_MAP(CCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
    // COM_INTERFACE_ENTRY(ICompressSetEncoderProperties)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCoder)

  // DECLARE_NO_REGISTRY()
  DECLARE_REGISTRY(CEncoder, TEXT("Compress.BZip2Encoder.1"), 
  TEXT("Compress.BZip2Encoder"), 0, THREADFLAGS_APARTMENT)

  // STDMETHOD(ReleaseStreams)();

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  // ICompressSetEncoderProperties
  // STDMETHOD(SetEncoderProperties)(const PROPVARIANT *aProperties, UINT32 aNumProperties);
};

}}}

#endif