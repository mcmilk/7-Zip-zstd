// Copy/Coder.h

#pragma once

#ifndef __COPY_CODER_H
#define __COPY_CODER_H

#include "Interface/ICoder.h"

// {23170F69-40C1-278B-0000-000000000000}
DEFINE_GUID(CLSID_CCompressCopyCoder, 
0x23170F69, 0x40C1, 0x278B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCompress {

class CCopyCoder :
  public ICompressCoder,
  public CComObjectRoot,
  public CComCoClass<CCopyCoder , &CLSID_CCompressCopyCoder>
{
  BYTE *m_Buffer;
public:
  CCopyCoder();
  ~CCopyCoder();

  BEGIN_COM_MAP(CCopyCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCopyCoder)

  // DECLARE_NO_REGISTRY()

  DECLARE_REGISTRY(CCopyCoder, "Compress.CopyCoder.1", "Compress.CopyCoder", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);
};

}

#endif