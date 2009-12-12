// ApmHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/MyString.h"

#include "Windows/PropVariant.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)

using namespace NWindows;

namespace NArchive {
namespace NApm {

struct CItem
{
  UInt32 StartBlock;
  UInt32 NumBlocks;
  char Name[32];
  char Type[32];
  /*
  UInt32 DataStartBlock;
  UInt32 NumDataBlocks;
  UInt32 Status;
  UInt32 BootStartBlock;
  UInt32 BootSize;
  UInt32 BootAddr;
  UInt32 BootEntry;
  UInt32 BootChecksum;
  char Processor[16];
  */

  bool Parse(const Byte *p, UInt32 &numBlocksInMap)
  {
    if (p[0] != 0x50 || p[1] != 0x4D || p[2] != 0 || p[3] != 0)
      return false;
    numBlocksInMap = Get32(p + 4);
    StartBlock = Get32(p + 8);
    NumBlocks = Get32(p + 0xC);
    memcpy(Name, p + 0x10, 32);
    memcpy(Type, p + 0x30, 32);
    /*
    DataStartBlock = Get32(p + 0x50);
    NumDataBlocks = Get32(p + 0x54);
    Status = Get32(p + 0x58);
    BootStartBlock = Get32(p + 0x5C);
    BootSize = Get32(p + 0x60);
    BootAddr = Get32(p + 0x64);
    if (Get32(p + 0x68) != 0)
      return false;
    BootEntry = Get32(p + 0x6C);
    if (Get32(p + 0x70) != 0)
      return false;
    BootChecksum = Get32(p + 0x74);
    memcpy(Processor, p + 0x78, 16);
    */
    return true;
  }
};

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  CRecordVector<CItem> _items;

  int _blockSizeLog;
  UInt32 _numBlocks;

  HRESULT ReadTables(IInStream *stream);
  UInt64 BlocksToBytes(UInt32 i) const { return (UInt64)i << _blockSizeLog; }
  UInt64 GetItemSize(const CItem &item) { return BlocksToBytes(item.NumBlocks); }
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

static inline int GetLog(UInt32 num)
{
  for (int i = 0; i < 31; i++)
    if (((UInt32)1 << i) == num)
      return i;
  return -1;
}

HRESULT CHandler::ReadTables(IInStream *stream)
{
  const UInt32 kSectorSize = 512;
  Byte buf[kSectorSize];
  {
    RINOK(ReadStream_FALSE(stream, buf, kSectorSize));
    if (buf[0] != 0x45 || buf[1] != 0x52)
      return S_FALSE;
    _blockSizeLog = GetLog(Get16(buf + 2));
    if (_blockSizeLog < 9 || _blockSizeLog > 14)
      return S_FALSE;
    _numBlocks = Get32(buf + 4);
    for (int i = 8; i < 16; i++)
      if (buf[i] != 0)
        return S_FALSE;
  }

  unsigned numSkips = (unsigned)1 << (_blockSizeLog - 9);
  for (unsigned j = 1; j < numSkips; j++)
  {
    RINOK(ReadStream_FALSE(stream, buf, kSectorSize));
  }

  UInt32 numBlocksInMap = 0;
  for (unsigned i = 0;;)
  {
    RINOK(ReadStream_FALSE(stream, buf, kSectorSize));
 
    CItem item;
    
    UInt32 numBlocksInMap2;
    if (!item.Parse(buf, numBlocksInMap2))
      return S_FALSE;
    if (i == 0)
    {
      numBlocksInMap = numBlocksInMap2;
      if (numBlocksInMap > (1 << 8))
        return S_FALSE;
    }
    else if (numBlocksInMap2 != numBlocksInMap)
      return S_FALSE;

    UInt32 finish = item.StartBlock + item.NumBlocks;
    if (finish < item.StartBlock)
      return S_FALSE;
    _numBlocks = MyMax(_numBlocks, finish);
    
    _items.Add(item);
    for (unsigned j = 1; j < numSkips; j++)
    {
      RINOK(ReadStream_FALSE(stream, buf, kSectorSize));
    }
    if (++i == numBlocksInMap)
      break;
  }
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  Close();
  RINOK(ReadTables(stream));
  _stream = stream;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _items.Clear();
  _stream.Release();
  return S_OK;
}

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidOffset, VT_UI8}
};

STATPROPSTG kArcProps[] =
{
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidPhySize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static AString GetString(const char *s)
{
  AString res;
  for (int i = 0; i < 32 && s[i] != 0; i++)
    res += s[i];
  return res;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidMainSubfile:
    {
      int mainIndex = -1;
      for (int i = 0; i < _items.Size(); i++)
      {
        AString s = GetString(_items[i].Type);
        if (s != "Apple_Free" &&
            s != "Apple_partition_map")
        {
          if (mainIndex >= 0)
          {
            mainIndex = -1;
            break;
          }
          mainIndex = i;
        }
      }
      if (mainIndex >= 0)
        prop = (UInt32)mainIndex;
      break;
    }
    case kpidClusterSize: prop = (UInt32)1 << _blockSizeLog; break;
    case kpidPhySize: prop = BlocksToBytes(_numBlocks); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CItem &item = _items[index];
  switch(propID)
  {
    case kpidPath:
    {
      AString s = GetString(item.Name);
      if (s.IsEmpty())
      {
        char s2[32];
        ConvertUInt32ToString(index, s2);
        s = s2;
      }
      AString type = GetString(item.Type);
      if (type == "Apple_HFS")
        type = "hfs";
      if (!type.IsEmpty())
      {
        s += '.';
        s += type;
      }
      prop = s;
      break;
    }
    case kpidSize:
    case kpidPackSize:
      prop = GetItemSize(item);
      break;
    case kpidOffset: prop = BlocksToBytes(item.StartBlock); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
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
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += GetItemSize(_items[allFilesMode ? i : indices[i]]);
  extractCallback->SetTotal(totalSize);

  totalSize = 0;
  
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
    lps->InSize = totalSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> outStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItem &item = _items[index];

    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    UInt64 size = GetItemSize(item);
    totalSize += size;
    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));

    RINOK(_stream->Seek(BlocksToBytes(item.StartBlock), STREAM_SEEK_SET, NULL));
    streamSpec->Init(size);
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(copyCoderSpec->TotalSize == size ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  const CItem &item = _items[index];
  return CreateLimitedInStream(_stream, BlocksToBytes(item.StartBlock), GetItemSize(item), stream);
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"APM", L"", 0, 0xD4, { 0x50, 0x4D, 0, 0, 0, 0, 0 }, 7, false, CreateArc, 0 };

REGISTER_ARC(Apm)

}}
