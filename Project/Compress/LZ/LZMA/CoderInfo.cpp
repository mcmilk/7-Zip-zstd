// LZArithmetic/CoderInfo.cpp

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

HRESULT DecodeProperties(ISequentialInStream *anInStream, 
    UINT32 &aNumPosStateBits, 
    UINT32 &aLiteralPosStateBits, 
    UINT32 &aLiteralContextBits, 
    UINT32 &aDictionarySize)
{
  UINT32 aProcessesedSize;

  BYTE aByte;
  RETURN_IF_NOT_S_OK(anInStream->Read(&aByte, sizeof(aByte), &aProcessesedSize));
  if (aProcessesedSize != sizeof(aByte))
    return E_INVALIDARG;

  aLiteralContextBits = aByte % 9;
  BYTE aRemainder = aByte / 9;
  aLiteralPosStateBits = aRemainder % 5;
  aNumPosStateBits = aRemainder / 5;

  RETURN_IF_NOT_S_OK(anInStream->Read(&aDictionarySize, sizeof(aDictionarySize), &aProcessesedSize));
  if (aProcessesedSize != sizeof(aDictionarySize))
    return E_INVALIDARG;
  return S_OK;
}

/*
STDMETHODIMP CCoderInfo::GetPropertyInfoEnumerator(IEnumSTATPROPSTG *anEnumProperty)
{
  return GetPropertyEnumerator(kCoderProperties, kNumCoderProperties, anEnumProperty);
}

STDMETHODIMP CCoderInfo::GetPropertyValueRange(UINT32 aPropID, UINT32 *aMinValue, UINT32 *aMaxValue)
{
  if (aPropID != NEncodedStreamProperies::kDictionarySize)
    return E_FAIL;
  *aMinValue = (1 << kDictionaryLogaritmicSizeMin);
  *aMaxValue = (1 << kDictionaryLogaritmicSizeMax);
  return S_OK;
}

STDMETHODIMP CCoderInfo::GetDecoderProperties(ISequentialInStream *anInStream, 
      PROPVARIANT *aProperties, UINT32 aNumProperties)
{
  if (aNumProperties != 1)
    return E_INVALIDARG;
  UINT32 aDictionarySize;
  RETURN_IF_NOT_S_OK(DecodeProperties(anInStream, aDictionarySize));
  aProperties[0].vt = VT_UI4;
  aProperties[0].ulVal = aDictionarySize;
  return S_OK;
}

STDMETHODIMP CEncoderInfo::GetPropertyInfoEnumerator(IEnumSTATPROPSTG *anEnumProperty)
{
  return GetPropertyEnumerator(kEncoderProperties, kNumEncoderProperties, anEnumProperty);
}

STDMETHODIMP CEncoderInfo::GetPropertyValueRange(UINT32 aPropID, UINT32 *aMinValue, UINT32 *aMaxValue)
{
  switch(aPropID)
  {
    case NEncodedStreamProperies::kDictionarySize:
      *aMinValue = (1 << kDictionaryLogaritmicSizeMin);
      *aMaxValue = (1 << kDictionaryLogaritmicSizeMax);
      break;
    case NEncodingProperies::kNumFastBytes:
      *aMinValue = 4;
      *aMaxValue = kMatchMaxLen;
      break;
    default:
      return E_INVALIDARG;
  }
  return S_OK;
}
*/

}}

