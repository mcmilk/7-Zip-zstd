// Compress/PPM/PPMDDecoder.h

#pragma once

#ifndef __COMPRESS_PPM_PPMD_DECODER_H
#define __COMPRESS_PPM_PPMD_DECODER_H

#include "../../Interface/CompressInterface.h"

#include "Common/Types.h"

#include "Stream/OutByte.h"

#include "Compression/RangeCoder.h"

#include "Decode.h"

// {23170F69-40C1-278B-0304-010000000000}
DEFINE_GUID(CLSID_CompressPPMDDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x03, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCompress {
namespace NPPMD {

class CDecoder : 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CompressPPMDDecoder>
{
  CMyRangeDecoder _rangeDecoder;

  NStream::COutByte _outStream;

  CDecodeInfo _info;

  BYTE _order;
  UINT32 _usedMemorySize;

public:

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICompressSetDecoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CDecoder, 
    // TEXT("Compress.PPMDDecoder.1"), TEXT("Compress.PPMDDecoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  void ReleaseStreams()
  {
    _rangeDecoder.ReleaseStream();
    _outStream.ReleaseStream();
  }

  // STDMETHOD(Code)(UINT32 aNumBytes, UINT32 &aProcessedBytes);
  HRESULT Flush()
    { return _outStream.Flush(); }


  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);


  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);

};

}}

#endif
