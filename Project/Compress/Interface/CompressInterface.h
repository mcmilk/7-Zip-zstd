// CompressInterface.h

#pragma once

#ifndef __COMPRESSINTERFACE_H
#define __COMPRESSINTERFACE_H

#include "Interface/IInOutStreams.h"
#include "Interface/ICoder.h"
#include "Common/Types.h"

// {23170F69-40C1-278A-0000-000200010000}
DEFINE_GUID(IID_IInWindowStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200010000")
IInWindowStream: public IUnknown
{
  STDMETHOD(Init)(ISequentialInStream *aStream) PURE;
  STDMETHOD_(void, ReleaseStream)() PURE;
  STDMETHOD(MovePos)() PURE;
  STDMETHOD_(BYTE, GetIndexByte)(UINT32 anIndex) PURE;
  STDMETHOD_(UINT32, GetMatchLen)(UINT32 aIndex, UINT32 aBack, UINT32 aLimit) PURE;
  STDMETHOD_(UINT32, GetNumAvailableBytes)() PURE;
  STDMETHOD_(const BYTE *, GetPointerToCurrentPos)() PURE;
};
 
// {23170F69-40C1-278A-0000-000200020000}
DEFINE_GUID(IID_IInWindowStreamMatch, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200020000")
IInWindowStreamMatch: public IInWindowStream
{
  STDMETHOD(Create)(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, 
      UINT32 aMatchMaxLen, UINT32 aKeepAddBufferAfter) PURE;
  STDMETHOD_(UINT32, GetLongestMatch)(UINT32 *aDistances) PURE;
  STDMETHOD_(void, DummyLongestMatch)() PURE;
};

// {23170F69-40C1-278A-0000-000200030000}
DEFINE_GUID(IID_IInitMatchFinder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200030000")
IInitMatchFinder: public IUnknown
{
  STDMETHOD(InitMatchFinder)(IInWindowStreamMatch *aMatchFinder) PURE;
};


namespace NEncodedStreamProperies
{
  enum EEnum
  {
    kDictionarySize = 0x400,
    kUsedMemorySize,
    kOrder,
    kPosStateBits = 0x700,
    kLitContextBits,
    kLitPosBits,
  };
};

namespace NEncodingProperies
{
  enum EEnum
  {
    kNumPasses = 0x800, 
    kNumFastBytes
  };
}


// {23170F69-40C1-278A-0000-000200170000}
DEFINE_GUID(IID_IGetPropertiesInfo, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x17, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200170000")
IGetPropertiesInfo: public IUnknown
{
  STDMETHOD(GetPropertyInfoEnumerator)(IEnumSTATPROPSTG *anEnumSTATPROPSTG) PURE;
  STDMETHOD(GetPropertyValueRange)(UINT32 aPropID, UINT32 *aMinValue, UINT32 *aMaxValue) PURE;
};

// {23170F69-40C1-278A-0000-000200180000}
DEFINE_GUID(IID_ICompressCoder2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x18, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200180000")
ICompressCoder2: public IUnknown
{
  STDMETHOD(Code)(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes, 
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams, 
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress) PURE;
};

// {23170F69-40C1-278A-0000-000200190000}
DEFINE_GUID(IID_ICompressSetEncoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x19, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200190000")
ICompressSetEncoderProperties: public IUnknown
{
  STDMETHOD(SetEncoderProperties)(const PROPVARIANT *aProperties, UINT32 aNumProperties) PURE;
};

// {23170F69-40C1-278A-0000-000200190001}
DEFINE_GUID(IID_ICompressSetEncoderProperties2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x19, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200190001")
ICompressSetEncoderProperties2: public IUnknown
{
  STDMETHOD(SetEncoderProperties2)(const PROPID *aPropIDs, 
      const PROPVARIANT *aProperties, UINT32 aNumProperties) PURE;
};

// {23170F69-40C1-278A-0000-000200200000}
DEFINE_GUID(IID_ICompressSetCoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200200000")
ICompressSetCoderProperties: public IUnknown
{
  STDMETHOD(SetCoderProperties)(const PROPVARIANT *aProperties, 
      UINT32 aNumProperties) PURE;
};

// {23170F69-40C1-278A-0000-000200200001}
DEFINE_GUID(IID_ICompressSetCoderProperties2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200200001")
ICompressSetCoderProperties2: public IUnknown
{
  STDMETHOD(SetCoderProperties2)(const PROPID *aPropIDs, 
      const PROPVARIANT *aProperties, UINT32 aNumProperties) PURE;
};

// {23170F69-40C1-278A-0000-000200210000}
DEFINE_GUID(IID_ICompressSetDecoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x21, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200210000")
ICompressSetDecoderProperties: public IUnknown
{
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *anInStream) PURE;
};

// {23170F69-40C1-278A-0000-000200220000}
DEFINE_GUID(IID_ICompressGetDecoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x22, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200220000")
ICompressGetDecoderProperties: public IUnknown
{
  STDMETHOD(GetDecoderProperties)(ISequentialInStream *anInStream, 
      PROPVARIANT *aProperties, UINT32 aNumProperties) PURE;
};

// {23170F69-40C1-278A-0000-000200230000}
DEFINE_GUID(IID_ICompressWriteCoderProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x23, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200230000")
ICompressWriteCoderProperties: public IUnknown
{
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *anOutStreams) PURE;
};

// {23170F69-40C1-278A-0000-000200240000}
DEFINE_GUID(IID_IGetInStreamProcessedSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x24, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200240000")
IGetInStreamProcessedSize: public IUnknown
{
  STDMETHOD(GetInStreamProcessedSize)(UINT64 *aValue) PURE;
};

// {23170F69-40C1-278A-0000-000200250000}
DEFINE_GUID(IID_ICompressGetSubStreamSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200250000")
ICompressGetSubStreamSize: public IUnknown
{
  STDMETHOD(GetSubStreamSize)(UINT64 aSubStream, UINT64 *aValue) PURE;
};


#endif
