// CramfsHandler.cpp

#include "StdAfx.h"

#include "../../../C/7zCrc.h"
#include "../../../C/CpuArch.h"
#include "../../../C/Alloc.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariantUtils.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"
#include "../Compress/ZlibDecoder.h"

namespace NArchive {
namespace NCramfs {

#define SIGNATURE { 'C','o','m','p','r','e','s','s','e','d',' ','R','O','M','F','S' }

static const UInt32 kSignatureSize = 16;
static const char kSignature[kSignatureSize] = SIGNATURE;

static const UInt32 kArcSizeMax = (256 + 16) << 20;
static const UInt32 kNumFilesMax = (1 << 19);
static const unsigned kNumDirLevelsMax = (1 << 8);

static const UInt32 kHeaderSize = 0x40;
static const unsigned kHeaderNameSize = 16;
static const UInt32 kNodeSize = 12;

static const UInt32 kFlag_FsVer2 = (1 << 0);

static const CUInt32PCharPair k_Flags[] =
{
  { 0, "Ver2" },
  { 1, "SortedDirs" },
  { 8, "Holes" },
  { 9, "WrongSignature" },
  { 10, "ShiftedRootOffset" }
};

static const unsigned kBlockSizeLog = 12;
static const UInt32 kBlockSize = 1 << kBlockSizeLog;

/*
struct CNode
{
  UInt16 Mode;
  UInt16 Uid;
  UInt32 Size;
  Byte Gid;
  UInt32 NameLen;
  UInt32 Offset;

  void Parse(const Byte *p)
  {
    Mode = GetUi16(p);
    Uid = GetUi16(p + 2);
    Size = Get32(p + 4) & 0xFFFFFF;
    Gid = p[7];
    NameLen = p[8] & 0x3F;
    Offset = Get32(p + 8) >> 6;
  }
};
*/

#define Get32(p) (be ? GetBe32(p) : GetUi32(p))

static UInt32 GetMode(const Byte *p, bool be) { return be ? GetBe16(p) : GetUi16(p); }
static bool IsDir(const Byte *p, bool be) { return (GetMode(p, be) & 0xF000) == 0x4000; }

static UInt32 GetSize(const Byte *p, bool be)
{
  if (be)
    return GetBe32(p + 4) >> 8;
  else
    return GetUi32(p + 4) & 0xFFFFFF;
}

static UInt32 GetNameLen(const Byte *p, bool be)
{
  if (be)
    return (p[8] & 0xFC);
  else
    return (p[8] & 0x3F) << 2;
}

static UInt32 GetOffset(const Byte *p, bool be)
{
  if (be)
    return (GetBe32(p + 8) & 0x03FFFFFF) << 2;
  else
    return GetUi32(p + 8) >> 6 << 2;
}

struct CItem
{
  UInt32 Offset;
  int Parent;
};

struct CHeader
{
  bool be;
  UInt32 Size;
  UInt32 Flags;
  // UInt32 Future;
  UInt32 Crc;
  // UInt32 Edition;
  UInt32 NumBlocks;
  UInt32 NumFiles;
  char Name[kHeaderNameSize];

  bool Parse(const Byte *p)
  {
    if (memcmp(p + 16, kSignature, kSignatureSize) != 0)
      return false;
    switch(GetUi32(p))
    {
      case 0x28CD3D45: be = false; break;
      case 0x453DCD28: be = true; break;
      default: return false;
    }
    Size = Get32(p + 4);
    Flags = Get32(p + 8);
    // Future = Get32(p + 0xC);
    Crc = Get32(p + 0x20);
    // Edition = Get32(p + 0x24);
    NumBlocks = Get32(p + 0x28);
    NumFiles = Get32(p + 0x2C);
    memcpy(Name, p + 0x30, kHeaderNameSize);
    return true;
  }

  bool IsVer2() const { return (Flags & kFlag_FsVer2) != 0; }
};

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CRecordVector<CItem> _items;
  CMyComPtr<IInStream> _stream;
  Byte *_data;
  UInt32 _size;
  UInt32 _headersSize;
  AString _errorMessage;
  CHeader _h;

  // Current file

  NCompress::NZlib::CDecoder *_zlibDecoderSpec;
  CMyComPtr<ICompressCoder> _zlibDecoder;
  
