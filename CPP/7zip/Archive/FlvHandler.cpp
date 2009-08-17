// FlvHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"
// #include "Common/Defs.h"
#include "Common/MyString.h"

#include "Windows/PropVariant.h"

#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#define GetBe24(p) ( \
    ((UInt32)((const Byte *)(p))[0] << 16) | \
    ((UInt32)((const Byte *)(p))[1] <<  8) | \
             ((const Byte *)(p))[2] )

#define Get16(p) GetBe16(p)
#define Get24(p) GetBe24(p)
#define Get32(p) GetBe32(p)

namespace NArchive {
namespace NFlv {

static const UInt32 kFileSizeMax = (UInt32)1 << 30;
static const int kNumChunksMax = (UInt32)1 << 23;

const UInt32 kTagHeaderSize = 11;

static const Byte kFlag_Video = 1;
static const Byte kFlag_Audio = 4;

static const Byte kType_Audio = 8;
static const Byte kType_Video = 9;
static const Byte kType_Meta = 18;
static const int kNumTypes = 19;

struct CItem
{
  UInt32 Offset;
  UInt32 Size;
  // UInt32 Time;
  Byte Type;
};

struct CItem2
{
  Byte Type;
  Byte SubType;
  Byte Props;
  bool SameSubTypes;
  int NumChunks;
  size_t Size;

  CReferenceBuf *BufSpec;
  CMyComPtr<IUnknown> RefBuf;

  bool IsAudio() const { return Type == kType_Audio; }
};

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  int _isRaw;
  CMyComPtr<IInStream> _stream;
  CObjectVector<CItem2> _items2;
  // CByteBuffer _metadata;
  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *callback);
  AString GetComment();
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

STATPROPSTG kProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidNumBlocks, VT_UI4},
  { NULL, kpidComment, VT_BSTR}
};

/*
STATPROPSTG kArcProps[] =
{
  { NULL, kpidComment, VT_BSTR}
};
*/

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

static const char *g_AudioTypes[16] =
{
  "pcm",
  "adpcm",
  "mp3",
  "pcm_le",
  "nellymoser16",
  "nellymoser8",
  "nellymoser",
  "g711a",
  "g711m",
  "audio9",
  "aac",
  "speex",
  "audio12",
  "audio13",
  "mp3",
  "audio15"
};

static const char *g_VideoTypes[16] =
{
  "video0",
  "jpeg",
  "h263",
  "screen",
  "vp6",
  "vp6alpha",
  "screen2",
  "avc",
  "video8",
  "video9",
  "video10",
  "video11",
  "video12",
  "video13",
  "video14",
  "video15"
};

static const char *g_Rates[4] =
{
  "5.5 kHz",
  "11 kHz",
  "22 kHz",
  "44 kHz"
};

