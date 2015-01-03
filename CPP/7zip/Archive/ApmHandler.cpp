// ApmHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../../Common/ComTry.h"
#include "../../Common/Defs.h"
#include "../../Common/IntToString.h"
#include "../../Common/MyString.h"

#include "../../Windows/PropVariant.h"

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

static const Byte kSig0 = 'E';
static const Byte kSig1 = 'R';

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
    numBlocksInMap = Get32(p + 4);
    StartBlock = Get32(p + 8);
    NumBlocks = Get32(p + 0xC);
    memcpy(Name, p + 0x10, 32);
    memcpy(Type, p + 0x30, 32);
    if (p[0] != 0x50 || p[1] != 0x4D || p[2] != 0 || p[3] != 0)
      return false;
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
  CRecordVector<CItem> _items;
  CMyComPtr<IInStream> _stream;
  unsigned _blockSizeLog;
  UInt32 _numBlocks;
  UInt64 _phySize;
  bool _isArc;

  HRESULT ReadTables(IInStream *stream);
  UInt64 BlocksToBytes(UInt32 i) const { return (UInt64)i << _blockSizeLog; }
  UInt64 GetItemSize(const CItem &item) const { return BlocksToBytes(item.NumBlocks); }
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

static const UInt32 kSectorSize = 512;

API_FUNC_static_IsArc IsArc_Apm(const Byte *p, size_t size)
{
  if (size < kSectorSize)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != kSig0 || p[1] != kSig1)
    return k_IsArc_Res_NO;
  unsigned i;
  for (i = 8; i < 16; i++)
    if (p[i] != 0)
      return k_IsArc_Res_NO;
  UInt32 blockSize = Get16(p + 2);
  for (i = 9; ((UInt32)1 << i) != blockSize; i++)
    if (i >= 12)
      return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

HRESULT CHandler::ReadTables(IInStream *stream)
{
  Byte buf[kSectorSize];
  {
    RINOK(ReadStream_FALSE(stream, buf, kSectorSize));
    if (buf[0] != kSig0 || buf[1] != kSig1)
      return S_FALSE;
    UInt32 blockSize = Get16(buf + 2);
    unsigned i;
    for (i = 9; ((UInt32)1 << i) != blockSize; i++)
      if (i >= 12)
        return S_FALSE;
    _blockSizeLog = i;
    _numBlocks = Get32(buf + 4);
    for (i = 8; i < 16; i++)
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
    
    UInt32 numBlocksInMap2 = 0;
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
  
  _phySize = BlocksToBytes(_numBlocks);
  _isArc = true;
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback * /* callback */)
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
  _isArc = false;
  _phySize = 0;
  _items.Clear();
  _stream.Release();
  return S_OK;
}

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidOffset
};

static const Byte kArcProps[] =
{
  kpidClusterSize
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static AString GetString(const char *s)
{
  AString res;
  for (unsigned i = 0; i < 32 && s[i] != 0; i++)
    res += s[i];
  return res;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile:
    {
      int mainIndex = -1;
      FOR_VECTOR (i, _items)
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
    case kpidPhySize: prop = _phySize; break;

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      prop = v;
      break;
    }
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
  switch (propID)
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
  bool allFilesMode = (numItems == (UInt32)(Int32)-1);
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

IMP_CreateArcIn

static CArcInfo g_ArcInfo =
  { "APM", "apm", 0, 0xD4,
  2, { kSig0, kSig1 },
  0,
  0,
  CreateArc, NULL, IsArc_Apm };

REGISTER_ARC(Apm)

}}