  CBufInStream *_inStreamSpec;
  CMyComPtr<ISequentialInStream> _inStream;

  CBufPtrSeqOutStream *_outStreamSpec;
  CMyComPtr<ISequentialOutStream> _outStream;

  UInt32 _curBlocksOffset;
  UInt32 _curNumBlocks;

  HRESULT OpenDir(int parent, UInt32 baseOffsetBase, unsigned level);
  HRESULT Open2(IInStream *inStream);
  AString GetPath(int index) const;
  bool GetPackSize(int index, UInt32 &res) const;
  void Free();
public:
  CHandler(): _data(0) {}
  ~CHandler() { Free(); }
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize);
};

static const STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI4},
  { NULL, kpidPackSize, VT_UI4},
  { NULL, kpidPosixAttrib, VT_UI4}
  // { NULL, kpidOffset, VT_UI4}
};

static const STATPROPSTG kArcProps[] =
{
  { NULL, kpidName, VT_BSTR},
  { NULL, kpidBigEndian, VT_BOOL},
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidPhySize, VT_UI4},
  { NULL, kpidHeadersSize, VT_UI4},
  { NULL, kpidNumSubFiles, VT_UI4},
  { NULL, kpidNumBlocks, VT_UI4}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

HRESULT CHandler::OpenDir(int parent, UInt32 baseOffset, unsigned level)
{
  const Byte *p = _data + baseOffset;
  bool be = _h.be;
  if (!IsDir(p, be))
    return S_OK;
  UInt32 offset = GetOffset(p, be);
  UInt32 size = GetSize(p, be);
  if (offset == 0 && size == 0)
    return S_OK;
  UInt32 end = offset + size;
  if (offset < kHeaderSize || end > _size || level > kNumDirLevelsMax)
    return S_FALSE;
  if (end > _headersSize)
    _headersSize = end;

  int startIndex = _items.Size();
  
  while (size != 0)
  {
    if (size < kNodeSize || (UInt32)_items.Size() >= kNumFilesMax)
      return S_FALSE;
    CItem item;
    item.Parent = parent;
    item.Offset = offset;
    _items.Add(item);
    UInt32 nodeLen = kNodeSize + GetNameLen(_data + offset, be);
    if (size < nodeLen)
      return S_FALSE;
    offset += nodeLen;
    size -= nodeLen;
  }

  int endIndex = _items.Size();
  for (int i = startIndex; i < endIndex; i++)
  {
    RINOK(OpenDir(i, _items[i].Offset, level + 1));
  }
  return S_OK;
}

HRESULT CHandler::Open2(IInStream *inStream)
{
  Byte buf[kHeaderSize];
  RINOK(ReadStream_FALSE(inStream, buf, kHeaderSize));
  if (!_h.Parse(buf))
    return S_FALSE;
  if (_h.IsVer2())
  {
    if (_h.Size < kHeaderSize || _h.Size > kArcSizeMax || _h.NumFiles > kNumFilesMax)
      return S_FALSE;
  }
  else
  {
    UInt64 size;
    RINOK(inStream->Seek(0, STREAM_SEEK_END, &size));
    if (size > kArcSizeMax)
      return S_FALSE;
    _h.Size = (UInt32)size;
    RINOK(inStream->Seek(kHeaderSize, STREAM_SEEK_SET, NULL));
  }
  _data = (Byte *)MidAlloc(_h.Size);
  if (_data == 0)
    return E_OUTOFMEMORY;
  memcpy(_data, buf, kHeaderSize);
  size_t processed = _h.Size - kHeaderSize;
  RINOK(ReadStream(inStream, _data + kHeaderSize, &processed));
  if (processed < kNodeSize)
    return S_FALSE;
  _size = kHeaderSize + (UInt32)processed;
  if (_size != _h.Size)
    _errorMessage = "Unexpected end of archive";
  else
  {
    SetUi32(_data + 0x20, 0);
    if (_h.IsVer2())
      if (CrcCalc(_data, _h.Size) != _h.Crc)
        _errorMessage = "CRC error";
  }
  if (_h.IsVer2())
    _items.Reserve(_h.NumFiles - 1);
  return OpenDir(-1, kHeaderSize, 0);
}

