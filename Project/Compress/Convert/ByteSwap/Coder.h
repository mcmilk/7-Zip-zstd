// CJ86/Coder.h

#ifndef __CJ86_CODER_H
#define __CJ86_CODER_H

#include "../../Interface/CompressInterface.h"

#include "Common/Types.h"

// {23170F69-40C1-278B-0203-020000000000}
DEFINE_GUID(CLSID_CCompressConvertByteSwap2, 
0x23170F69, 0x40C1, 0x278B, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0203-040000000000}
DEFINE_GUID(CLSID_CCompressConvertByteSwap4, 
0x23170F69, 0x40C1, 0x278B, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00);

class CBuffer
{
protected:
  BYTE *_buffer;
public:
  CBuffer();
  ~CBuffer();
};


class CByteSwap2 : 
  public ICompressCoder,
  public CBuffer,
  public CComObjectRoot,
  public CComCoClass<CByteSwap2, &CLSID_CCompressConvertByteSwap2>
{
public:
BEGIN_COM_MAP(CByteSwap2)
  COM_INTERFACE_ENTRY(ICompressCoder)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CByteSwap2)
//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CByteSwap2, 
  // "Compress.ConvertSwap2.1", "Compress.ConvertSwap2", 
  TEXT("SevenZip.1"), TEXT("SevenZip"),
  UINT(0), THREADFLAGS_APARTMENT)
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
};

class CByteSwap4 : 
  public ICompressCoder,
  public CBuffer,
  public CComObjectRoot,
  public CComCoClass<CByteSwap2, &CLSID_CCompressConvertByteSwap4>
{
public:
BEGIN_COM_MAP(CByteSwap2)
  COM_INTERFACE_ENTRY(ICompressCoder)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CByteSwap4)
//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CByteSwap2, 
  // "Compress.ConvertSwap4.1", "Compress.ConvertSwap4", 
  TEXT("SevenZip.1"), TEXT("SevenZip"),
  UINT(0), THREADFLAGS_APARTMENT)
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
};


#endif