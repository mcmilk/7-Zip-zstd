// ICoder.h

#ifndef __ICODER_H
#define __ICODER_H

#include "IStream.h"

// {23170F69-40C1-278A-0000-000200040000}
DEFINE_GUID(IID_ICompressProgressInfo, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200040000")
ICompressProgressInfo: public IUnknown
{
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize) = 0;
};

// {23170F69-40C1-278A-0000-000200050000}
DEFINE_GUID(IID_ICompressCoder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200050000")
ICompressCoder: public IUnknown
{
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, 
      const UInt64 *outSize,
      ICompressProgressInfo *progress) = 0;
};

// {23170F69-40C1-278A-0000-000200180000}
DEFINE_GUID(IID_ICompressCoder2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x18, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200180000")
ICompressCoder2: public IUnknown
{
  STDMETHOD(Code)(ISequentialInStream **inStreams,
      const UInt64 **inSizes, 
      UInt32 numInStreams,
      ISequentialOutStream **outStreams, 
      const UInt64 **outSizes,
      UInt32 numOutStreams,
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
    kMultiThread = 0x480,
    kEndMarker = 0x490
  };
}

// {23170F69-40C1-278A-0000-000200200000}
DEFINE_GUID(IID_ICompressSetCoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200200000")
ICompressSetCoderProperties: public IUnknown
{
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties) PURE;
};

/*
// {23170F69-40C1-278A-0000-000200210000}
DEFINE_GUID(IID_ICompressSetDecoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x21, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200210000")
ICompressSetDecoderProperties: public IUnknown
{
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream) PURE;
};
*/

// {23170F69-40C1-278A-0000-000200210200}
DEFINE_GUID(IID_ICompressSetDecoderProperties2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x21, 0x02, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200210200")
ICompressSetDecoderProperties2: public IUnknown
{
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size) PURE;
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
  STDMETHOD(GetInStreamProcessedSize)(UInt64 *value) PURE;
};

// {23170F69-40C1-278A-0000-000200250000}
DEFINE_GUID(IID_ICompressGetSubStreamSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200250000")
ICompressGetSubStreamSize: public IUnknown
{
  STDMETHOD(GetSubStreamSize)(UInt64 subStream, UInt64 *value) PURE;
};

// {23170F69-40C1-278A-0000-000200260000}
DEFINE_GUID(IID_ICompressSetInStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x26, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200260000")
ICompressSetInStream: public IUnknown
{
  STDMETHOD(SetInStream)(ISequentialInStream *inStream) PURE;
  STDMETHOD(ReleaseInStream)() PURE;
};

// {23170F69-40C1-278A-0000-000200270000}
DEFINE_GUID(IID_ICompressSetOutStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x27, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200270000")
ICompressSetOutStream: public IUnknown
{
  STDMETHOD(SetOutStream)(ISequentialOutStream *outStream) PURE;
  STDMETHOD(ReleaseOutStream)() PURE;
};

// {23170F69-40C1-278A-0000-000200280000}
DEFINE_GUID(IID_ICompressSetInStreamSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200280000")
ICompressSetInStreamSize: public IUnknown
{
  STDMETHOD(SetInStreamSize)(const UInt64 *inSize) PURE;
};

// {23170F69-40C1-278A-0000-000200290000}
DEFINE_GUID(IID_ICompressSetOutStreamSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x29, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200290000")
ICompressSetOutStreamSize: public IUnknown
{
  STDMETHOD(SetOutStreamSize)(const UInt64 *outSize) PURE;
};

// {23170F69-40C1-278A-0000-000200400000}
DEFINE_GUID(IID_ICompressFilter, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x40, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200400000")
ICompressFilter: public IUnknown
{
  STDMETHOD(Init)() PURE;
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size) PURE;
  // Filter return outSize (UInt32)
  // if (outSize <= size): Filter have converted outSize bytes
  // if (outSize > size): Filter have not converted anything.
  //      and it needs at least outSize bytes to convert one block 
  //      (it's for crypto block algorithms).
};

// {23170F69-40C1-278A-0000-000200800000}
DEFINE_GUID(IID_ICryptoProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x80, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200800000")
ICryptoProperties: public IUnknown
{
  STDMETHOD(SetKey)(const Byte *data, UInt32 size) PURE;
  STDMETHOD(SetInitVector)(const Byte *data, UInt32 size) PURE;
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
    kDescription
  };
}

#endif
