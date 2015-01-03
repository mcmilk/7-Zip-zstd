// CreateCoder.cpp

#include "StdAfx.h"

#include "../../Windows/Defs.h"
#include "../../Windows/PropVariant.h"

#include "CreateCoder.h"

#include "FilterCoder.h"
#include "RegisterCodec.h"

static const unsigned int kNumCodecsMax = 64;
unsigned int g_NumCodecs = 0;
const CCodecInfo *g_Codecs[kNumCodecsMax];
void RegisterCodec(const CCodecInfo *codecInfo) throw()
{
  if (g_NumCodecs < kNumCodecsMax)
    g_Codecs[g_NumCodecs++] = codecInfo;
}

static const unsigned int kNumHashersMax = 16;
unsigned int g_NumHashers = 0;
const CHasherInfo *g_Hashers[kNumHashersMax];
void RegisterHasher(const CHasherInfo *hashInfo) throw()
{
  if (g_NumHashers < kNumHashersMax)
    g_Hashers[g_NumHashers++] = hashInfo;
}

#ifdef EXTERNAL_CODECS
static HRESULT ReadNumberOfStreams(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, UInt32 &res)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(codecsInfo->GetProperty(index, propID, &prop));
  if (prop.vt == VT_EMPTY)
    res = 1;
  else if (prop.vt == VT_UI4)
    res = prop.ulVal;
  else
    return E_INVALIDARG;
  return S_OK;
}

static HRESULT ReadIsAssignedProp(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, bool &res)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(codecsInfo->GetProperty(index, propID, &prop));
  if (prop.vt == VT_EMPTY)
    res = true;
  else if (prop.vt == VT_BOOL)
    res = VARIANT_BOOLToBool(prop.boolVal);
  else
    return E_INVALIDARG;
  return S_OK;
}

HRESULT CExternalCodecs::LoadCodecs()
{
  if (GetCodecs)
  {
    UInt32 num;
    RINOK(GetCodecs->GetNumberOfMethods(&num));
    for (UInt32 i = 0; i < num; i++)
    {
      CCodecInfoEx info;
      NWindows::NCOM::CPropVariant prop;
      RINOK(GetCodecs->GetProperty(i, NMethodPropID::kID, &prop));
      // if (prop.vt != VT_BSTR)
      // info.Id.IDSize = (Byte)SysStringByteLen(prop.bstrVal);
      // memcpy(info.Id.ID, prop.bstrVal, info.Id.IDSize);
      if (prop.vt != VT_UI8)
        continue; // old Interface
      info.Id = prop.uhVal.QuadPart;
      prop.Clear();
      
      RINOK(GetCodecs->GetProperty(i, NMethodPropID::kName, &prop));
      if (prop.vt == VT_BSTR)
        info.Name = prop.bstrVal;
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
      
      RINOK(ReadNumberOfStreams(GetCodecs, i, NMethodPropID::kInStreams, info.NumInStreams));
      RINOK(ReadNumberOfStreams(GetCodecs, i, NMethodPropID::kOutStreams, info.NumOutStreams));
      RINOK(ReadIsAssignedProp(GetCodecs, i, NMethodPropID::kEncoderIsAssigned, info.EncoderIsAssigned));
      RINOK(ReadIsAssignedProp(GetCodecs, i, NMethodPropID::kDecoderIsAssigned, info.DecoderIsAssigned));
      
      Codecs.Add(info);
    }
  }
  if (GetHashers)
  {
    UInt32 num = GetHashers->GetNumHashers();
    for (UInt32 i = 0; i < num; i++)
    {
      CHasherInfoEx info;
      NWindows::NCOM::CPropVariant prop;
      RINOK(GetHashers->GetHasherProp(i, NMethodPropID::kID, &prop));
      if (prop.vt != VT_UI8)
        continue;
      info.Id = prop.uhVal.QuadPart;
      prop.Clear();
      
      RINOK(GetHashers->GetHasherProp(i, NMethodPropID::kName, &prop));
      if (prop.vt == VT_BSTR)
        info.Name = prop.bstrVal;
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
      
      Hashers.Add(info);
    }
  }
  return S_OK;
}

#endif

