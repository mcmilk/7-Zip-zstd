// LZMA/CoderInfo.h

#pragma once 

#ifndef __LZMA_CODERINFO_H
#define __LZMA_CODERINFO_H

#include "Interface/ICoder.h"

#include "../../Interface/CompressInterface.h"

/*
// {23170F69-40C1-278A-1000-000200060001}
DEFINE_GUID(CLSID_CLZMACoderInfo, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0x01);

// {23170F69-40C1-278A-1000-000200060002}
DEFINE_GUID(CLSID_CLZHuffmanEncoderInfo, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x00, 0x06, 0x00, 0x02);
*/

namespace NCompress {
namespace NLZMA {

HRESULT DecodeProperties(ISequentialInStream *inStream, 
    UINT32 &numPosStateBits, 
    UINT32 &numLiteralPosStateBits, 
    UINT32 &numLiteralContextBits, 
    UINT32 &dictionarySize);

/*
class CCoderInfo : 
  public IGetPropertiesInfo,
  public ICompressGetDecoderProperties,
  public CComObjectRoot,
  public CComCoClass<CCoderInfo, &CLSID_CLZMACoderInfo>
{
public:

BEGIN_COM_MAP(CCoderInfo)
  COM_INTERFACE_ENTRY(IGetPropertiesInfo)
  COM_INTERFACE_ENTRY(ICompressGetDecoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCoderInfo)

DECLARE_REGISTRY(CCoderInfo, "Compress.LZMACoderInfo.1", "Compress.LZMACoderInfo", 0, THREADFLAGS_APARTMENT)

  //DECLARE_NO_REGISTRY()


  // IGetPropertyInfoEnumerator interface
  STDMETHOD(GetPropertyInfoEnumerator)(IEnumSTATPROPSTG *anEnumProperty);
  STDMETHOD(GetPropertyValueRange)(UINT32 aPropID, UINT32 *minValue, UINT32 *maxValue);
  STDMETHOD(GetDecoderProperties)(ISequentialInStream *inStream, 
      PROPVARIANT *aProperties, UINT32 aNumProperties);
};

class CEncoderInfo: 
  public IGetPropertiesInfo,
  public CComObjectRoot,
  public CComCoClass<CEncoderInfo, &CLSID_CLZHuffmanEncoderInfo>
{
public:

BEGIN_COM_MAP(CEncoderInfo)
  COM_INTERFACE_ENTRY(IGetPropertiesInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEncoderInfo)

DECLARE_REGISTRY(CCoderInfo, "Compress.LZMAEncoderInfo.1", "Compress.LZMAEncoderInfo", 0, THREADFLAGS_APARTMENT)

  //DECLARE_NO_REGISTRY()

  // IGetPropertyInfoEnumerator interface
  STDMETHOD(GetPropertyInfoEnumerator)(IEnumSTATPROPSTG *anEnumProperty);
  STDMETHOD(GetPropertyValueRange)(UINT32 aPropID, UINT32 *minValue, UINT32 *maxValue);
};
*/

}}

#endif
