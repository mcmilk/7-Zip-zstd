// VhdHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
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
#define Get64(p) GetBe64(p)

#define G32(p, dest) dest = Get32(p);
#define G64(p, dest) dest = Get64(p);

using namespace NWindows;

namespace NArchive {
namespace NVhd {

static const UInt32 kUnusedBlock = 0xFFFFFFFF;

static const UInt32 kDiskType_Fixed = 2;
static const UInt32 kDiskType_Dynamic = 3;
static const UInt32 kDiskType_Diff = 4;

static const char *kDiskTypes[] =
{
  "0",
  "1",
  "Fixed",
  "Dynamic",
  "Differencing"
};

struct CFooter
{
  // UInt32 Features;
  // UInt32 FormatVersion;
  UInt64 DataOffset;
  UInt32 CTime;
  UInt32 CreatorApp;
  UInt32 CreatorVersion;
  UInt32 CreatorHostOS;
  // UInt64 OriginalSize;
  UInt64 CurrentSize;
  UInt32 DiskGeometry;
  UInt32 Type;
  Byte Id[16];
  Byte SavedState;

  bool IsFixed() const { return Type == kDiskType_Fixed; }
  bool ThereIsDynamic() const { return Type == kDiskType_Dynamic || Type == kDiskType_Diff; }
  // bool IsSupported() const { return Type == kDiskType_Fixed || Type == kDiskType_Dynamic || Type == kDiskType_Diff; }
  UInt32 NumCyls() const { return DiskGeometry >> 16; }
  UInt32 NumHeads() const { return (DiskGeometry >> 8) & 0xFF; }
  UInt32 NumSectorsPerTrack() const { return DiskGeometry & 0xFF; }
  AString GetTypeString() const;
  bool Parse(const Byte *p);
};

AString CFooter::GetTypeString() const
{
  if (Type < sizeof(kDiskTypes) / sizeof(kDiskTypes[0]))
    return kDiskTypes[Type];
  char s[16];
  ConvertUInt32ToString(Type, s);
  return s;
}

static bool CheckBlock(const Byte *p, unsigned size, unsigned checkSumOffset, unsigned zeroOffset)
{
  UInt32 sum = 0;
  unsigned i;
  for (i = 0; i < checkSumOffset; i++)
    sum += p[i];
  for (i = checkSumOffset + 4; i < size; i++)
    sum += p[i];
  if (~sum != Get32(p + checkSumOffset))
    return false;
  for (i = zeroOffset; i < size; i++)
    if (p[i] != 0)
      return false;
  return true;
}

bool CFooter::Parse(const Byte *p)
{
  if (memcmp(p, "conectix", 8) != 0)
    return false;
  // G32(p + 0x08, Features);
  // G32(p + 0x0C, FormatVersion);
  G64(p + 0x10, DataOffset);
  G32(p + 0x18, CTime);
  G32(p + 0x1C, CreatorApp);
  G32(p + 0x20, CreatorVersion);
  G32(p + 0x24, CreatorHostOS);
  // G64(p + 0x28, OriginalSize);
  G64(p + 0x30, CurrentSize);
  G32(p + 0x38, DiskGeometry);
  G32(p + 0x3C, Type);
  memcpy(Id, p + 0x44, 16);
  SavedState = p[0x54];
  return CheckBlock(p, 512, 0x40, 0x55);
}

/*
struct CParentLocatorEntry
{
  UInt32 Code;
  UInt32 DataSpace;
  UInt32 DataLen;
  UInt64 DataOffset;

  bool Parse(const Byte *p);
};
bool CParentLocatorEntry::Parse(const Byte *p)
{
  G32(p + 0x00, Code);
  G32(p + 0x04, DataSpace);
  G32(p + 0x08, DataLen);
  G32(p + 0x10, DataOffset);
  return (Get32(p + 0x0C) == 0); // Resrved
}
*/

struct CDynHeader
{
  // UInt64 DataOffset;
  UInt64 TableOffset;
  // UInt32 HeaderVersion;
  UInt32 NumBlocks;
  int BlockSizeLog;
  UInt32 ParentTime;
  Byte ParentId[16];
  UString ParentName;
  // CParentLocatorEntry ParentLocators[8];