bool FindMethod(DECL_EXTERNAL_CODECS_LOC_VARS
    const UString &name, CMethodId &methodId, UInt32 &numInStreams, UInt32 &numOutStreams)
{
  UInt32 i;
  for (i = 0; i < g_NumCodecs; i++)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    if (name.IsEqualToNoCase(codec.Name))
    {
      methodId = codec.Id;
      numInStreams = codec.NumInStreams;
      numOutStreams = 1;
      return true;
    }
  }
  #ifdef EXTERNAL_CODECS
  if (__externalCodecs)
    for (i = 0; i < (UInt32)__externalCodecs->Codecs.Size(); i++)
    {
      const CCodecInfoEx &codec = __externalCodecs->Codecs[i];
      if (codec.Name.IsEqualToNoCase(name))
      {
        methodId = codec.Id;
        numInStreams = codec.NumInStreams;
        numOutStreams = codec.NumOutStreams;
        return true;
      }
    }
  #endif
  return false;
}

bool FindMethod(DECL_EXTERNAL_CODECS_LOC_VARS
   CMethodId methodId, UString &name)
{
  UInt32 i;
  for (i = 0; i < g_NumCodecs; i++)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    if (methodId == codec.Id)
    {
      name = codec.Name;
      return true;
    }
  }
  #ifdef EXTERNAL_CODECS
  if (__externalCodecs)
    for (i = 0; i < (UInt32)__externalCodecs->Codecs.Size(); i++)
    {
      const CCodecInfoEx &codec = __externalCodecs->Codecs[i];
      if (methodId == codec.Id)
      {
        name = codec.Name;
        return true;
      }
    }
  #endif
  return false;
}

bool FindHashMethod(DECL_EXTERNAL_CODECS_LOC_VARS
  const UString &name,
  CMethodId &methodId)
{
  UInt32 i;
  for (i = 0; i < g_NumHashers; i++)
  {
    const CHasherInfo &codec = *g_Hashers[i];
    if (name.IsEqualToNoCase(codec.Name))
    {
      methodId = codec.Id;
      return true;
    }
  }
  #ifdef EXTERNAL_CODECS
  if (__externalCodecs)
    for (i = 0; i < (UInt32)__externalCodecs->Hashers.Size(); i++)
    {
      const CHasherInfoEx &codec = __externalCodecs->Hashers[i];
      if (codec.Name.IsEqualToNoCase(name))
      {
        methodId = codec.Id;
        return true;
      }
    }
  #endif
  return false;
}

void GetHashMethods(DECL_EXTERNAL_CODECS_LOC_VARS
    CRecordVector<CMethodId> &methods)
{
  methods.ClearAndSetSize(g_NumHashers);
  UInt32 i;
  for (i = 0; i < g_NumHashers; i++)
    methods[i] = (*g_Hashers[i]).Id;
  #ifdef EXTERNAL_CODECS
  if (__externalCodecs)
    for (i = 0; i < (UInt32)__externalCodecs->Hashers.Size(); i++)
      methods.Add(__externalCodecs->Hashers[i].Id);
  #endif
}

