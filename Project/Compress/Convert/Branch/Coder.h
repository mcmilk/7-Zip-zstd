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
  BYTE *m_Buffer;
public:
  CDataBuffer()
    { m_Buffer = new BYTE[kBufferSize]; }
  ~CDataBuffer()
    { delete []m_Buffer; }
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
/*DECLARE_REGISTRY(C ## Name, "Compress.Convert" ## "Name.1", */ \
                  /*"Compress.Convert" ## "Name", 0, THREADFLAGS_APARTMENT) */\
  DECLARE_REGISTRY(C ## Name, "Compress.ConvertBranch.1", "Compress.ConvertBranch", 0, THREADFLAGS_APARTMENT) \
  STDMETHOD(Code)(ISequentialInStream *anInStream, \
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize, \
      ICompressProgressInfo *aProgress); \
}; 

// {23170F69-40C1-278B-0303-010100000100}
#define MyClass2(Name, anId, aSubId, anEncodingId)  \
DEFINE_GUID(CLSID_CCompressConvert ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x03, 0x03, anId, aSubId, 0x00, 0x00, anEncodingId, 0x00); \
MyClass3(Name) \


#define MyClass(Name, anId, aSubId)  \
MyClass2(Name ## _Encoder, anId, aSubId, 0x01) \
MyClass2(Name ## _Decoder, anId, aSubId, 0x00) 

#define MyClassImp(Name) \
STDMETHODIMP C ## Name ## _Encoder::Code(ISequentialInStream *anInStream, \
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize, \
      ICompressProgressInfo *aProgress) \
{ \
  return Name ## _Code(anInStream, anOutStream, anInSize, anOutSize, \
      aProgress, m_Buffer, true); \
} \
STDMETHODIMP C ## Name ## _Decoder::Code(ISequentialInStream *anInStream, \
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize, \
      ICompressProgressInfo *aProgress) \
{ \
  return Name ## _Code(anInStream, anOutStream, anInSize, anOutSize, \
      aProgress, m_Buffer, false); \
}

#endif