  bool Parse(const Byte *p);
  UInt32 NumBitMapSectors() const
  {
    UInt32 numSectorsInBlock = (1 << (BlockSizeLog - 9));
    return (numSectorsInBlock + 512 * 8 - 1) / (512 * 8);
  }
};

static int GetLog(UInt32 num)
{
  for (int i = 0; i < 31; i++)
    if (((UInt32)1 << i) == num)
      return i;
  return -1;
}

bool CDynHeader::Parse(const Byte *p)
{
  if (memcmp(p, "cxsparse", 8) != 0)
    return false;
  // G64(p + 0x08, DataOffset);
  G64(p + 0x10, TableOffset);
  // G32(p + 0x18, HeaderVersion);
  G32(p + 0x1C, NumBlocks);
  BlockSizeLog = GetLog(Get32(p + 0x20));
  if (BlockSizeLog < 9 || BlockSizeLog > 30)
    return false;
  G32(p + 0x38, ParentTime);
  if (Get32(p + 0x3C) != 0) // reserved
    return false;
  memcpy(ParentId, p + 0x28, 16);
  {
    const int kNameLength = 256;
    wchar_t *s = ParentName.GetBuffer(kNameLength);
    for (unsigned i = 0; i < kNameLength; i++)
      s[i] = Get16(p + 0x40 + i * 2);
    s[kNameLength] = 0;
    ParentName.ReleaseBuffer();
  }
  /*
  for (int i = 0; i < 8; i++)
    if (!ParentLocators[i].Parse(p + 0x240 + i * 24))
      return false;
  */
  return CheckBlock(p, 1024, 0x24, 0x240 + 8 * 24);
}

class CHandler:
  public IInStream,
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  UInt64 _virtPos;
  UInt64 _phyPos;
  UInt64 _phyLimit;

  CFooter Footer;
  CDynHeader Dyn;
  CRecordVector<UInt32> Bat;
  CByteBuffer BitMap;
  UInt32 BitMapTag;
  UInt32 NumUsedBlocks;
  CMyComPtr<IInStream> Stream;
  CMyComPtr<IInStream> ParentStream;
  CHandler *Parent;

  HRESULT Seek(UInt64 offset);
  HRESULT InitAndSeek();
  HRESULT ReadPhy(UInt64 offset, void *data, UInt32 size);

  bool NeedParent() const { return Footer.Type == kDiskType_Diff; }
  UInt64 GetPackSize() const
    { return Footer.ThereIsDynamic() ? ((UInt64)NumUsedBlocks << Dyn.BlockSizeLog) : Footer.CurrentSize; }

  UString GetParentName() const
  {
    const CHandler *p = this;
    UString res;
    while (p && p->NeedParent())
    {
      if (!res.IsEmpty())
        res += L" -> ";
      res += p->Dyn.ParentName;
      p = p->Parent;
    }
    return res;
  }

  bool IsOK() const
  {
    const CHandler *p = this;
    while (p->NeedParent())
    {
      p = p->Parent;
      if (p == 0)
        return false;
    }
    return true;
  }