static void MyStrCat(char *d, const char *s)
{
  MyStringCopy(d + MyStringLen(d), s);
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  const CItem2 &item = _items2[index];
  switch(propID)
  {
    case kpidExtension:
      prop = _isRaw ?
        (item.IsAudio() ? g_AudioTypes[item.SubType] : g_VideoTypes[item.SubType]) :
        (item.IsAudio() ? "audio.flv" : "video.flv");
      break;
    case kpidSize:
    case kpidPackSize:
      prop = (UInt64)item.Size;
      break;
    case kpidNumBlocks: prop = (UInt32)item.NumChunks; break;
    case kpidComment:
    {
      char sz[64];
      MyStringCopy(sz, (item.IsAudio() ? g_AudioTypes[item.SubType] : g_VideoTypes[item.SubType]) );
      if (item.IsAudio())
      {
        MyStrCat(sz, " ");
        MyStrCat(sz, g_Rates[(item.Props >> 2) & 3]);
        MyStrCat(sz, (item.Props & 2) ? " 16-bit" : " 8-bit");
        MyStrCat(sz, (item.Props & 1) ? " stereo" : " mono");
      }
      prop = sz;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

/*
AString CHandler::GetComment()
{
  const Byte *p = _metadata;
  size_t size = _metadata.GetCapacity();
  AString res;
  if (size > 0)
  {
    p++;
    size--;
    for (;;)
    {
      if (size < 2)
        break;
      int len = Get16(p);
      p += 2;
      size -= 2;
      if (len == 0 || (size_t)len > size)
        break;
      {
        AString temp;
        char *sz = temp.GetBuffer(len);
        memcpy(sz, p, len);
        sz[len] = 0;
        temp.ReleaseBuffer();
        if (!res.IsEmpty())
          res += '\n';
        res += temp;
      }
      p += len;
      size -= len;
      if (size < 1)
        break;
      Byte type = *p++;
      size--;
      bool ok = false;
      switch(type)
      {
        case 0:
        {
          if (size < 8)
            break;
          ok = true;
          Byte reverse[8];
          for (int i = 0; i < 8; i++)
          {
            bool little_endian = 1;
            if (little_endian)
              reverse[i] = p[7 - i];
            else
              reverse[i] = p[i];
          }
          double d = *(double *)reverse;
          char temp[32];
          sprintf(temp, " = %.3f", d);
          res += temp;
          p += 8;
          size -= 8;
          break;
        }
        case 8:
        {
          if (size < 4)
            break;
          ok = true;
          // UInt32 numItems = Get32(p);
          p += 4;
          size -= 4;
          break;
        }
      }
      if (!ok)
        break;
    }
  }
  return res;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidComment: prop = GetComment(); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}
*/

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  CRecordVector<CItem> items;

  const UInt32 kHeaderSize = 13;
  Byte header[kHeaderSize];
  RINOK(ReadStream_FALSE(stream, header, kHeaderSize));
  if (header[0] != 'F' ||
      header[1] != 'L' ||
      header[2] != 'V' ||
      header[3] != 1 ||
      (header[4] & 0xFA) != 0)
    return S_FALSE;
  UInt32 offset = Get32(header + 5);
  if (offset != 9 || Get32(header + 9) != 0)
    return S_FALSE;
  offset += 4;
 
  CByteBuffer inBuf;
  size_t fileSize;
  {
    UInt64 fileSize64;
    RINOK(stream->Seek(0, STREAM_SEEK_END, &fileSize64));
    if (fileSize64 > kFileSizeMax)
      return S_FALSE;

    if (callback)
      RINOK(callback->SetTotal(NULL, &fileSize64))

    RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
    fileSize = (size_t)fileSize64;
    inBuf.SetCapacity(fileSize);
    for (size_t pos = 0; pos < fileSize;)
    {
      UInt64 offset64 = pos;
      if (callback)
        RINOK(callback->SetCompleted(NULL, &offset64))
      size_t rem = MyMin(fileSize - pos, (size_t)(1 << 20));
      RINOK(ReadStream_FALSE(stream, inBuf + pos, rem));
      pos += rem;
    }
  }

  int lasts[kNumTypes];
  int i;
  for (i = 0; i < kNumTypes; i++)
    lasts[i] = -1;

  while (offset < fileSize)
  {
    CItem item;
    item.Offset = offset;
    const Byte *buf = inBuf + offset;
    offset += kTagHeaderSize;
    if (offset > fileSize)
      return S_FALSE;

    item.Type = buf[0];
    UInt32 size = Get24(buf + 1);
    if (size < 1)
      return S_FALSE;
    // item.Time = Get24(buf + 4);
    // item.Time |= (UInt32)buf[7] << 24;
    if (Get24(buf + 8) != 0) // streamID
      return S_FALSE;

    UInt32 curSize = kTagHeaderSize + size + 4;
    item.Size = curSize;
    
    offset += curSize - kTagHeaderSize;
    if (offset > fileSize)
      return S_FALSE;
    
    if (Get32(buf + kTagHeaderSize + size) != kTagHeaderSize + size)
      return S_FALSE;

    // printf("\noffset = %6X type = %2d time = %6d size = %6d", (UInt32)offset, item.Type, item.Time, item.Size);

    if (item.Type == kType_Meta)
    {
      // _metadata = item.Buf;
    }
    else
    {
      if (item.Type != kType_Audio && item.Type != kType_Video)
        return S_FALSE;
      if (items.Size() >= kNumChunksMax)
        return S_FALSE;
      Byte firstByte = buf[kTagHeaderSize];
      Byte subType, props;
      if (item.Type == kType_Audio)
      {
        subType = firstByte >> 4;
        props = firstByte & 0xF;
      }
      else
      {
        subType = firstByte & 0xF;
        props = firstByte >> 4;
      }
      int last = lasts[item.Type];
      if (last < 0)
      {
        CItem2 item2;
        item2.RefBuf = item2.BufSpec = new CReferenceBuf;
        item2.Size = curSize;
        item2.Type = item.Type;
        item2.SubType = subType;
        item2.Props = props;
        item2.NumChunks = 1;
        item2.SameSubTypes = true;
        lasts[item.Type] = _items2.Add(item2);
      }
      else
      {
        CItem2 &item2 = _items2[last];
        if (subType != item2.SubType)
          item2.SameSubTypes = false;
        item2.Size += curSize;
        item2.NumChunks++;
      }
      items.Add(item);
    }
  }

  _isRaw = (_items2.Size() == 1);
  for (i = 0; i < _items2.Size(); i++)
  {
    CItem2 &item2 = _items2[i];
    CByteBuffer &itemBuf = item2.BufSpec->Buf;
    if (_isRaw)
    {
      if (!item2.SameSubTypes)
        return S_FALSE;
      itemBuf.SetCapacity((size_t)item2.Size - (kTagHeaderSize + 4 + 1) * item2.NumChunks);
      item2.Size = 0;
    }
    else
    {
      itemBuf.SetCapacity(kHeaderSize + (size_t)item2.Size);
      memcpy(itemBuf, header, kHeaderSize);
      itemBuf[4] = item2.IsAudio() ? kFlag_Audio : kFlag_Video;
      item2.Size = kHeaderSize;
    }
  }

  for (i = 0; i < items.Size(); i++)
  {
    const CItem &item = items[i];
    CItem2 &item2 = _items2[lasts[item.Type]];
    size_t size = item.Size;
    const Byte *src = inBuf + item.Offset;
    if (_isRaw)
    {
      src += kTagHeaderSize + 1;
      size -= (kTagHeaderSize + 4 + 1);
    }
    memcpy(item2.BufSpec->Buf + item2.Size, src, size);
    item2.Size += size;
  }
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  Close();
  HRESULT res;
  try
  {
    res = Open2(inStream, callback);
    if (res == S_OK)
      _stream = inStream;
  }
  catch(...) { res = S_FALSE; }
  if (res != S_OK)
  {
    Close();
    return S_FALSE;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  _items2.Clear();
  // _metadata.SetCapacity(0);
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items2.Size();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _items2.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _items2[allFilesMode ? i : indices[i]].Size;
  extractCallback->SetTotal(totalSize);

  totalSize = 0;
  
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> outStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    const CItem2 &item = _items2[index];
    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    totalSize += item.Size;
    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    if (outStream)
    {
      RINOK(WriteStream(outStream, item.BufSpec->Buf, item.BufSpec->Buf.GetCapacity()));
    }
    RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  *stream = 0;
  CBufInStream *streamSpec = new CBufInStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  streamSpec->Init(_items2[index].BufSpec);
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"FLV", L"flv", 0, 0xD6, { 'F', 'L', 'V' }, 3, false, CreateArc, 0 };

REGISTER_ARC(Flv)

}}
