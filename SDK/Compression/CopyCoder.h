// Compression::CopyCoder.h

#pragma once

#ifndef __COMPRESSION_COPYCODER_H
#define __COMPRESSION_COPYCODER_H

#include "Interface/ICoder.h"

// {3D3BBF41-6593-11d3-BFBD-000001009116}
DEFINE_GUID(CLSID_CCopyCoder, 
0x3d3bbf41, 0x6593, 0x11d3, 0xbf, 0xbd, 0x0, 0x0, 0x1, 0x0, 0x91, 0x16);

namespace NCompression {

class CCopyCoder :
  public ICompressCoder,
  public CComObjectRoot,
  public CComCoClass<CCopyCoder , &CLSID_CCopyCoder>
{
  BYTE *m_Buffer;
public:
  CCopyCoder();
  ~CCopyCoder();

  BEGIN_COM_MAP(CCopyCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCopyCoder)

  DECLARE_NO_REGISTRY()

  /*
  STDMETHOD(Init)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream);
  STDMETHOD(ReleaseStreams)();
  STDMETHOD(Code)(UINT32 aSize, UINT32 &aProcessedSize);
  STDMETHOD(Flush)();
  */

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);
};

}

#endif