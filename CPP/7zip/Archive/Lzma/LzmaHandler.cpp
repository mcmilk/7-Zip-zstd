// LzmaHandler.cpp

#include "StdAfx.h"

#include "LzmaHandler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/PropVariant.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamUtils.h"
#include "../Common/DummyOutStream.h"

#include "LzmaFiltersDecode.h"

namespace NArchive {
namespace NLzma {

STATPROPSTG kProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMethod, VT_UI1}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

static void ConvertUInt32ToString(UInt32 value, wchar_t *s)
{
  ConvertUInt64ToString(value, s + MyStringLen(s));
}

static void DictSizeToString(UInt32 value, wchar_t *s)
{
  for (int i = 0; i <= 31; i++)
    if ((UInt32(1) << i) == value)
    {
      ConvertUInt32ToString(i, s);
      return;
    }
  wchar_t c = L'b';
  if ((value & ((1 << 20) - 1)) == 0)
  {
    value >>= 20;
    c = L'm';
  }
  else if ((value & ((1 << 10) - 1)) == 0)
  {
    value >>= 10;
    c = L'k';
  }
  ConvertUInt32ToString(value, s);
  int p = MyStringLen(s);
  s[p++] = c;
  s[p++] = L'\0';
}

static void MyStrCat(wchar_t *d, const wchar_t *s)
{
  MyStringCopy(d + MyStringLen(d), s);
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  if (index != 0)
    return E_INVALIDARG;
  NWindows::NCOM::CPropVariant propVariant;
  switch(propID)
  {
    case kpidSize:
      if (m_StreamInfo.HasUnpackSize())
        propVariant = (UInt64)m_StreamInfo.UnpackSize;
      break;
    case kpidPackSize:
      propVariant = (UInt64)m_PackSize;
      break;
    case kpidMethod:
    {
      wchar_t s[64];
      s[0] = '\0';
      if (m_StreamInfo.IsThereFilter)
      {
        const wchar_t *f;
        if (m_StreamInfo.FilterMethod == 0)
          f = L"Copy";
        else if (m_StreamInfo.FilterMethod == 1)
          f = L"BCJ";
        else
          f = L"Unknown";
        MyStrCat(s, f);
        MyStrCat(s, L" ");
      }
      MyStrCat(s, L"LZMA:");
      DictSizeToString(m_StreamInfo.GetDicSize(), s);
      propVariant = s;
      break;
    }
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  {
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));

    HRESULT res = ReadStreamHeader(inStream, m_StreamInfo);
    if (res != S_OK)
      return S_FALSE;
    
    Byte b;
    RINOK(ReadStream_FALSE(inStream, &b, 1));
    if (b != 0)
      return S_FALSE;

    UInt64 endPos;
    RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPos));
    m_PackSize = endPos - m_StreamStartPosition - m_StreamInfo.GetHeaderSize();

    m_Stream = inStream;
  }
  return S_OK;
}

STDMETHODIMP CHandler::Close()
{
  m_Stream.Release();
  return S_OK;
}


STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UInt32(-1));
  if (!allFilesMode)
  {
    if (numItems == 0)
      return S_OK;
    if (numItems != 1)
      return E_INVALIDARG;
    if (indices[0] != 0)
      return E_INVALIDARG;
  }

  bool testMode = (_aTestMode != 0);

  RINOK(extractCallback->SetTotal(m_PackSize));
    
  UInt64 currentTotalPacked = 0;

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);

  {
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    
    RINOK(extractCallback->GetStream(0, &realOutStream, askMode));

    outStreamSpec->SetStream(realOutStream);
    outStreamSpec->Init();
    if(!testMode && !realOutStream)
      return S_OK;
    extractCallback->PrepareOperation(askMode);
  }
  
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, true);

  CDecoder decoder;
  RINOK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  UInt64 streamPos = m_StreamStartPosition;
  Int32 opRes = NArchive::NExtract::NOperationResult::kOK;
  bool firstItem = true;
  for (;;)
  {
    CHeader st;
    HRESULT result = ReadStreamHeader(m_Stream, st);
    if (result != S_OK)
    {
      if (firstItem)
        return E_FAIL;
      break;
    }
    firstItem = false;

    lps->OutSize = outStreamSpec->GetSize();
    lps->InSize = currentTotalPacked;
    RINOK(lps->SetCur());
    
    streamPos += st.GetHeaderSize();
    UInt64 packProcessed;

    {
      result = decoder.Code(
          EXTERNAL_CODECS_VARS
          st, m_Stream, outStream, &packProcessed, progress);
      if (result == E_NOTIMPL)
      {
        opRes = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
        break;
      }
      if (result == S_FALSE)
      {
        opRes = NArchive::NExtract::NOperationResult::kDataError;
        break;
      }
      RINOK(result);
    }

    if (packProcessed == (UInt64)(Int64)-1)
      break;
    RINOK(m_Stream->Seek(streamPos + packProcessed, STREAM_SEEK_SET, NULL));
    currentTotalPacked += packProcessed;
    streamPos += packProcessed;
  }
  outStream.Release();
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

IMPL_ISetCompressCodecsInfo

}}