AString CHandler::GetPath(int index) const
{
  unsigned len = 0;
  int indexMem = index;
  do
  {
    const CItem &item = _items[index];
    index = item.Parent;
    const Byte *p = _data + item.Offset;
    unsigned size = GetNameLen(p, _h.be);
    p += kNodeSize;
    unsigned i;
    for (i = 0; i < size && p[i]; i++);
    len += i + 1;
  }
  while (index >= 0);
  len--;

  AString path;
  char *dest = path.GetBuffer(len) + len;
  index = indexMem;
  for (;;)
  {
    const CItem &item = _items[index];
    index = item.Parent;
    const Byte *p = _data + item.Offset;
    unsigned size = GetNameLen(p, _h.be);
    p += kNodeSize;
    unsigned i;
    for (i = 0; i < size && p[i]; i++);
    dest -= i;
    memcpy(dest, p, i);
    if (index < 0)
      break;
    *(--dest) = CHAR_PATH_SEPARATOR;
  }
  path.ReleaseBuffer(len);
  return path;
}

bool CHandler::GetPackSize(int index, UInt32 &res) const
{
  const CItem &item = _items[index];
  const Byte *p = _data + item.Offset;
  bool be = _h.be;
  UInt32 offset = GetOffset(p, be);
  if (offset < kHeaderSize)
    return false;
  UInt32 numBlocks = (GetSize(p, be) + kBlockSize - 1) >> kBlockSizeLog;
  UInt32 start = offset + numBlocks * 4;
  if (start > _size)
    return false;
  UInt32 end = Get32(_data + start - 4);
  if (end < start)
    return false;
  res = end - start;
  return true;
}

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback * /* callback */)
{
  COM_TRY_BEGIN
  {
    Close();
    RINOK(Open2(stream));
    _stream = stream;
  }
  return S_OK;
  COM_TRY_END
}

void CHandler::Free()
{
  MidFree(_data);
  _data = 0;
}

STDMETHODIMP CHandler::Close()
{
  _headersSize = 0;
  _items.Clear();
  _stream.Release();
  _errorMessage.Empty();
  Free();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidName:
    {
      char dest[kHeaderNameSize + 4];
      memcpy(dest, _h.Name, kHeaderNameSize);
      dest[kHeaderNameSize] = 0;
      prop = dest;
      break;
    }
    case kpidBigEndian: prop = _h.be; break;
    case kpidCharacts: FLAGS_TO_PROP(k_Flags, _h.Flags, prop); break;
    case kpidNumBlocks: if (_h.IsVer2()) prop = _h.NumBlocks; break;
    case kpidNumSubFiles: if (_h.IsVer2()) prop = _h.NumFiles; break;
    case kpidPhySize: if (_h.IsVer2()) prop = _h.Size; break;
    case kpidHeadersSize: prop = _headersSize; break;
    case kpidError: if (!_errorMessage.IsEmpty()) prop = _errorMessage; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = _items[index];
  const Byte *p = _data + item.Offset;
  bool be = _h.be;
  bool isDir = IsDir(p, be);
  switch(propID)
  {
    case kpidPath: prop = MultiByteToUnicodeString(GetPath(index), CP_OEMCP); break;
    case kpidIsDir: prop = isDir; break;
    // case kpidOffset: prop = (UInt32)GetOffset(p, be); break;
    case kpidSize: if (!isDir) prop = GetSize(p, be); break;
    case kpidPackSize:
      if (!isDir)
      {
        UInt32 size;
        if (GetPackSize(index, size))
          prop = size;
      }
      break;
    case kpidPosixAttrib: prop = (UInt32)GetMode(p, be); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CCramfsInStream: public CCachedInStream
{
  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize);
public:
  CHandler *Handler;
};

HRESULT CCramfsInStream::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize)
{
  return Handler->ReadBlock(blockIndex, dest, blockSize);
}

