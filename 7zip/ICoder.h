// ICoder.h

// #pragma once

#ifndef __ICODER_H
#define __ICODER_H

#include "IStream.h"

// {23170F69-40C1-278A-0000-000200040000}
DEFINE_GUID(IID_ICompressProgressInfo, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200040000")
ICompressProgressInfo: public IUnknown
{
  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize) = 0;
};

// {23170F69-40C1-278A-0000-000200050000}
DEFINE_GUID(IID_ICompressCoder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200050000")
ICompressCoder: public IUnknown
{
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, 
      const UINT64 *outSize,
      ICompressProgressInfo *progress) = 0;
};

// {23170F69-40C1-278A-0000-000200180000}
DEFINE_GUID(IID_ICompressCoder2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x18, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200180000")
ICompressCoder2: public IUnknown
{
  STDMETHOD(Code)(ISequentialInStream **inStreams,
      const UINT64 **inSizes, 
      UINT32 numInStreams,
      ISequentialOutStream **outStreams, 
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress) PURE;
};

namespace NCoderPropID
{
  enum EEnum
  {
    kDictionarySize = 0x400,
    kUsedMemorySize,
    kOrder,
    kPosStateBits = 0x440,
    kLitContextBits,
    kLitPosBits,
    kNumFastBytes = 0x450,
    kMatchFinder,
    kNumPasses = 0x460, 
    kAlgorithm = 0x470,
    kMultiThread = 0x480
  };
}

// {23170F69-40C1-278A-0000-000200200000}
DEFINE_GUID(IID_ICompressSetCoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200200000")
ICompressSetCoderProperties: public IUnknown
{
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties) PURE;
};

// {23170F69-40C1-278A-0000-000200210000}
DEFINE_GUID(IID_ICompressSetDecoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x21, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200210000")
ICompressSetDecoderProperties: public IUnknown
{
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *anInStream) PURE;
};

// {23170F69-40C1-278A-0000-000200230000}
DEFINE_GUID(IID_ICompressWriteCoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x23, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200230000")
ICompressWriteCoderProperties: public IUnknown
{
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStreams) PURE;
};

// {23170F69-40C1-278A-0000-000200240000}
DEFINE_GUID(IID_ICompressGetInStreamProcessedSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x24, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200240000")
ICompressGetInStreamProcessedSize: public IUnknown
{
  STDMETHOD(GetInStreamProcessedSize)(UINT64 *value) PURE;
};

// {23170F69-40C1-278A-0000-000200250000}
DEFINE_GUID(IID_ICompressGetSubStreamSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200250000")
ICompressGetSubStreamSize: public IUnknown
{
  STDMETHOD(GetSubStreamSize)(UINT64 subStream, UINT64 *value) PURE;
};

//////////////////////
// It's for DLL file
namespace NMethodPropID
{
  enum EEnum
  {
    kID,
    kName,
    kDecoder,
    kEncoder,
    kInStreams,
    kOutStreams,
    kDescription,
  };
}

#endif
