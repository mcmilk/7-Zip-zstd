// Branch/Coder.h

#pragma once

#ifndef __CallPowerPC_CODER_H
#define __CallPowerPC_CODER_H

#include "../../Interface/CompressInterface.h"

#include "Common/Types.h"

const kBufferSize = 1 << 17;

class CDataBuffer
{
protected:
  BYTE *_buffer;
public:
  CDataBuffer()
    { _buffer = new BYTE[kBufferSize]; }
  ~CDataBuffer()
    { delete []_buffer; }
};

#define MyClass3(Name)  \
class C ## Name:  \
  public ICompressCoder, \
  public CDataBuffer, \
  public CComObjectRoot, \
  public CComCoClass<C ## Name, & CLSID_CCompressConvert ## Name> { \
public: \
BEGIN_COM_MAP(C ## Name) \
  COM_INTERFACE_ENTRY(ICompressCoder) \
END_COM_MAP() \
DECLARE_NOT_AGGREGATABLE(C ## Name) \
  DECLARE_REGISTRY(C ## Name, TEXT("Compress.ConvertBranch.1"), \
  TEXT("Compress.ConvertBranch"), 0, THREADFLAGS_APARTMENT) \
  STDMETHOD(Code)(ISequentialInStream *inStream, \
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize, \
      ICompressProgressInfo *progress); \
}; 

// {23170F69-40C1-278B-0303-010100000100}
#define MyClass2(Name, id, subId, encodingId)  \
DEFINE_GUID(CLSID_CCompressConvert ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x03, 0x03, id, subId, 0x00, 0x00, encodingId, 0x00); \
MyClass3(Name) \


#define MyClass(Name, id, subId)  \
MyClass2(Name ## _Encoder, id, subId, 0x01) \
MyClass2(Name ## _Decoder, id, subId, 0x00) 

#define MyClassImp(Name) \
STDMETHODIMP C ## Name ## _Encoder::Code(ISequentialInStream *inStream, \
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize, \
      ICompressProgressInfo *progress) \
{ \
  return Name ## _Code(inStream, outStream, inSize, outSize, \
      progress, _buffer, true); \
} \
STDMETHODIMP C ## Name ## _Decoder::Code(ISequentialInStream *inStream, \
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize, \
      ICompressProgressInfo *progress) \
{ \
  return Name ## _Code(inStream, outStream, inSize, outSize, \
      progress, _buffer, false); \
}

#endif