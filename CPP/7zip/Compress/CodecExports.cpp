// CodecExports.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../../Common/ComTry.h"
#include "../../Common/MyCom.h"

#include "../../Windows/PropVariant.h"

#include "../ICoder.h"

#include "../Common/RegisterCodec.h"

extern unsigned int g_NumCodecs;
extern const CCodecInfo *g_Codecs[];

extern unsigned int g_NumHashers;
extern const CHasherInfo *g_Hashers[];

static const UInt16 kDecodeId = 0x2790;
static const UInt16 kEncodeId = 0x2791;
static const UInt16 kHasherId = 0x2792;

DEFINE_GUID(CLSID_CCodec,
0x23170F69, 0x40C1, kDecodeId, 0, 0, 0, 0, 0, 0, 0, 0);

static inline HRESULT SetPropString(const char *s, unsigned int size, PROPVARIANT *value)
{
  if ((value->bstrVal = ::SysAllocStringByteLen(s, size)) != 0)
    value->vt = VT_BSTR;
  return S_OK;
}

static inline HRESULT SetPropGUID(const GUID &guid, PROPVARIANT *value)
{
  return SetPropString((const char *)&guid, sizeof(GUID), value);
}

static HRESULT SetClassID(CMethodId id, UInt16 typeId, PROPVARIANT *value)
{
  GUID clsId;
  clsId.Data1 = CLSID_CCodec.Data1;
  clsId.Data2 = CLSID_CCodec.Data2;
  clsId.Data3 = typeId;
  SetUi64(clsId.Data4, id);
  return SetPropGUID(clsId, value);
}

static HRESULT FindCodecClassId(const GUID *clsID, UInt32 isCoder2, bool isFilter, bool &encode, int &index)
{
  index = -1;
  if (clsID->Data1 != CLSID_CCodec.Data1 ||
      clsID->Data2 != CLSID_CCodec.Data2)
    return S_OK;
  encode = true;
  if (clsID->Data3 == kDecodeId)
    encode = false;
  else if (clsID->Data3 != kEncodeId)
    return S_OK;
  UInt64 id = GetUi64(clsID->Data4);
  for (unsigned i = 0; i < g_NumCodecs; i++)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    if (id != codec.Id || encode && !codec.CreateEncoder || !encode && !codec.CreateDecoder)
      continue;
    if (!isFilter && codec.IsFilter || isFilter && !codec.IsFilter ||
        codec.NumInStreams != 1 && !isCoder2 || codec.NumInStreams == 1 && isCoder2)
      return E_NOINTERFACE;
    index = i;
    return S_OK;
  }
  return S_OK;
}

STDAPI CreateCoder2(bool encode, int index, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  bool isCoder = (*iid == IID_ICompressCoder) != 0;
  bool isCoder2 = (*iid == IID_ICompressCoder2) != 0;
  bool isFilter = (*iid == IID_ICompressFilter) != 0;
  const CCodecInfo &codec = *g_Codecs[index];
  if (!isFilter && codec.IsFilter || isFilter && !codec.IsFilter ||
      codec.NumInStreams != 1 && !isCoder2 || codec.NumInStreams == 1 && isCoder2)
    return E_NOINTERFACE;
  if (encode)
  {
    if (!codec.CreateEncoder)
      return CLASS_E_CLASSNOTAVAILABLE;
    *outObject = codec.CreateEncoder();
  }
  else
  {
    if (!codec.CreateDecoder)
      return CLASS_E_CLASSNOTAVAILABLE;
    *outObject = codec.CreateDecoder();
  }
  if (*outObject)
  {
    if (isCoder)
      ((ICompressCoder *)*outObject)->AddRef();
    else if (isCoder2)
      ((ICompressCoder2 *)*outObject)->AddRef();
    else
      ((ICompressFilter *)*outObject)->AddRef();
  }
  return S_OK;
  COM_TRY_END
}

STDAPI CreateCoder(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  bool isCoder = (*iid == IID_ICompressCoder) != 0;
  bool isCoder2 = (*iid == IID_ICompressCoder2) != 0;
  bool isFilter = (*iid == IID_ICompressFilter) != 0;
  if (!isCoder && !isCoder2 && !isFilter)
    return E_NOINTERFACE;
  bool encode;
  int codecIndex;
  HRESULT res = FindCodecClassId(clsid, isCoder2, isFilter, encode, codecIndex);
  if (res != S_OK)
    return res;
  if (codecIndex < 0)
    return CLASS_E_CLASSNOTAVAILABLE;

  const CCodecInfo &codec = *g_Codecs[codecIndex];
  if (encode)
    *outObject = codec.CreateEncoder();
  else
    *outObject = codec.CreateDecoder();
  if (*outObject)
  {
    if (isCoder)
      ((ICompressCoder *)*outObject)->AddRef();
    else if (isCoder2)
      ((ICompressCoder2 *)*outObject)->AddRef();
    else
      ((ICompressFilter *)*outObject)->AddRef();
  }
  return S_OK;
  COM_TRY_END
}

