// Compression/CopyCoder.h

#pragma once

#ifndef __COMPRESSION_COPYCODER_H
#define __COMPRESSION_COPYCODER_H

#include "Interface/ICoder.h"

// {23170F69-40C1-278B-0000-000000000000}
DEFINE_GUID(CLSID_CCompressionCopyCoder, 
0x23170F69, 0x40C1, 0x278B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCompression {

class CCopyCoder :
  public ICompressCoder,
  public CComObjectRoot,
  public CComCoClass<CCopyCoder, &CLSID_CCompressionCopyCoder>
{
  BYTE *_buffer;
public:
  UINT64 TotalSize;
  CCopyCoder();
  ~CCopyCoder();

  BEGIN_COM_MAP(CCopyCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCopyCoder)

  // DECLARE_NO_REGISTRY()
  DECLARE_REGISTRY(CCopyCoder, 
    // TEXT("Compress.CopyCoder.1"), TEXT("Compress.CopyCoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
};

}

#endif