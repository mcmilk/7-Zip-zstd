// LzmaEncoder.cpp

#include "StdAfx.h"

extern "C"
{
#include "../../../C/Alloc.h"
}

#include "../Common/StreamUtils.h"

#include "LzmaEncoder.h"

static HRESULT SResToHRESULT(SRes res)
{
  switch(res)
  {
    case SZ_OK: return S_OK;
    case SZ_ERROR_MEM: return E_OUTOFMEMORY;
    case SZ_ERROR_PARAM: return E_INVALIDARG;
    // case SZ_ERROR_THREAD: return E_FAIL;
  }
  return E_FAIL;
}

namespace NCompress {
namespace NLzma {

static const UInt32 kStreamStepSize = (UInt32)1 << 31;

static SRes MyRead(void *object, void *data, size_t *size)
{
  UInt32 curSize = ((*size < kStreamStepSize) ? (UInt32)*size : kStreamStepSize);
  HRESULT res = ((CSeqInStream *)object)->RealStream->Read(data, curSize, &curSize);
  *size = curSize;
  return (SRes)res;
}

static size_t MyWrite(void *object, const void *data, size_t size)
{
  CSeqOutStream *p = (CSeqOutStream *)object;
  p->Res = WriteStream(p->RealStream, data, size);
  if (p->Res != 0)
    return 0;
  return size;
}

static void *SzBigAlloc(void *, size_t size) { return BigAlloc(size); }
static void SzBigFree(void *, void *address) { BigFree(address); }
static ISzAlloc g_BigAlloc = { SzBigAlloc, SzBigFree };

static void *SzAlloc(void *, size_t size) { return MyAlloc(size); }
static void SzFree(void *, void *address) { MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

CEncoder::CEncoder()
{
  _seqInStream.SeqInStream.Read = MyRead;
  _seqOutStream.SeqOutStream.Write = MyWrite;
  _encoder = 0;
  _encoder = LzmaEnc_Create(&g_Alloc);
  if (_encoder == 0)
    throw 1;
}

CEncoder::~CEncoder()
{
  if (_encoder != 0)
    LzmaEnc_Destroy(_encoder, &g_Alloc, &g_BigAlloc);
}

inline wchar_t GetUpperChar(wchar_t c)
{
  if (c >= 'a' && c <= 'z')
    c -= 0x20;
  return c;
}

static int ParseMatchFinder(const wchar_t *s, int *btMode, int *numHashBytes)
{
  wchar_t c = GetUpperChar(*s++);
  if (c == L'H')
  {
    if (GetUpperChar(*s++) != L'C')
      return 0;
    int numHashBytesLoc = (int)(*s++ - L'0');
    if (numHashBytesLoc < 4 || numHashBytesLoc > 4)
      return 0;
    if (*s++ != 0)
      return 0;
    *btMode = 0;
    *numHashBytes = numHashBytesLoc;
    return 1;
  }
  if (c != L'B')
    return 0;

  if (GetUpperChar(*s++) != L'T')
    return 0;
  int numHashBytesLoc = (int)(*s++ - L'0');
  if (numHashBytesLoc < 2 || numHashBytesLoc > 4)
    return 0;
  c = GetUpperChar(*s++);
  if (c != L'\0')
    return 0;
  *btMode = 1;
  *numHashBytes = numHashBytesLoc;
  return 1;
}

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps)
{
  CLzmaEncProps props;
  LzmaEncProps_Init(&props);

  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    switch (propIDs[i])
    {
      case NCoderPropID::kNumFastBytes:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.fb = prop.ulVal; break;
      case NCoderPropID::kMatchFinderCycles:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.mc = prop.ulVal; break;
      case NCoderPropID::kAlgorithm:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.algo = prop.ulVal; break;
      case NCoderPropID::kDictionarySize:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.dictSize = prop.ulVal; break;
      case NCoderPropID::kPosStateBits:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.pb = prop.ulVal; break;
      case NCoderPropID::kLitPosBits:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.lp = prop.ulVal; break;
      case NCoderPropID::kLitContextBits:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.lc = prop.ulVal; break;
      case NCoderPropID::kNumThreads:
        if (prop.vt != VT_UI4) return E_INVALIDARG; props.numThreads = prop.ulVal; break;
      case NCoderPropID::kMultiThread:
        if (prop.vt != VT_BOOL) return E_INVALIDARG; props.numThreads = ((prop.boolVal == VARIANT_TRUE) ? 2 : 1); break;
      case NCoderPropID::kEndMarker:
        if (prop.vt != VT_BOOL) return E_INVALIDARG; props.writeEndMark = (prop.boolVal == VARIANT_TRUE); break;
      case NCoderPropID::kMatchFinder:
        if (prop.vt != VT_BSTR) return E_INVALIDARG;
        if (!ParseMatchFinder(prop.bstrVal, &props.btMode, &props.numHashBytes /* , &_matchFinderBase.skipModeBits */))
          return E_INVALIDARG; break;
      default:
        return E_INVALIDARG;
    }
  }
  return SResToHRESULT(LzmaEnc_SetProps(_encoder, &props));
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{
  Byte props[LZMA_PROPS_SIZE];
  size_t size = LZMA_PROPS_SIZE;
  RINOK(LzmaEnc_WriteProperties(_encoder, props, &size));
  return WriteStream(outStream, props, size);
}

STDMETHODIMP CEncoder::SetOutStream(ISequentialOutStream *outStream)
{
  _seqOutStream.RealStream = outStream;
  _seqOutStream.Res = S_OK;
  return S_OK;
}

STDMETHODIMP CEncoder::ReleaseOutStream()
{
  _seqOutStream.RealStream.Release();
  return S_OK;
}

typedef struct _CCompressProgressImp
{
  ICompressProgress p;
  ICompressProgressInfo *Progress;
  HRESULT Res;
} CCompressProgressImp;

#define PROGRESS_UNKNOWN_VALUE ((UInt64)(Int64)-1)

#define CONVERT_PR_VAL(x) (x == PROGRESS_UNKNOWN_VALUE ? NULL : &x)

SRes CompressProgress(void *pp, UInt64 inSize, UInt64 outSize)
{
  CCompressProgressImp *p = (CCompressProgressImp *)pp;
  p->Res = p->Progress->SetRatioInfo(CONVERT_PR_VAL(inSize), CONVERT_PR_VAL(outSize));
  return (SRes)p->Res;
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
{
  CCompressProgressImp progressImp;
  progressImp.p.Progress = CompressProgress;
  progressImp.Progress = progress;
  progressImp.Res = SZ_OK;

  _seqInStream.RealStream = inStream;
  SetOutStream(outStream);
  SRes res = LzmaEnc_Encode(_encoder, &_seqOutStream.SeqOutStream, &_seqInStream.SeqInStream, progress ? &progressImp.p : NULL, &g_Alloc, &g_BigAlloc);
  ReleaseOutStream();
  if (res == SZ_ERROR_WRITE && _seqOutStream.Res != S_OK)
    return _seqOutStream.Res;
  if (res == SZ_ERROR_PROGRESS && progressImp.Res != S_OK)
    return progressImp.Res;
  return SResToHRESULT(res);
}
  
}}
