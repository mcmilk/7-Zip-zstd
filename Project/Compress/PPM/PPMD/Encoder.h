// Compress/PPM/PPMD/Encoder.h

#pragma once

#ifndef __COMPRESS_PPM_PPMD_ENCODER_H
#define __COMPRESS_PPM_PPMD_ENCODER_H

#include "../../Interface/CompressInterface.h"

#include "Common/Types.h"

#include "Stream/InByte.h"

#include "Encode.h"

#include "Compression/RangeCoder.h"

// {23170F69-40C1-278B-0304-010000000100}
DEFINE_GUID(CLSID_CompressPPMDEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x03, 0x04, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00);


namespace NCompress {
namespace NPPMD {

class CEncoder : 
  public ICompressCoder,
  public ICompressSetCoderProperties2,
  public ICompressWriteCoderProperties,
  public CComObjectRoot,
  public CComCoClass<CEncoder, &CLSID_CompressPPMDEncoder>
{
public:
  NStream::CInByte _inStream;

  CMyRangeEncoder _rangeEncoder;

  CEncodeInfo _info;
  UINT32 _usedMemorySize;
  BYTE _order;

public:

  BEGIN_COM_MAP(CEncoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICompressSetCoderProperties2)
  COM_INTERFACE_ENTRY(ICompressWriteCoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEncoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CEncoder, 
    // TEXT("Compress.PPMDEncoder.1"), TEXT("Compress.PPMDEncoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  // ICoder interface
  HRESULT Flush();
  void ReleaseStreams()
  {
    _inStream.ReleaseStream();
    _rangeEncoder.ReleaseStream();
  }

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(SetCoderProperties2)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties);

  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CEncoder();

};

}}

#endif