HRESULT CreateCoder(
  DECL_EXTERNAL_CODECS_LOC_VARS
  CMethodId methodId,
  CMyComPtr<ICompressFilter> &filter,
  CMyComPtr<ICompressCoder> &coder,
  CMyComPtr<ICompressCoder2> &coder2,
  bool encode, bool onlyCoder)
{
  UInt32 i;
  for (i = 0; i < g_NumCodecs; i++)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    if (codec.Id == methodId)
    {
      if (encode)
      {
        if (codec.CreateEncoder)
        {
          void *p = codec.CreateEncoder();
          if (codec.IsFilter) filter = (ICompressFilter *)p;
          else if (codec.NumInStreams == 1) coder = (ICompressCoder *)p;
          else coder2 = (ICompressCoder2 *)p;
          break;
        }
      }
      else
        if (codec.CreateDecoder)
        {
          void *p = codec.CreateDecoder();
          if (codec.IsFilter) filter = (ICompressFilter *)p;
          else if (codec.NumInStreams == 1) coder = (ICompressCoder *)p;
          else coder2 = (ICompressCoder2 *)p;
          break;
        }
    }
  }

  #ifdef EXTERNAL_CODECS
  if (!filter && !coder && !coder2 && __externalCodecs)
    for (i = 0; i < (UInt32)__externalCodecs->Codecs.Size(); i++)
    {
      const CCodecInfoEx &codec = __externalCodecs->Codecs[i];
      if (codec.Id == methodId)
      {
        if (encode)
        {
          if (codec.EncoderIsAssigned)
          {
            if (codec.IsSimpleCodec())
            {
              HRESULT result = __externalCodecs->GetCodecs->CreateEncoder(i, &IID_ICompressCoder, (void **)&coder);
              if (result != S_OK && result != E_NOINTERFACE && result != CLASS_E_CLASSNOTAVAILABLE)
                return result;
              if (!coder)
              {
                RINOK(__externalCodecs->GetCodecs->CreateEncoder(i, &IID_ICompressFilter, (void **)&filter));
              }
            }
            else
            {
              RINOK(__externalCodecs->GetCodecs->CreateEncoder(i, &IID_ICompressCoder2, (void **)&coder2));
            }
            break;
          }
        }
        else
          if (codec.DecoderIsAssigned)
          {
            if (codec.IsSimpleCodec())
            {
              HRESULT result = __externalCodecs->GetCodecs->CreateDecoder(i, &IID_ICompressCoder, (void **)&coder);
              if (result != S_OK && result != E_NOINTERFACE && result != CLASS_E_CLASSNOTAVAILABLE)
                return result;
              if (!coder)
              {
                RINOK(__externalCodecs->GetCodecs->CreateDecoder(i, &IID_ICompressFilter, (void **)&filter));
              }
            }
            else
            {
              RINOK(__externalCodecs->GetCodecs->CreateDecoder(i, &IID_ICompressCoder2, (void **)&coder2));
            }
            break;
          }
      }
    }
  #endif

  if (onlyCoder && filter)
  {
    CFilterCoder *coderSpec = new CFilterCoder;
    coder = coderSpec;
    coderSpec->Filter = filter;
  }
  return S_OK;
}

HRESULT CreateCoder(
  DECL_EXTERNAL_CODECS_LOC_VARS
  CMethodId methodId,
  CMyComPtr<ICompressCoder> &coder,
  CMyComPtr<ICompressCoder2> &coder2,
  bool encode)
{
  CMyComPtr<ICompressFilter> filter;
  return CreateCoder(
    EXTERNAL_CODECS_LOC_VARS
    methodId,
    filter, coder, coder2, encode, true);
}

HRESULT CreateCoder(
  DECL_EXTERNAL_CODECS_LOC_VARS
  CMethodId methodId,
  CMyComPtr<ICompressCoder> &coder, bool encode)
{
  CMyComPtr<ICompressFilter> filter;
  CMyComPtr<ICompressCoder2> coder2;
  return CreateCoder(
    EXTERNAL_CODECS_LOC_VARS
    methodId,
    coder, coder2, encode);
}

HRESULT CreateFilter(
  DECL_EXTERNAL_CODECS_LOC_VARS
  CMethodId methodId,
  CMyComPtr<ICompressFilter> &filter,
  bool encode)
{
  CMyComPtr<ICompressCoder> coder;
  CMyComPtr<ICompressCoder2> coder2;
  return CreateCoder(
    EXTERNAL_CODECS_LOC_VARS
    methodId,
    filter, coder, coder2, encode, false);
}

HRESULT CreateHasher(
  DECL_EXTERNAL_CODECS_LOC_VARS
  CMethodId methodId,
  UString &name,
  CMyComPtr<IHasher> &hasher)
{
  UInt32 i;
  for (i = 0; i < g_NumHashers; i++)
  {
    const CHasherInfo &codec = *g_Hashers[i];
    if (codec.Id == methodId)
    {
      hasher = (IHasher *)codec.CreateHasher();
      name = codec.Name;
      break;
    }
  }

  #ifdef EXTERNAL_CODECS
  if (!hasher && __externalCodecs)
    for (i = 0; i < (UInt32)__externalCodecs->Hashers.Size(); i++)
    {
      const CHasherInfoEx &codec = __externalCodecs->Hashers[i];
      if (codec.Id == methodId)
      {
        name = codec.Name;
        return __externalCodecs->GetHashers->CreateHasher(i, &hasher);
      }
    }
  #endif

  return S_OK;
}