  HRESULT Open3();
  HRESULT Open2(IInStream *stream, CHandler *child, IArchiveOpenCallback *openArchiveCallback, int level);

public:
  MY_UNKNOWN_IMP3(IInArchive, IInArchiveGetStream, IInStream)

  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
};

HRESULT CHandler::Seek(UInt64 offset) { return Stream->Seek(offset, STREAM_SEEK_SET, NULL); }

HRESULT CHandler::InitAndSeek()
{
  if (ParentStream)
  {
    RINOK(Parent->InitAndSeek());
  }
  _virtPos = _phyPos = 0;
  BitMapTag = kUnusedBlock;
  BitMap.SetCapacity(Dyn.NumBitMapSectors() << 9);
  return Seek(0);
}

HRESULT CHandler::ReadPhy(UInt64 offset, void *data, UInt32 size)
{
  if (offset + size > _phyLimit)
    return S_FALSE;
  if (offset != _phyPos)
  {
    _phyPos = offset;
    RINOK(Seek(offset));
  }
  HRESULT res = ReadStream_FALSE(Stream, data, size);
  _phyPos += size;
  return res;
}

HRESULT CHandler::Open3()
{
  RINOK(Stream->Seek(0, STREAM_SEEK_END, &_phyPos));
  if (_phyPos < 512)
    return S_FALSE;
  const UInt32 kDynSize = 1024;
  Byte buf[kDynSize];

  _phyLimit = _phyPos;
  RINOK(ReadPhy(_phyLimit - 512, buf, 512));
  if (!Footer.Parse(buf))
    return S_FALSE;
  _phyLimit -= 512;

  if (!Footer.ThereIsDynamic())
    return S_OK;

  RINOK(ReadPhy(0, buf + 512, 512));
  if (memcmp(buf, buf + 512, 512) != 0)
    return S_FALSE;

  RINOK(ReadPhy(Footer.DataOffset, buf, kDynSize));
  if (!Dyn.Parse(buf))
    return S_FALSE;
  
  if (Dyn.NumBlocks >= (UInt32)1 << 31)
    return S_FALSE;
  if (Footer.CurrentSize == 0)
  {
    if (Dyn.NumBlocks != 0)
      return S_FALSE;
  }
  else if (((Footer.CurrentSize - 1) >> Dyn.BlockSizeLog) + 1 != Dyn.NumBlocks)
    return S_FALSE;

  Bat.Reserve(Dyn.NumBlocks);
  while ((UInt32)Bat.Size() < Dyn.NumBlocks)
  {
    RINOK(ReadPhy(Dyn.TableOffset + (UInt64)Bat.Size() * 4, buf, 512));
    for (UInt32 j = 0; j < 512; j += 4)
    {
      UInt32 v = Get32(buf + j);
      if (v != kUnusedBlock)
        NumUsedBlocks++;
      Bat.Add(v);
      if ((UInt32)Bat.Size() >= Dyn.NumBlocks)
        break;
    }
  }
  return S_OK;
}

STDMETHODIMP CHandler::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize != NULL)
    *processedSize = 0;
  if (_virtPos >= Footer.CurrentSize)
    return (Footer.CurrentSize == _virtPos) ? S_OK: E_FAIL;
  UInt64 rem = Footer.CurrentSize - _virtPos;
  if (size > rem)
    size = (UInt32)rem;
  if (size == 0)
    return S_OK;
  UInt32 blockIndex = (UInt32)(_virtPos >> Dyn.BlockSizeLog);
  UInt32 blockSectIndex = Bat[blockIndex];
  UInt32 blockSize = (UInt32)1 << Dyn.BlockSizeLog;
  UInt32 offsetInBlock = (UInt32)_virtPos & (blockSize - 1);
  size = MyMin(blockSize - offsetInBlock, size);

  HRESULT res = S_OK;
  if (blockSectIndex == kUnusedBlock)
  {
    if (ParentStream)
    {
      RINOK(ParentStream->Seek(_virtPos, STREAM_SEEK_SET, NULL));
      res = ParentStream->Read(data, size, &size);
    }
    else
      memset(data, 0, size);
  }
  else
  {
    UInt64 newPos = (UInt64)blockSectIndex << 9;
    if (BitMapTag != blockIndex)
    {
      RINOK(ReadPhy(newPos, BitMap, (UInt32)BitMap.GetCapacity()));
      BitMapTag = blockIndex;
    }
    RINOK(ReadPhy(newPos + BitMap.GetCapacity() + offsetInBlock, data, size));
    for (UInt32 cur = 0; cur < size;)
    {
      UInt32 rem = MyMin(0x200 - (offsetInBlock & 0x1FF), size - cur);
      UInt32 bmi = offsetInBlock >> 9;
      if (((BitMap[bmi >> 3] >> (7 - (bmi & 7))) & 1) == 0)
      {
        if (ParentStream)
        {
          RINOK(ParentStream->Seek(_virtPos + cur, STREAM_SEEK_SET, NULL));
          RINOK(ReadStream_FALSE(ParentStream, (Byte *)data + cur, rem));
        }
        else
        {
          const Byte *p = (const Byte *)data + cur;
          for (UInt32 i = 0; i < rem; i++)
            if (p[i] != 0)
              return S_FALSE;
        }
      }
      offsetInBlock += rem;
      cur += rem;
    }
  }
  if (processedSize != NULL)
    *processedSize = size;
  _virtPos += size;
  return res;
}

