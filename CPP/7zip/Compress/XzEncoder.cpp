// XzEncoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/CWrappers.h"
#include "../Common/StreamUtils.h"

#include "XzEncoder.h"

namespace NCompress {

namespace NLzma2 {

HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props);

}

namespace NXz {

extern "C" {

static void *SzBigAlloc(ISzAllocPtr, size_t size) { return BigAlloc(size); }
static void SzBigFree(ISzAllocPtr, void *address) { BigFree(address); }
static const ISzAlloc g_BigAlloc = { SzBigAlloc, SzBigFree };

static void *SzAlloc(ISzAllocPtr, size_t size) { return MyAlloc(size); }
static void SzFree(ISzAllocPtr, void *address) { MyFree(address); }
static const ISzAlloc g_Alloc = { SzAlloc, SzFree };

}

void CEncoder::InitCoderProps()
{
  Lzma2EncProps_Init(&_lzma2Props);
  XzProps_Init(&xzProps);
  XzFilterProps_Init(&filter);
  
  xzProps.lzma2Props = &_lzma2Props;
  // xzProps.filterProps = (_filterId != 0 ? &filter : NULL);
  xzProps.filterProps = NULL;
}

CEncoder::CEncoder()
{
  InitCoderProps();
}

CEncoder::~CEncoder()
{
}


HRESULT CEncoder::SetCoderProp(PROPID propID, const PROPVARIANT &prop)
{
  return NLzma2::SetLzma2Prop(propID, prop, _lzma2Props);
}

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps)
{
  Lzma2EncProps_Init(&_lzma2Props);

  for (UInt32 i = 0; i < numProps; i++)
  {
    RINOK(SetCoderProp(propIDs[i], coderProps[i]));
  }
  return S_OK;
  // return SResToHRESULT(Lzma2Enc_SetProps(_encoder, &lzma2Props));
}



STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 * /* inSize */, const UInt64 * /* outSize */,
    ICompressProgressInfo *progress)
{
  CSeqOutStreamWrap seqOutStream;
  
  seqOutStream.Init(outStream);
  
  // if (IntToBool(newData))
  {
    /*
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
      RINOK(updateCallback->SetTotal(size));
    }
    */

    /*
    CLzma2EncProps lzma2Props;
    Lzma2EncProps_Init(&lzma2Props);

    lzma2Props.lzmaProps.level = GetLevel();
    */

    CSeqInStreamWrap seqInStream;
    
    seqInStream.Init(inStream);


    /*
    {
      NCOM::CPropVariant prop = (UInt64)size;
      RINOK(NCompress::NLzma2::SetLzma2Prop(NCoderPropID::kReduceSize, prop, lzma2Props));
    }

    FOR_VECTOR (i, _methods)
    {
      COneMethodInfo &m = _methods[i];
      SetGlobalLevelAndThreads(m
      #ifndef _7ZIP_ST
      , _numThreads
      #endif
      );
      {
        FOR_VECTOR (j, m.Props)
        {
          const CProp &prop = m.Props[j];
          RINOK(NCompress::NLzma2::SetLzma2Prop(prop.Id, prop.Value, lzma2Props));
        }
      }
    }

    #ifndef _7ZIP_ST
    lzma2Props.numTotalThreads = _numThreads;
    #endif

    */

    CCompressProgressWrap progressWrap;
    
    progressWrap.Init(progress);

    xzProps.checkId = XZ_CHECK_CRC32;
    // xzProps.checkId = XZ_CHECK_CRC64;
    /*
    CXzProps xzProps;
    CXzFilterProps filter;
    XzProps_Init(&xzProps);
    XzFilterProps_Init(&filter);
    xzProps.lzma2Props = &_lzma2Props;
    */
    /*
    xzProps.filterProps = (_filterId != 0 ? &filter : NULL);
    switch (_crcSize)
    {
      case  0: xzProps.checkId = XZ_CHECK_NO; break;
      case  4: xzProps.checkId = XZ_CHECK_CRC32; break;
      case  8: xzProps.checkId = XZ_CHECK_CRC64; break;
      case 32: xzProps.checkId = XZ_CHECK_SHA256; break;
      default: return E_INVALIDARG;
    }
    filter.id = _filterId;
    if (_filterId == XZ_ID_Delta)
    {
      bool deltaDefined = false;
      FOR_VECTOR (j, _filterMethod.Props)
      {
        const CProp &prop = _filterMethod.Props[j];
        if (prop.Id == NCoderPropID::kDefaultProp && prop.Value.vt == VT_UI4)
        {
          UInt32 delta = (UInt32)prop.Value.ulVal;
          if (delta < 1 || delta > 256)
            return E_INVALIDARG;
          filter.delta = delta;
          deltaDefined = true;
        }
      }
      if (!deltaDefined)
        return E_INVALIDARG;
    }
    */
    SRes res = Xz_Encode(&seqOutStream.vt, &seqInStream.vt, &xzProps, progress ? &progressWrap.vt : NULL);
    /*
    if (res == SZ_OK)
      return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
    */
    return SResToHRESULT(res);
  }
}
  
}}