STDAPI GetMethodProperty(UInt32 codecIndex, PROPID propID, PROPVARIANT *value)
{
  ::VariantClear((VARIANTARG *)value);
  const CCodecInfo &codec = *g_Codecs[codecIndex];
  switch (propID)
  {
    case NMethodPropID::kID:
      value->uhVal.QuadPart = (UInt64)codec.Id;
      value->vt = VT_UI8;
      break;
    case NMethodPropID::kName:
      if ((value->bstrVal = ::SysAllocString(codec.Name)) != 0)
        value->vt = VT_BSTR;
      break;
    case NMethodPropID::kDecoder:
      if (codec.CreateDecoder)
        return SetClassID(codec.Id, kDecodeId, value);
      break;
    case NMethodPropID::kEncoder:
      if (codec.CreateEncoder)
        return SetClassID(codec.Id, kEncodeId, value);
      break;
    case NMethodPropID::kInStreams:
      if (codec.NumInStreams != 1)
      {
        value->vt = VT_UI4;
        value->ulVal = (ULONG)codec.NumInStreams;
      }
      break;
  }
  return S_OK;
}

STDAPI GetNumberOfMethods(UINT32 *numCodecs)
{
  *numCodecs = g_NumCodecs;
  return S_OK;
}


static int FindHasherClassId(const GUID *clsID)
{
  if (clsID->Data1 != CLSID_CCodec.Data1 ||
      clsID->Data2 != CLSID_CCodec.Data2 ||
      clsID->Data3 != kHasherId)
    return -1;
  UInt64 id = GetUi64(clsID->Data4);
  for (unsigned i = 0; i < g_NumCodecs; i++)
    if (id == g_Hashers[i]->Id)
      return i;
  return -1;
}

static HRESULT CreateHasher2(UInt32 index, IHasher **hasher)
{
  COM_TRY_BEGIN
  *hasher = g_Hashers[index]->CreateHasher();
  if (*hasher)
    (*hasher)->AddRef();
  return S_OK;
  COM_TRY_END
}

STDAPI CreateHasher(const GUID *clsid, IHasher **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int index = FindHasherClassId(clsid);
  if (index < 0)
    return CLASS_E_CLASSNOTAVAILABLE;
  return CreateHasher2(index, outObject);
  COM_TRY_END
}

STDAPI GetHasherProp(UInt32 codecIndex, PROPID propID, PROPVARIANT *value)
{
  ::VariantClear((VARIANTARG *)value);
  const CHasherInfo &codec = *g_Hashers[codecIndex];
  switch (propID)
  {
    case NMethodPropID::kID:
      value->uhVal.QuadPart = (UInt64)codec.Id;
      value->vt = VT_UI8;
      break;
    case NMethodPropID::kName:
      if ((value->bstrVal = ::SysAllocString(codec.Name)) != 0)
        value->vt = VT_BSTR;
      break;
    case NMethodPropID::kEncoder:
      if (codec.CreateHasher)
        return SetClassID(codec.Id, kHasherId, value);
      break;
    case NMethodPropID::kDigestSize:
      value->ulVal = (ULONG)codec.DigestSize;
      value->vt = VT_UI4;
      break;
  }
  return S_OK;
}

class CHashers:
  public IHashers,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IHashers)

  STDMETHOD_(UInt32, GetNumHashers)();
  STDMETHOD(GetHasherProp)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(CreateHasher)(UInt32 index, IHasher **hasher);
};

STDAPI GetHashers(IHashers **hashers)
{
  COM_TRY_BEGIN
  *hashers = new CHashers;
  if (*hashers)
    (*hashers)->AddRef();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP_(UInt32) CHashers::GetNumHashers()
{
  return g_NumHashers;
}

STDMETHODIMP CHashers::GetHasherProp(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  return ::GetHasherProp(index, propID, value);
}

STDMETHODIMP CHashers::CreateHasher(UInt32 index, IHasher **hasher)
{
  return ::CreateHasher2(index, hasher);
}