STDMETHODIMP CHandler::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch(seekOrigin)
  {
    case STREAM_SEEK_SET: _virtPos = offset; break;
    case STREAM_SEEK_CUR: _virtPos += offset; break;
    case STREAM_SEEK_END: _virtPos = Footer.CurrentSize + offset; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

enum
{
  kpidParent = kpidUserDefined,
  kpidSavedState
};

STATPROPSTG kArcProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidClusterSize, VT_UI8},
  { NULL, kpidMethod, VT_BSTR},
  { L"Parent", kpidParent, VT_BSTR},
  { NULL, kpidCreatorApp, VT_BSTR},
  { NULL, kpidHostOS, VT_BSTR},
  { L"Saved State", kpidSavedState, VT_BOOL},
  { NULL, kpidId, VT_BSTR}
 };

STATPROPSTG kProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME}
  
  /*
  { NULL, kpidNumCyls, VT_UI4},
  { NULL, kpidNumHeads, VT_UI4},
  { NULL, kpidSectorsPerTrack, VT_UI4}
  */
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

// VHD start time: 2000-01-01
static const UInt64 kVhdTimeStartValue = (UInt64)3600 * 24 * (399 * 365 + 24 * 4);

static void VhdTimeToFileTime(UInt32 vhdTime, NCOM::CPropVariant &prop)
{
  FILETIME ft, utc;
  UInt64 v = (kVhdTimeStartValue + vhdTime) * 10000000;
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
  // specification says that it's UTC time, but Virtual PC 6 writes local time. Why?
  LocalFileTimeToFileTime(&ft, &utc);
  prop = utc;
}

static void StringToAString(char *dest, UInt32 s)
{
  for (int i = 24; i >= 0; i -= 8)
  {
    Byte b = (Byte)((s >> i) & 0xFF);
    if (b < 0x20 || b > 0x7F)
      break;
    *dest++ = b;
  }
  *dest = 0;
}

