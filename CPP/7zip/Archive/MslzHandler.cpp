// MslzHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/MyString.h"

#include "Windows/PropVariant.h"

#include "../Common/InBuffer.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "Common/DummyOutStream.h"

namespace NArchive {
namespace NMslz {

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  UInt32 _size;
  UInt64 _packSize;
  UString _name;
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPath: if (!_name.IsEmpty()) prop = _name; break;
    case kpidSize: prop = _size; break;
    case kpidPackSize: prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

static const unsigned kSignatureSize = 9;
static const unsigned kHeaderSize = kSignatureSize + 1 + 4;
#define MSLZ_SIGNATURE { 0x53, 0x5A, 0x44, 0x44, 0x88, 0xF0, 0x27, 0x33, 0x41 }
// old signature: 53 5A 20 88 F0 27 33
static const Byte signature[kSignatureSize] = MSLZ_SIGNATURE;

static const wchar_t *g_Exts[] =
{
  L"dll",
  L"exe",
  L"kmd",
  L"sys"
};

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  {
    Close();
    Byte buffer[kHeaderSize];
    RINOK(ReadStream_FALSE(stream, buffer, kHeaderSize));
    if (memcmp(buffer, signature, kSignatureSize) != 0)
      return S_FALSE;
    _size = GetUi32(buffer + 10);
    if (_size > 0xFFFFFFE0)
      return S_FALSE;
    RINOK(stream->Seek(0, STREAM_SEEK_END, &_packSize));

    if (callback)
    {
      CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
      callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
      if (openVolumeCallback)
      {
        NWindows::NCOM::CPropVariant prop;
        if (openVolumeCallback->GetProperty(kpidName, &prop) == S_OK && prop.vt == VT_BSTR)
        {
          UString baseName = prop.bstrVal;
          if (!baseName.IsEmpty() && baseName.Back() == L'_')
          {
            baseName.DeleteBack();
            Byte replaceByte = buffer[kSignatureSize];
            if (replaceByte == 0)
            {
              for (int i = 0; i < sizeof(g_Exts) / sizeof(g_Exts[0]); i++)
              {
                UString s = g_Exts[i];
                int len = s.Length();
                Byte b = (Byte)s.Back();
                s.DeleteBack();
                if (baseName.Length() >= len &&
                    baseName[baseName.Length() - len] == '.' &&
                    s.CompareNoCase(baseName.Right(len - 1)) == 0)
                {
                  replaceByte = b;
                  break;
                }
              }
            }
            if (replaceByte >= 0x20 && replaceByte < 0x80)
              _name = baseName + (wchar_t)replaceByte;
          }
        }
      }
    }
    _stream = stream;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  _name.Empty();
  return S_OK;
}

// MslzDec is modified LZSS algorithm of Haruhiko Okumura:
//   maxLen = 18; Okumura
//   maxLen = 16; MS

#define PROGRESS_AND_WRITE \
  if ((dest & kMask) == 0) { RINOK(WriteStream(outStream, buf, kBufSize)); \
    if ((dest & ((1 << 20) - 1)) == 0) \
    { UInt64 inSize = inStream.GetProcessedSize(); UInt64 outSize = dest; \
      RINOK(progress->SetRatioInfo(&inSize, &outSize)); }}

static HRESULT MslzDec(CInBuffer &inStream, ISequentialOutStream *outStream, UInt32 unpackSize, ICompressProgressInfo *progress)
{
  const unsigned kBufSize = (1 << 12);
  const unsigned kMask = kBufSize - 1;
  Byte buf[kBufSize];
  UInt32 dest = 0;
  memset(buf, ' ', kBufSize);
  while (dest < unpackSize)
  {
    Byte b;
    if (!inStream.ReadByte(b))
      return S_FALSE;
    for (unsigned mask = (unsigned)b | 0x100; mask > 1 && dest < unpackSize; mask >>= 1)
    {
      if (!inStream.ReadByte(b))
        return S_FALSE;
      if (mask & 1)
      {
        buf[dest++ & kMask] = b;
        PROGRESS_AND_WRITE
      }
      else
      {
        Byte b1;
        if (!inStream.ReadByte(b1))
          return S_FALSE;
        const unsigned kMaxLen = 16; // 18 in Okumura's code.
        unsigned src = (((((unsigned)b1 & 0xF0) << 4) | b) + kMaxLen) & kMask;
        unsigned len = (b1 & 0xF) + 3;
        if (len > kMaxLen || dest + len > unpackSize)
          return S_FALSE;
        do
        {
          buf[dest++ & kMask] = buf[src++ & kMask];
          PROGRESS_AND_WRITE
        }
        while (--len != 0);
      }
    }
  }
  return WriteStream(outStream, buf, dest & kMask);
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  extractCallback->SetTotal(_size);

  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
  if (!testMode && !realOutStream)
    return S_OK;

  extractCallback->PrepareOperation(askMode);

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);
  
  RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
  CInBuffer s;
  if (!s.Create(1 << 20))
    return E_OUTOFMEMORY;
  s.SetStream(_stream);
  s.Init();
  Byte buffer[kHeaderSize];
  Int32 opRes = NExtract::NOperationResult::kDataError;
  if (s.ReadBytes(buffer, kHeaderSize) == kHeaderSize)
  {
    HRESULT result = MslzDec(s, outStream, _size, progress);
    if (result == S_OK)
      opRes = NExtract::NOperationResult::kOK;
    else if (result != S_FALSE)
      return result;
  }
  outStream.Release();
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"MsLZ", L"", 0, 0xD5, MSLZ_SIGNATURE, kSignatureSize, false, CreateArc, 0 };

REGISTER_ARC(Mslz)

}}
