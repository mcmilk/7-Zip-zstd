// LZMA/CoderInfo.cpp

#include "StdAfx.h"

#include "CoderInfo.h"

#include "Windows/Defs.h"
#include "LZMA.h"

namespace NCompress {
namespace NLZMA {

STATPROPSTG kCoderProperties[] = 
{
  { NULL, NEncodedStreamProperies::kDictionarySize, VT_UI4}
};

static const kNumCoderProperties = sizeof(kCoderProperties) / sizeof(kCoderProperties[0]);

STATPROPSTG kEncoderProperties[] = 
{
  { NULL, NEncodingProperies::kNumFastBytes, VT_UI4}
};

static const kNumEncoderProperties = sizeof(kEncoderProperties) / sizeof(kEncoderProperties[0]);

HRESULT DecodeProperties(ISequentialInStream *inStream, 
    UINT32 &numPosStateBits, 
    UINT32 &numLiteralPosStateBits, 
    UINT32 &numLiteralContextBits, 
    UINT32 &dictionarySize)
{
  UINT32 processesedSize;

  BYTE firstByte;
  RETURN_IF_NOT_S_OK(inStream->Read(&firstByte, sizeof(firstByte), &processesedSize));
  if (processesedSize != sizeof(firstByte))
    return E_INVALIDARG;

  numLiteralContextBits = firstByte % 9;
  BYTE remainder = firstByte / 9;
  numLiteralPosStateBits = remainder % 5;
  numPosStateBits = remainder / 5;

  RETURN_IF_NOT_S_OK(inStream->Read(&dictionarySize, sizeof(dictionarySize), &processesedSize));
  if (processesedSize != sizeof(dictionarySize))
    return E_INVALIDARG;
  return S_OK;
}

/*
STDMETHODIMP CCoderInfo::GetPropertyInfoEnumerator(IEnumSTATPROPSTG *anEnumProperty)
{
  return GetPropertyEnumerator(kCoderProperties, kNumCoderProperties, anEnumProperty);
}

STDMETHODIMP CCoderInfo::GetPropertyValueRange(UINT32 propID, UINT32 *minValue, UINT32 *maxValue)
{
  if (propID != NEncodedStreamProperies::kDictionarySize)
    return E_FAIL;
  *minValue = (1 << kDictionaryLogaritmicSizeMin);
  *maxValue = (1 << kDictionaryLogaritmicSizeMax);
  return S_OK;
}

STDMETHODIMP CCoderInfo::GetDecoderProperties(ISequentialInStream *inStream, 
      PROPVARIANT *aProperties, UINT32 aNumProperties)
{
  if (aNumProperties != 1)
    return E_INVALIDARG;
  UINT32 dictionarySize;
  RETURN_IF_NOT_S_OK(DecodeProperties(inStream, dictionarySize));
  aProperties[0].vt = VT_UI4;
  aProperties[0].ulVal = dictionarySize;
  return S_OK;
}

STDMETHODIMP CEncoderInfo::GetPropertyInfoEnumerator(IEnumSTATPROPSTG *anEnumProperty)
{
  return GetPropertyEnumerator(kEncoderProperties, kNumEncoderProperties, anEnumProperty);
}

STDMETHODIMP CEncoderInfo::GetPropertyValueRange(UINT32 propID, UINT32 *minValue, UINT32 *maxValue)
{
  switch(propID)
  {
    case NEncodedStreamProperies::kDictionarySize:
      *minValue = (1 << kDictionaryLogaritmicSizeMin);
      *maxValue = (1 << kDictionaryLogaritmicSizeMax);
      break;
    case NEncodingProperies::kNumFastBytes:
      *minValue = 4;
      *maxValue = kMatchMaxLen;
      break;
    default:
      return E_INVALIDARG;
  }
  return S_OK;
}
*/

}}