static void ConvertByteToHex(unsigned value, char *s)
{
  for (int i = 0; i < 2; i++)
  {
    unsigned t = value & 0xF;
    value >>= 4;
    s[1 - i] = (char)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
  }
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidCTime: VhdTimeToFileTime(Footer.CTime, prop); break;
    case kpidClusterSize: if (Footer.ThereIsDynamic()) prop = (UInt32)1 << Dyn.BlockSizeLog; break;
    case kpidMethod:
    {
      AString s = Footer.GetTypeString();
      if (NeedParent())
      {
        s += " -> ";
        const CHandler *p = this;
        while (p != 0 && p->NeedParent())
          p = p->Parent;
        if (p == 0)
          s += '?';
        else
          s += p->Footer.GetTypeString();
      }
      prop = s;
      break;
    }
    case kpidCreatorApp:
    {
      char s[16];
      StringToAString(s, Footer.CreatorApp);
      AString res = s;
      res.Trim();
      ConvertUInt32ToString(Footer.CreatorVersion >> 16, s);
      res += ' ';
      res += s;
      res += '.';
      ConvertUInt32ToString(Footer.CreatorVersion & 0xFFFF, s);
      res += s;
      prop = res;
      break;
    }
    case kpidHostOS:
    {
      if (Footer.CreatorHostOS == 0x5769326b)
        prop = "Windows";
      else
      {
        char s[16];
        StringToAString(s, Footer.CreatorHostOS);
        prop = s;
      }
      break;
    }
    case kpidId:
    {
      char s[32 + 4];
      for (int i = 0; i < 16; i++)
        ConvertByteToHex(Footer.Id[i], s + i * 2);
      s[32] = 0;
      prop = s;
      break;
    }
    case kpidSavedState: prop = Footer.SavedState ? true : false; break;
    case kpidParent: if (NeedParent()) prop = GetParentName(); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *stream, CHandler *child, IArchiveOpenCallback *openArchiveCallback, int level)
{
  Close();
  Stream = stream;
  if (level > 32)
    return S_FALSE;
  RINOK(Open3());
  if (child && memcmp(child->Dyn.ParentId, Footer.Id, 16) != 0)
    return S_FALSE;
  if (Footer.Type != kDiskType_Diff)
    return S_OK;
  CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
  if (openArchiveCallback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback) != S_OK)
    return S_FALSE;
  CMyComPtr<IInStream> nextStream;
  HRESULT res = openVolumeCallback->GetStream(Dyn.ParentName, &nextStream);
  if (res == S_FALSE)
    return S_OK;
  RINOK(res);

  Parent = new CHandler;
  ParentStream = Parent;
  return Parent->Open2(nextStream, this, openArchiveCallback, level + 1);
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * openArchiveCallback)
{
  COM_TRY_BEGIN
  {
    HRESULT res;
    try
    {
      res = Open2(stream, NULL, openArchiveCallback, 0);
      if (res == S_OK)
        return S_OK;
    }
    catch(...)
    {
      Close();
      throw;
    }
    Close();
    return res;
  }
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  Bat.Clear();
  NumUsedBlocks = 0;
  Parent = 0;
  Stream.Release();
  ParentStream.Release();
  return S_OK;
}

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
    case kpidSize: prop = Footer.CurrentSize; break;
    case kpidPackSize: prop = GetPackSize(); break;
    case kpidCTime: VhdTimeToFileTime(Footer.CTime, prop); break;
    /*
    case kpidNumCyls: prop = Footer.NumCyls(); break;
    case kpidNumHeads: prop = Footer.NumHeads(); break;
    case kpidSectorsPerTrack: prop = Footer.NumSectorsPerTrack(); break;
    */
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  RINOK(extractCallback->SetTotal(Footer.CurrentSize));
  CMyComPtr<ISequentialOutStream> outStream;
  Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &outStream, askMode));
  if (!testMode && !outStream)
    return S_OK;
  RINOK(extractCallback->PrepareOperation(askMode));

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  int res = NExtract::NOperationResult::kDataError;
  CMyComPtr<ISequentialInStream> inStream;
  HRESULT hres = GetStream(0, &inStream);
  if (hres == S_FALSE)
    res = NExtract::NOperationResult::kUnSupportedMethod;
  else
  {
    RINOK(hres);
    HRESULT hres = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
    if (hres == S_OK)
    {
      if (copyCoderSpec->TotalSize == Footer.CurrentSize)
        res = NExtract::NOperationResult::kOK;
    }
    else
    {
      if (hres != S_FALSE)
      {
        RINOK(hres);
      }
    }
  }
  outStream.Release();
  return extractCallback->SetOperationResult(res);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  *stream = 0;
  if (Footer.IsFixed())
  {
    CLimitedInStream *streamSpec = new CLimitedInStream;
    CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
    streamSpec->SetStream(Stream);
    streamSpec->InitAndSeek(0, Footer.CurrentSize);
    RINOK(streamSpec->SeekToStart());
    *stream = streamTemp.Detach();
    return S_OK;
  }
  if (!Footer.ThereIsDynamic() || !IsOK())
    return S_FALSE;
  CMyComPtr<ISequentialInStream> streamTemp = this;
  RINOK(InitAndSeek());
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"VHD", L"vhd", L".mbr", 0xDC, { 'c', 'o', 'n', 'e', 'c', 't', 'i', 'x', 0, 0 }, 10, false, CreateArc, 0 };

REGISTER_ARC(Vhd)

}}