HRESULT CHandler::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize)
{
  if (!_zlibDecoder)
  {
    _zlibDecoderSpec = new NCompress::NZlib::CDecoder();
    _zlibDecoder = _zlibDecoderSpec;
  }
  if (!_inStream)
  {
    _inStreamSpec = new CBufInStream();
    _inStream = _inStreamSpec;
  }
  if (!_outStream)
  {
    _outStreamSpec = new CBufPtrSeqOutStream();
    _outStream = _outStreamSpec;
  }
  bool be = _h.be;
  const Byte *p = _data + (_curBlocksOffset + (UInt32)blockIndex * 4);
  UInt32 start = (blockIndex == 0 ? _curBlocksOffset + _curNumBlocks * 4: Get32(p - 4));
  UInt32 end = Get32(p);
  if (end < start || end > _size)
    return S_FALSE;
  UInt32 inSize = end - start;
  _inStreamSpec->Init(_data + start, inSize);
  _outStreamSpec->Init(dest, blockSize);
  RINOK(_zlibDecoder->Code(_inStream, _outStream, NULL, NULL, NULL));
  return (_zlibDecoderSpec->GetInputProcessedSize() == inSize &&
      _outStreamSpec->GetPos() == blockSize) ? S_OK : S_FALSE;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _items.Size();
  if (numItems == 0)
    return S_OK;
  bool be = _h.be;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const Byte *p = _data + _items[allFilesMode ? i : indices[i]].Offset;
    if (!IsDir(p, be))
      totalSize += GetSize(p, be);
  }
  extractCallback->SetTotal(totalSize);

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_stream);

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> outStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    const CItem &item = _items[index];
    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    const Byte *p = _data + item.Offset;

    if (IsDir(p, be))
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }
    UInt32 curSize = GetSize(p, be);
    totalSize += curSize;
    UInt32 packSize;
    if (GetPackSize(index, packSize))
      totalPackSize += packSize;

    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));

    UInt32 offset = GetOffset(p, be);
    if (offset < kHeaderSize)
      curSize = 0;

    int res = NExtract::NOperationResult::kDataError;
    {
      CMyComPtr<ISequentialInStream> inSeqStream;
      CMyComPtr<IInStream> inStream;
      HRESULT hres = GetStream(index, &inSeqStream);
      if (inSeqStream)
        inSeqStream.QueryInterface(IID_IInStream, &inStream);
      if (hres == E_OUTOFMEMORY)
        return E_OUTOFMEMORY;
      if (hres == S_FALSE || !inStream)
        res = NExtract::NOperationResult::kUnSupportedMethod;
      else
      {
        RINOK(hres);
        if (inStream)
        {
          HRESULT hres = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
          if (hres != S_OK && hres != S_FALSE)
          {
            RINOK(hres);
          }
          if (copyCoderSpec->TotalSize == curSize && hres == S_OK)
            res = NExtract::NOperationResult::kOK;
        }
      }
    }
    RINOK(extractCallback->SetOperationResult(res));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN

  const CItem &item = _items[index];
  const Byte *p = _data + item.Offset;

  bool be = _h.be;
  if (IsDir(p, be))
    return E_FAIL;

  UInt32 size = GetSize(p, be);
  UInt32 numBlocks = (size + kBlockSize - 1) >> kBlockSizeLog;
  UInt32 offset = GetOffset(p, be);
  if (offset < kHeaderSize)
  {
    if (offset != 0)
      return S_FALSE;
    CBufInStream *streamSpec = new CBufInStream;
    CMyComPtr<IInStream> streamTemp = streamSpec;
    streamSpec->Init(NULL, 0);
    *stream = streamTemp.Detach();
    return S_OK;
  }

  if (offset + numBlocks * 4 > _size)
    return S_FALSE;
  UInt32 prev = offset;
  for (UInt32 i = 0; i < numBlocks; i++)
  {
    UInt32 next = Get32(_data + offset + i * 4);
    if (next < prev || next > _size)
      return S_FALSE;
    prev = next;
  }

  CCramfsInStream *streamSpec = new CCramfsInStream;
  CMyComPtr<IInStream> streamTemp = streamSpec;
  _curNumBlocks = numBlocks;
  _curBlocksOffset = offset;
  streamSpec->Handler = this;
  if (!streamSpec->Alloc(kBlockSizeLog, 21 - kBlockSizeLog))
    return E_OUTOFMEMORY;
  streamSpec->Init(size);
  *stream = streamTemp.Detach();

  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new NArchive::NCramfs::CHandler; }

static CArcInfo g_ArcInfo =
  { L"CramFS", L"cramfs", 0, 0xD3, SIGNATURE, kSignatureSize, false, CreateArc, 0 };

REGISTER_ARC(Cramfs)

}}
