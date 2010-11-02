// PeHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/DynamicBuffer.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariantUtils.h"
#include "Windows/Time.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

using namespace NWindows;

namespace NArchive {
namespace NPe {

#define NUM_SCAN_SECTIONS_MAX (1 << 6)

#define PE_SIG 0x00004550
#define PE_OptHeader_Magic_32 0x10B
#define PE_OptHeader_Magic_64 0x20B

static AString GetDecString(UInt32 v)
{
  char sz[32];
  ConvertUInt64ToString(v, sz);
  return sz;
}

struct CVersion
{
  UInt16 Major;
  UInt16 Minor;

  void Parse(const Byte *buf);
  AString GetString() const { return GetDecString(Major) + '.' + GetDecString(Minor); }
};

void CVersion::Parse(const Byte *p)
{
  Major = Get16(p);
  Minor = Get16(p + 2);
}

static const UInt32 kHeaderSize =  4 + 20;

struct CHeader
{
  UInt16 NumSections;
  UInt32 Time;
  UInt32 PointerToSymbolTable;
  UInt32 NumSymbols;
  UInt16 OptHeaderSize;
  UInt16 Flags;
  UInt16 Machine;

  bool Parse(const Byte *buf);
};

bool CHeader::Parse(const Byte *p)
{
  if (Get32(p) != PE_SIG)
    return false;
  p += 4;
  Machine = Get16(p + 0);
  NumSections = Get16(p + 2);
  Time = Get32(p + 4);
  PointerToSymbolTable = Get32(p + 8);
  NumSymbols = Get32(p + 12);
  OptHeaderSize = Get16(p + 16);
  Flags = Get16(p + 18);
  return true;
}

struct CDirLink
{
  UInt32 Va;
  UInt32 Size;
  void Parse(const Byte *p);
};

void CDirLink::Parse(const Byte *p)
{
  Va = Get32(p);
  Size = Get32(p + 4);
}

enum
{
  kDirLink_Certificate = 4,
  kDirLink_Debug = 6
};

struct CDebugEntry
{
  UInt32 Flags;
  UInt32 Time;
  CVersion Ver;
  UInt32 Type;
  UInt32 Size;
  UInt32 Va;
  UInt32 Pa;
  
  void Parse(const Byte *p);
};

void CDebugEntry::Parse(const Byte *p)
{
  Flags = Get32(p);
  Time = Get32(p + 4);
  Ver.Parse(p + 8);
  Type = Get32(p + 12);
  Size = Get32(p + 16);
  Va = Get32(p + 20);
  Pa = Get32(p + 24);
}

static const UInt32 kNumDirItemsMax = 16;

struct COptHeader
{
  UInt16 Magic;
  Byte LinkerVerMajor;
  Byte LinkerVerMinor;

  UInt32 CodeSize;
  UInt32 InitDataSize;
  UInt32 UninitDataSize;
  
  // UInt32 AddressOfEntryPoint;
  // UInt32 BaseOfCode;
  // UInt32 BaseOfData32;
  UInt64 ImageBase;

  UInt32 SectAlign;
  UInt32 FileAlign;

  CVersion OsVer;
  CVersion ImageVer;
  CVersion SubsysVer;
  
  UInt32 ImageSize;
  UInt32 HeadersSize;
  UInt32 CheckSum;
  UInt16 SubSystem;
  UInt16 DllCharacts;

  UInt64 StackReserve;
  UInt64 StackCommit;
  UInt64 HeapReserve;
  UInt64 HeapCommit;

  UInt32 NumDirItems;
  CDirLink DirItems[kNumDirItemsMax];

  bool Is64Bit() const { return Magic == PE_OptHeader_Magic_64; }
  bool Parse(const Byte *p, UInt32 size);

  int GetNumFileAlignBits() const
  {
    for (int i = 9; i <= 16; i++)
      if (((UInt32)1 << i) == FileAlign)
        return i;
    return -1;
  }
};

bool COptHeader::Parse(const Byte *p, UInt32 size)
{
  Magic = Get16(p);
  switch (Magic)
  {
    case PE_OptHeader_Magic_32:
    case PE_OptHeader_Magic_64:
      break;
    default:
      return false;
  }
  LinkerVerMajor = p[2];
  LinkerVerMinor = p[3];
  
  bool hdr64 = Is64Bit();
  
  CodeSize = Get32(p + 4);
  InitDataSize = Get32(p + 8);
  UninitDataSize = Get32(p + 12);

  // AddressOfEntryPoint = Get32(p + 16);
  // BaseOfCode = Get32(p + 20);
  // BaseOfData32 = hdr64 ? 0: Get32(p + 24);
  ImageBase = hdr64 ? GetUi64(p + 24) : Get32(p + 28);

  SectAlign = Get32(p + 32);
  FileAlign = Get32(p + 36);

  OsVer.Parse(p + 40);
  ImageVer.Parse(p + 44);
  SubsysVer.Parse(p + 48);

  // reserved = Get32(p + 52);

  ImageSize = Get32(p + 56);
  HeadersSize = Get32(p + 60);
  CheckSum = Get32(p + 64);
  SubSystem = Get16(p + 68);
  DllCharacts = Get16(p + 70);

  if (hdr64)
  {
    StackReserve = Get64(p + 72);
    StackCommit = Get64(p + 80);
    HeapReserve = Get64(p + 88);
    HeapCommit = Get64(p + 96);
  }
  else
  {
    StackReserve = Get32(p + 72);
    StackCommit = Get32(p + 76);
    HeapReserve = Get32(p + 80);
    HeapCommit = Get32(p + 84);
  }
  UInt32 pos = (hdr64 ? 108 : 92);
  NumDirItems = Get32(p + pos);
  pos += 4;
  if (pos + 8 * NumDirItems != size)
    return false;
  for (UInt32 i = 0; i < NumDirItems && i < kNumDirItemsMax; i++)
    DirItems[i].Parse(p + pos + i * 8);
  return true;
}

static const UInt32 kSectionSize = 40;

struct CSection
{
  AString Name;

  UInt32 VSize;
  UInt32 Va;
  UInt32 PSize;
  UInt32 Pa;
  UInt32 Flags;
  UInt32 Time;
  // UInt16 NumRelocs;
  bool IsDebug;
  bool IsRealSect;
  bool IsAdditionalSection;

  CSection(): IsRealSect(false), IsDebug(false), IsAdditionalSection(false) {}
  UInt64 GetPackSize() const { return PSize; }

  void UpdateTotalSize(UInt32 &totalSize)
  {
    UInt32 t = Pa + PSize;
    if (t > totalSize)
      totalSize = t;
  }
  void Parse(const Byte *p);
};

static bool operator <(const CSection &a1, const CSection &a2) { return (a1.Pa < a2.Pa) || ((a1.Pa == a2.Pa) && (a1.PSize < a2.PSize)) ; }
static bool operator ==(const CSection &a1, const CSection &a2) { return (a1.Pa == a2.Pa) && (a1.PSize == a2.PSize); }

static AString GetName(const Byte *name)
{
  const int kNameSize = 8;
  AString res;
  char *p = res.GetBuffer(kNameSize);
  memcpy(p, name, kNameSize);
  p[kNameSize] = 0;
  res.ReleaseBuffer();
  return res;
}

void CSection::Parse(const Byte *p)
{
  Name = GetName(p);
  VSize = Get32(p + 8);
  Va = Get32(p + 12);
  PSize = Get32(p + 16);
  Pa = Get32(p + 20);
  // NumRelocs = Get16(p + 32);
  Flags = Get32(p + 36);
}

static const CUInt32PCharPair g_HeaderCharacts[] =
{
  {  1, "Executable" },
  { 13, "DLL" },
  {  8, "32-bit" },
  {  5, "LargeAddress" },
  {  0, "NoRelocs" },
  {  2, "NoLineNums" },
  {  3, "NoLocalSyms" },
  {  4, "AggressiveWsTrim" },
  {  9, "NoDebugInfo" },
  { 10, "RemovableRun" },
  { 11, "NetRun" },
  { 12, "System" },
  { 14, "UniCPU" },
  {  7, "Little-Endian" },
  { 15, "Big-Endian" }
};

static const CUInt32PCharPair g_DllCharacts[] =
{
  {  6, "Relocated" },
  {  7, "Integrity" },
  {  8, "NX-Compatible" },
  {  9, "NoIsolation" },
  { 10, "NoSEH" },
  { 11, "NoBind" },
  { 13, "WDM" },
  { 15, "TerminalServerAware" }
};

static const CUInt32PCharPair g_SectFlags[] =
{
  {  3, "NoPad" },
  {  5, "Code" },
  {  6, "InitializedData" },
  {  7, "UninitializedData" },
  {  9, "Comments" },
  { 11, "Remove" },
  { 12, "COMDAT" },
  { 15, "GP" },
  { 24, "ExtendedRelocations" },
  { 25, "Discardable" },
  { 26, "NotCached" },
  { 27, "NotPaged" },
  { 28, "Shared" },
  { 29, "Execute" },
  { 30, "Read" },
  { 31, "Write" }
};

static const CUInt32PCharPair g_MachinePairs[] =
{
  { 0x014C, "x86" },
  { 0x0162, "MIPS-R3000" },
  { 0x0166, "MIPS-R4000" },
  { 0x0168, "MIPS-R10000" },
  { 0x0169, "MIPS-V2" },
  { 0x0184, "Alpha" },
  { 0x01A2, "SH3" },
  { 0x01A3, "SH3-DSP" },
  { 0x01A4, "SH3E" },
  { 0x01A6, "SH4" },
  { 0x01A8, "SH5" },
  { 0x01C0, "ARM" },
  { 0x01C2, "ARM-Thumb" },
  { 0x01F0, "PPC" },
  { 0x01F1, "PPC-FP" },
  { 0x0200, "IA-64" },
  { 0x0284, "Alpha-64" },
  { 0x0200, "IA-64" },
  { 0x0366, "MIPSFPU" },
  { 0x8664, "x64" },
  { 0x0EBC, "EFI" }
};

static const CUInt32PCharPair g_SubSystems[] =
{
  { 0, "Unknown" },
  { 1, "Native" },
  { 2, "Windows GUI" },
  { 3, "Windows CUI" },
  { 7, "Posix" },
  { 9, "Windows CE" },
  { 10, "EFI" },
  { 11, "EFI Boot" },
  { 12, "EFI Runtime" },
  { 13, "EFI ROM" },
  { 14, "XBOX" }
};

static const wchar_t *g_ResTypes[] =
{
  NULL,
  L"CURSOR",
  L"BITMAP",
  L"ICON",
  L"MENU",
  L"DIALOG",
  L"STRING",
  L"FONTDIR",
  L"FONT",
  L"ACCELERATOR",
  L"RCDATA",
  L"MESSAGETABLE",
  L"GROUP_CURSOR",
  NULL,
  L"GROUP_ICON",
  NULL,
  L"VERSION",
  L"DLGINCLUDE",
  NULL,
  L"PLUGPLAY",
  L"VXD",
  L"ANICURSOR",
  L"ANIICON",
  L"HTML",
  L"MANIFEST"
};

const UInt32 kFlag = (UInt32)1 << 31;
const UInt32 kMask = ~kFlag;

struct CTableItem
{
  UInt32 Offset;
  UInt32 ID;
};


const UInt32 kBmpHeaderSize = 14;
const UInt32 kIconHeaderSize = 22;

struct CResItem
{
  UInt32 Type;
  UInt32 ID;
  UInt32 Lang;

  UInt32 Size;
  UInt32 Offset;

  UInt32 HeaderSize;
  Byte Header[kIconHeaderSize]; // it must be enough for max size header.
  bool Enabled;

  bool IsNameEqual(const CResItem &item) const { return Lang == item.Lang; }
  UInt32 GetSize() const { return Size + HeaderSize; }
  bool IsBmp() const { return Type == 2; }
  bool IsIcon() const { return Type == 3; }
  bool IsString() const { return Type == 6; }
  bool IsRcData() const { return Type == 10; }
  bool IsRcDataOrUnknown() const { return IsRcData() || Type > 64; }
};

struct CStringItem
{
  UInt32 Lang;
  UInt32 Size;
  CByteDynamicBuffer Buf;

  void AddChar(Byte c);
  void AddWChar(UInt16 c);
};

void CStringItem::AddChar(Byte c)
{
  Buf.EnsureCapacity(Size + 2);
  Buf[Size++] = c;
  Buf[Size++] = 0;
}

void CStringItem::AddWChar(UInt16 c)
{
  if (c == '\n')
  {
    AddChar('\\');
    c = 'n';
  }
  Buf.EnsureCapacity(Size + 2);
  SetUi16(Buf + Size, c);
  Size += 2;
}

struct CMixItem
{
  int SectionIndex;
  int ResourceIndex;
  int StringIndex;

  bool IsSectionItem() const { return ResourceIndex < 0 && StringIndex < 0; };
};

struct CUsedBitmap
{
  CByteBuffer Buf;
public:
  void Alloc(size_t size)
  {
    size = (size + 7) / 8;
    Buf.SetCapacity(size);
    memset(Buf, 0, size);
  }
  void Free()
  {
    Buf.SetCapacity(0);
  }
  bool SetRange(size_t from, int size)
  {
    for (int i = 0; i < size; i++)
    {
      size_t pos = (from + i) >> 3;
      Byte mask = (Byte)(1 << ((from + i) & 7));
      Byte b = Buf[pos];
      if ((b & mask) != 0)
        return false;
      Buf[pos] = b | mask;
    }
    return true;
  }
};
 

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  CObjectVector<CSection> _sections;
  UInt32 _peOffset;
  CHeader _header;
  COptHeader _optHeader;
  UInt32 _totalSize;
  UInt32 _totalSizeLimited;
  Int32 _mainSubfile;

  CRecordVector<CResItem> _items;
  CObjectVector<CStringItem> _strings;

  CByteBuffer _buf;
  bool _oneLang;
  UString _resourceFileName;
  CUsedBitmap _usedRes;
  bool _parseResources;

  CRecordVector<CMixItem> _mixItems;

  HRESULT LoadDebugSections(IInStream *stream, bool &thereIsSection);
  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *callback);
  bool Parse(const Byte *buf, UInt32 size);

  void AddResNameToString(UString &s, UInt32 id) const;
  UString GetLangPrefix(UInt32 lang);
  HRESULT ReadString(UInt32 offset, UString &dest) const;
  HRESULT ReadTable(UInt32 offset, CRecordVector<CTableItem> &items);
  bool ParseStringRes(UInt32 id, UInt32 lang, const Byte *src, UInt32 size);
  HRESULT OpenResources(int sectIndex, IInStream *stream, IArchiveOpenCallback *callback);
  void CloseResources();


  bool CheckItem(const CSection &sect, const CResItem &item, size_t offset) const
  {
    return item.Offset >= sect.Va && offset <= _buf.GetCapacity() && _buf.GetCapacity() - offset >= item.Size;
  }

public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

bool CHandler::Parse(const Byte *buf, UInt32 size)
{
  UInt32 i;
  if (size < 512)
    return false;
  _peOffset = Get32(buf + 0x3C);
  if (_peOffset >= 0x1000 || _peOffset + 512 > size || (_peOffset & 7) != 0)
    return false;

  UInt32 pos = _peOffset;
  if (!_header.Parse(buf + pos))
    return false;
  if (_header.OptHeaderSize > 512 || _header.NumSections > NUM_SCAN_SECTIONS_MAX)
    return false;
  pos += kHeaderSize;

  if (!_optHeader.Parse(buf + pos, _header.OptHeaderSize))
    return false;

  pos += _header.OptHeaderSize;
  _totalSize = pos;

  for (i = 0; i < _header.NumSections; i++, pos += kSectionSize)
  {
    CSection sect;
    if (pos + kSectionSize > size)
      return false;
    sect.Parse(buf + pos);
    sect.IsRealSect = true;
    sect.UpdateTotalSize(_totalSize);
    _sections.Add(sect);
  }

  return true;
}

enum
{
  kpidSectAlign = kpidUserDefined,
  kpidFileAlign,
  kpidLinkerVer,
  kpidOsVer,
  kpidImageVer,
  kpidSubsysVer,
  kpidCodeSize,
  kpidImageSize,
  kpidInitDataSize,
  kpidUnInitDataSize,
  kpidHeadersSizeUnInitDataSize,
  kpidSubSystem,
  kpidDllCharacts,
  kpidStackReserve,
  kpidStackCommit,
  kpidHeapReserve,
  kpidHeapCommit,
  kpidImageBase
  // kpidAddressOfEntryPoint,
  // kpidBaseOfCode,
  // kpidBaseOfData32,
};

STATPROPSTG kArcProps[] =
{
  { NULL, kpidCpu, VT_BSTR},
  { NULL, kpidBit64, VT_BOOL},
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidPhySize, VT_UI4},
  { NULL, kpidHeadersSize, VT_UI4},
  { NULL, kpidChecksum, VT_UI4},
  { L"Image Size", kpidImageSize, VT_UI4},
  { L"Section Alignment", kpidSectAlign, VT_UI4},
  { L"File Alignment", kpidFileAlign, VT_UI4},
  { L"Code Size", kpidCodeSize, VT_UI4},
  { L"Initialized Data Size", kpidInitDataSize, VT_UI4},
  { L"Uninitialized Data Size", kpidUnInitDataSize, VT_UI4},
  { L"Linker Version", kpidLinkerVer, VT_BSTR},
  { L"OS Version", kpidOsVer, VT_BSTR},
  { L"Image Version", kpidImageVer, VT_BSTR},
  { L"Subsystem Version", kpidSubsysVer, VT_BSTR},
  { L"Subsystem", kpidSubSystem, VT_BSTR},
  { L"DLL Characteristics", kpidDllCharacts, VT_BSTR},
  { L"Stack Reserve", kpidStackReserve, VT_UI8},
  { L"Stack Commit", kpidStackCommit, VT_UI8},
  { L"Heap Reserve", kpidHeapReserve, VT_UI8},
  { L"Heap Commit", kpidHeapCommit, VT_UI8},
  { L"Image Base", kpidImageBase, VT_UI8}
  // { L"Address Of Entry Point", kpidAddressOfEntryPoint, VT_UI8},
  // { L"Base Of Code", kpidBaseOfCode, VT_UI8},
  // { L"Base Of Data", kpidBaseOfData32, VT_UI8},
};

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidOffset, VT_UI8},
  { NULL, kpidVa, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

static void VerToProp(const CVersion &v, NCOM::CPropVariant &prop)
{
  StringToProp(v.GetString(), prop);
}

void TimeToProp(UInt32 unixTime, NCOM::CPropVariant &prop)
{
  if (unixTime != 0)
  {
    FILETIME ft;
    NTime::UnixTimeToFileTime(unixTime, ft);
    prop = ft;
  }
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidSectAlign: prop = _optHeader.SectAlign; break;
    case kpidFileAlign: prop = _optHeader.FileAlign; break;
    case kpidLinkerVer:
    {
      CVersion v = { _optHeader.LinkerVerMajor, _optHeader.LinkerVerMinor };
      VerToProp(v, prop);
      break;
    }
  
    case kpidOsVer: VerToProp(_optHeader.OsVer, prop); break;
    case kpidImageVer: VerToProp(_optHeader.ImageVer, prop); break;
    case kpidSubsysVer: VerToProp(_optHeader.SubsysVer, prop); break;
    case kpidCodeSize: prop = _optHeader.CodeSize; break;
    case kpidInitDataSize: prop = _optHeader.InitDataSize; break;
    case kpidUnInitDataSize: prop = _optHeader.UninitDataSize; break;
    case kpidImageSize: prop = _optHeader.ImageSize; break;
    case kpidPhySize: prop = _totalSize; break;
    case kpidHeadersSize: prop = _optHeader.HeadersSize; break;
    case kpidChecksum: prop = _optHeader.CheckSum; break;
      
    case kpidCpu: PAIR_TO_PROP(g_MachinePairs, _header.Machine, prop); break;
    case kpidBit64: if (_optHeader.Is64Bit()) prop = true; break;
    case kpidSubSystem: PAIR_TO_PROP(g_SubSystems, _optHeader.SubSystem, prop); break;

    case kpidMTime:
    case kpidCTime: TimeToProp(_header.Time, prop); break;
    case kpidCharacts: FLAGS_TO_PROP(g_HeaderCharacts, _header.Flags, prop); break;
    case kpidDllCharacts: FLAGS_TO_PROP(g_DllCharacts, _optHeader.DllCharacts, prop); break;
    case kpidStackReserve: prop = _optHeader.StackReserve; break;
    case kpidStackCommit: prop = _optHeader.StackCommit; break;
    case kpidHeapReserve: prop = _optHeader.HeapReserve; break;
    case kpidHeapCommit: prop = _optHeader.HeapCommit; break;

    case kpidImageBase: prop = _optHeader.ImageBase; break;
    // case kpidAddressOfEntryPoint: prop = _optHeader.AddressOfEntryPoint; break;
    // case kpidBaseOfCode: prop = _optHeader.BaseOfCode; break;
    // case kpidBaseOfData32: if (!_optHeader.Is64Bit()) prop = _optHeader.BaseOfData32; break;

    case kpidMainSubfile: if (_mainSubfile >= 0) prop = (UInt32)_mainSubfile; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

void CHandler::AddResNameToString(UString &s, UInt32 id) const
{
  if ((id & kFlag) != 0)
  {
    UString name;
    if (ReadString(id & kMask, name) == S_OK)
    {
      if (name.IsEmpty())
        s += L"[]";
      else
      {
        if (name.Length() > 1 && name[0] == '"' && name.Back() == '"')
          name = name.Mid(1, name.Length() - 2);
        s += name;
      }
      return;
    }
  }
  wchar_t sz[32];
  ConvertUInt32ToString(id, sz);
  s += sz;
}

UString CHandler::GetLangPrefix(UInt32 lang)
{
  UString s = _resourceFileName;
  s += WCHAR_PATH_SEPARATOR;
  if (!_oneLang)
  {
    AddResNameToString(s, lang);
    s += WCHAR_PATH_SEPARATOR;
  }
  return s;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CMixItem &mixItem = _mixItems[index];
  if (mixItem.StringIndex >= 0)
  {
    const CStringItem &item = _strings[mixItem.StringIndex];
    switch(propID)
    {
      case kpidPath: prop = GetLangPrefix(item.Lang) + L"string.txt"; break;
      case kpidSize:
      case kpidPackSize:
        prop = (UInt64)item.Size; break;
    }
  }
  else if (mixItem.ResourceIndex < 0)
  {
    const CSection &item = _sections[mixItem.SectionIndex];
    switch(propID)
    {
      case kpidPath: StringToProp(item.Name, prop); break;
      case kpidSize: prop = (UInt64)item.VSize; break;
      case kpidPackSize: prop = (UInt64)item.GetPackSize(); break;
      case kpidOffset: prop = item.Pa; break;
      case kpidVa: if (item.IsRealSect) prop = item.Va; break;
      case kpidMTime:
      case kpidCTime:
        TimeToProp(item.IsDebug ? item.Time : _header.Time, prop); break;
      case kpidCharacts: if (item.IsRealSect) FLAGS_TO_PROP(g_SectFlags, item.Flags, prop); break;
    }
  }
  else
  {
    const CResItem &item = _items[mixItem.ResourceIndex];
    switch(propID)
    {
      case kpidPath:
      {
        UString s = GetLangPrefix(item.Lang);
        {
          const wchar_t *p = NULL;
          if (item.Type < sizeof(g_ResTypes) / sizeof(g_ResTypes[0]))
            p = g_ResTypes[item.Type];
          if (p != 0)
            s += p;
          else
            AddResNameToString(s, item.Type);
        }
        s += WCHAR_PATH_SEPARATOR;
        AddResNameToString(s, item.ID);
        if (item.HeaderSize != 0)
        {
          if (item.IsBmp())
            s += L".bmp";
          else if (item.IsIcon())
            s += L".ico";
        }
        prop = s;
        break;
      }
      case kpidSize: prop = (UInt64)item.GetSize(); break;
      case kpidPackSize: prop = (UInt64)item.Size; break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::LoadDebugSections(IInStream *stream, bool &thereIsSection)
{
  thereIsSection = false;
  const CDirLink &debugLink = _optHeader.DirItems[kDirLink_Debug];
  if (debugLink.Size == 0)
    return S_OK;
  const unsigned kEntrySize = 28;
  UInt32 numItems = debugLink.Size / kEntrySize;
  if (numItems * kEntrySize != debugLink.Size || numItems > 16)
    return S_FALSE;
  
  UInt64 pa = 0;
  int i;
  for (i = 0; i < _sections.Size(); i++)
  {
    const CSection &sect = _sections[i];
    if (sect.Va < debugLink.Va && debugLink.Va + debugLink.Size <= sect.Va + sect.PSize)
    {
      pa = sect.Pa + (debugLink.Va - sect.Va);
      break;
    }
  }
  if (i == _sections.Size())
  {
    return S_OK;
    // Exe for ARM requires S_OK
    // return S_FALSE;
  }
  
  CByteBuffer buffer;
  buffer.SetCapacity(debugLink.Size);
  Byte *buf = buffer;
  
  RINOK(stream->Seek(pa, STREAM_SEEK_SET, NULL));
  RINOK(ReadStream_FALSE(stream, buf, debugLink.Size));

  for (i = 0; i < (int)numItems; i++)
  {
    CDebugEntry de;
    de.Parse(buf);

    if (de.Size == 0)
      continue;
    
    CSection sect;
    sect.Name = ".debug" + GetDecString(i);
    
    sect.IsDebug = true;
    sect.Time = de.Time;
    sect.Va = de.Va;
    sect.Pa = de.Pa;
    sect.PSize = sect.VSize = de.Size;
    UInt32 totalSize = sect.Pa + sect.PSize;
    if (totalSize > _totalSize)
    {
      _totalSize = totalSize;
      _sections.Add(sect);
      thereIsSection = true;
    }
    buf += kEntrySize;
  }

  return S_OK;
}

HRESULT CHandler::ReadString(UInt32 offset, UString &dest) const
{
  if ((offset & 1) != 0 || offset >= _buf.GetCapacity())
    return S_FALSE;
  size_t rem = _buf.GetCapacity() - offset;
  if (rem < 2)
    return S_FALSE;
  unsigned length = Get16(_buf + offset);
  if ((rem - 2) / 2 < length)
    return S_FALSE;
  dest.Empty();
  offset += 2;
  for (unsigned i = 0; i < length; i++)
    dest += (wchar_t)Get16(_buf + offset + i * 2);
  return S_OK;
}

HRESULT CHandler::ReadTable(UInt32 offset, CRecordVector<CTableItem> &items)
{
  if ((offset & 3) != 0 || offset >= _buf.GetCapacity())
    return S_FALSE;
  size_t rem = _buf.GetCapacity() - offset;
  if (rem < 16)
    return S_FALSE;
  items.Clear();
  unsigned numNameItems = Get16(_buf + offset + 12);
  unsigned numIdItems = Get16(_buf + offset + 14);
  unsigned numItems = numNameItems + numIdItems;
  if ((rem - 16) / 8 < numItems)
    return S_FALSE;
  if (!_usedRes.SetRange(offset, 16 + numItems * 8))
    return S_FALSE;
  offset += 16;
  _oneLang = true;
  unsigned i;
  for (i = 0; i < numItems; i++)
  {
    CTableItem item;
    const Byte *buf = _buf + offset;
    offset += 8;
    item.ID = Get32(buf + 0);
    if (((item.ID & kFlag) != 0) != (i < numNameItems))
      return S_FALSE;
    item.Offset = Get32(buf + 4);
    items.Add(item);
  }
  return S_OK;
}

static const UInt32 kFileSizeMax = (UInt32)1 << 30;
static const int kNumResItemsMax = (UInt32)1 << 23;
static const int kNumStringLangsMax = 128;

// BITMAPINFOHEADER
struct CBitmapInfoHeader
{
  // UInt32 HeaderSize;
  UInt32 XSize;
  Int32 YSize;
  UInt16 Planes;
  UInt16 BitCount;
  UInt32 Compression;
  UInt32 SizeImage;

  bool Parse(const Byte *p, size_t size);
};

static const UInt32 kBitmapInfoHeader_Size = 0x28;

bool CBitmapInfoHeader::Parse(const Byte *p, size_t size)
{
  if (size < kBitmapInfoHeader_Size || Get32(p) != kBitmapInfoHeader_Size)
    return false;
  XSize = Get32(p + 4);
  YSize = (Int32)Get32(p + 8);
  Planes = Get16(p + 12);
  BitCount = Get16(p + 14);
  Compression = Get32(p + 16);
  SizeImage = Get32(p + 20);
  return true;
}

static UInt32 GetImageSize(UInt32 xSize, UInt32 ySize, UInt32 bitCount)
{
  return ((xSize * bitCount + 7) / 8 + 3) / 4 * 4 * ySize;
}
  
static UInt32 SetBitmapHeader(Byte *dest, const Byte *src, UInt32 size)
{
  CBitmapInfoHeader h;
  if (!h.Parse(src, size))
    return 0;
  if (h.YSize < 0)
    h.YSize = -h.YSize;
  if (h.XSize > (1 << 26) || h.YSize > (1 << 26) || h.Planes != 1 || h.BitCount > 32 ||
      h.Compression != 0) // BI_RGB
    return 0;
  if (h.SizeImage == 0)
    h.SizeImage = GetImageSize(h.XSize, h.YSize, h.BitCount);
  UInt32 totalSize = kBmpHeaderSize + size;
  UInt32 offBits = totalSize - h.SizeImage;
  // BITMAPFILEHEADER
  SetUi16(dest, 0x4D42);
  SetUi32(dest + 2, totalSize);
  SetUi32(dest + 6, 0);
  SetUi32(dest + 10, offBits);
  return kBmpHeaderSize;
}

static UInt32 SetIconHeader(Byte *dest, const Byte *src, UInt32 size)
{
  CBitmapInfoHeader h;
  if (!h.Parse(src, size))
    return 0;
  if (h.YSize < 0)
    h.YSize = -h.YSize;
  if (h.XSize > (1 << 26) || h.YSize > (1 << 26) || h.Planes != 1 ||
      h.Compression != 0) // BI_RGB
    return 0;

  UInt32 numBitCount = h.BitCount;
  if (numBitCount != 1 &&
      numBitCount != 4 &&
      numBitCount != 8 &&
      numBitCount != 24 &&
      numBitCount != 32)
    return 0;

  if ((h.YSize & 1) != 0)
    return 0;
  h.YSize /= 2;
  if (h.XSize > 0x100 || h.YSize > 0x100)
    return 0;

  UInt32 imageSize;
  // imageSize is not correct if AND mask array contains zeros
  // in this case it is equal image1Size

  // UInt32 imageSize = h.SizeImage;
  // if (imageSize == 0)
  // {
    UInt32 image1Size = GetImageSize(h.XSize, h.YSize, h.BitCount);
    UInt32 image2Size = GetImageSize(h.XSize, h.YSize, 1);
    imageSize = image1Size + image2Size;
  // }
  UInt32 numColors = 0;
  if (numBitCount < 16)
    numColors = 1 << numBitCount;

  SetUi16(dest, 0); // Reserved
  SetUi16(dest + 2, 1); // RES_ICON
  SetUi16(dest + 4, 1); // ResCount

  dest[6] = (Byte)h.XSize; // Width
  dest[7] = (Byte)h.YSize; // Height
  dest[8] = (Byte)numColors; // ColorCount
  dest[9] = 0; // Reserved
  
  SetUi32(dest + 10, 0); // Reserved1 / Reserved2

  UInt32 numQuadsBytes = numColors * 4;
  UInt32 BytesInRes = kBitmapInfoHeader_Size + numQuadsBytes + imageSize;
  SetUi32(dest + 14, BytesInRes);
  SetUi32(dest + 18, kIconHeaderSize);

  /*
  Description = DWORDToString(xSize) +
      kDelimiterChar + DWORDToString(ySize) +
      kDelimiterChar + DWORDToString(numBitCount);
  */
  return kIconHeaderSize;
}

bool CHandler::ParseStringRes(UInt32 id, UInt32 lang, const Byte *src, UInt32 size)
{
  if ((size & 1) != 0)
    return false;

  int i;
  for (i = 0; i < _strings.Size(); i++)
    if (_strings[i].Lang == lang)
      break;
  if (i == _strings.Size())
  {
    if (_strings.Size() >= kNumStringLangsMax)
      return false;
    CStringItem item;
    item.Size = 0;
    item.Lang = lang;
    i = _strings.Add(item);
  }
  
  CStringItem &item = _strings[i];
  id = (id - 1) << 4;
  UInt32 pos = 0;
  for (i = 0; i < 16; i++)
  {
    if (size - pos < 2)
      return false;
    UInt32 len = Get16(src + pos);
    pos += 2;
    if (len != 0)
    {
      if (size - pos < len * 2)
        return false;
      char temp[32];
      ConvertUInt32ToString(id  + i, temp);
      size_t tempLen = strlen(temp);
      size_t j;
      for (j = 0; j < tempLen; j++)
        item.AddChar(temp[j]);
      item.AddChar('\t');
      for (j = 0; j < len; j++, pos += 2)
        item.AddWChar(Get16(src + pos));
      item.AddChar(0x0D);
      item.AddChar(0x0A);
    }
  }
  return (size == pos);
}

HRESULT CHandler::OpenResources(int sectionIndex, IInStream *stream, IArchiveOpenCallback *callback)
{
  const CSection &sect = _sections[sectionIndex];
  size_t fileSize = sect.PSize; // Maybe we need sect.VSize here !!!
  if (fileSize > kFileSizeMax)
    return S_FALSE;
  {
    UInt64 fileSize64 = fileSize;
    if (callback)
      RINOK(callback->SetTotal(NULL, &fileSize64));
    RINOK(stream->Seek(sect.Pa, STREAM_SEEK_SET, NULL));
    _buf.SetCapacity(fileSize);
    for (size_t pos = 0; pos < fileSize;)
    {
      UInt64 offset64 = pos;
      if (callback)
        RINOK(callback->SetCompleted(NULL, &offset64))
      size_t rem = MyMin(fileSize - pos, (size_t)(1 << 20));
      RINOK(ReadStream_FALSE(stream, _buf + pos, rem));
      pos += rem;
    }
  }
  
  _usedRes.Alloc(fileSize);
  CRecordVector<CTableItem> specItems;
  RINOK(ReadTable(0, specItems));

  _oneLang = true;
  bool stringsOk = true;
  size_t maxOffset = 0;
  for (int i = 0; i < specItems.Size(); i++)
  {
    const CTableItem &item1 = specItems[i];
    if ((item1.Offset & kFlag) == 0)
      return S_FALSE;

    CRecordVector<CTableItem> specItems2;
    RINOK(ReadTable(item1.Offset & kMask, specItems2));

    for (int j = 0; j < specItems2.Size(); j++)
    {
      const CTableItem &item2 = specItems2[j];
      if ((item2.Offset & kFlag) == 0)
        return S_FALSE;
      
      CRecordVector<CTableItem> specItems3;
      RINOK(ReadTable(item2.Offset & kMask, specItems3));
      
      CResItem item;
      item.Type = item1.ID;
      item.ID = item2.ID;
      
      for (int k = 0; k < specItems3.Size(); k++)
      {
        if (_items.Size() >= kNumResItemsMax)
          return S_FALSE;
        const CTableItem &item3 = specItems3[k];
        if ((item3.Offset & kFlag) != 0)
          return S_FALSE;
        if (item3.Offset >= _buf.GetCapacity() || _buf.GetCapacity() - item3.Offset < 16)
          return S_FALSE;
        const Byte *buf = _buf + item3.Offset;
        item.Lang = item3.ID;
        item.Offset = Get32(buf + 0);
        item.Size = Get32(buf + 4);
        // UInt32 codePage = Get32(buf + 8);
        if (Get32(buf + 12) != 0)
          return S_FALSE;
        if (!_items.IsEmpty() && _oneLang && !item.IsNameEqual(_items.Back()))
          _oneLang = false;

        item.HeaderSize = 0;
      
        size_t offset = item.Offset - sect.Va;
        if (offset > maxOffset)
          maxOffset = offset;
        if (offset + item.Size > maxOffset)
          maxOffset = offset + item.Size;

        if (CheckItem(sect, item, offset))
        {
          const Byte *data = _buf + offset;
          if (item.IsBmp())
            item.HeaderSize = SetBitmapHeader(item.Header, data, item.Size);
          else if (item.IsIcon())
            item.HeaderSize = SetIconHeader(item.Header, data, item.Size);
          else if (item.IsString())
          {
            if (stringsOk)
              stringsOk = ParseStringRes(item.ID, item.Lang, data, item.Size);
          }
        }

        item.Enabled = true;
        _items.Add(item);
      }
    }
  }
  
  if (stringsOk && !_strings.IsEmpty())
  {
    int i;
    for (i = 0; i < _items.Size(); i++)
    {
      CResItem &item = _items[i];
      if (item.IsString())
        item.Enabled = false;
    }
    for (i = 0; i < _strings.Size(); i++)
    {
      if (_strings[i].Size == 0)
        continue;
      CMixItem mixItem;
      mixItem.ResourceIndex = -1;
      mixItem.StringIndex = i;
      mixItem.SectionIndex = sectionIndex;
      _mixItems.Add(mixItem);
    }
  }

  _usedRes.Free();

  int numBits = _optHeader.GetNumFileAlignBits();
  if (numBits >= 0)
  {
    UInt32 mask = (1 << numBits) - 1;
    size_t end = ((maxOffset + mask) & ~mask);
    if (end < sect.VSize && end <= sect.PSize)
    {
      CSection sect2;
      sect2.Flags = 0;

      // we skip Zeros to start of aligned block
      size_t i;
      for (i = maxOffset; i < end; i++)
        if (_buf[i] != 0)
          break;
      if (i == end)
        maxOffset = end;
      
      sect2.Pa = sect.Pa + (UInt32)maxOffset;
      sect2.Va = sect.Va + (UInt32)maxOffset;
      sect2.PSize = sect.VSize - (UInt32)maxOffset;
      sect2.VSize = sect2.PSize;
      sect2.Name = ".rsrc_1";
      sect2.Time = 0;
      sect2.IsAdditionalSection = true;
      _sections.Add(sect2);
    }
  }

  return S_OK;
}

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  const UInt32 kBufSize = 1 << 18;
  const UInt32 kSigSize = 2;

  _mainSubfile = -1;

  CByteBuffer buffer;
  buffer.SetCapacity(kBufSize);
  Byte *buf = buffer;

  size_t processed = kSigSize;
  RINOK(ReadStream_FALSE(stream, buf, processed));
  if (buf[0] != 'M' || buf[1] != 'Z')
    return S_FALSE;
  processed = kBufSize - kSigSize;
  RINOK(ReadStream(stream, buf + kSigSize, &processed));
  processed += kSigSize;
  if (!Parse(buf, (UInt32)processed))
    return S_FALSE;
  bool thereISDebug;
  RINOK(LoadDebugSections(stream, thereISDebug));

  const CDirLink &certLink = _optHeader.DirItems[kDirLink_Certificate];
  if (certLink.Size != 0)
  {
    CSection sect;
    sect.Name = "CERTIFICATE";
    sect.Va = 0;
    sect.Pa = certLink.Va;
    sect.PSize = sect.VSize = certLink.Size;
    sect.UpdateTotalSize(_totalSize);
    _sections.Add(sect);
  }

  if (thereISDebug)
  {
    const UInt32 kAlign = 1 << 12;
    UInt32 alignPos = _totalSize & (kAlign - 1);
    if (alignPos != 0)
    {
      UInt32 size = kAlign - alignPos;
      RINOK(stream->Seek(_totalSize, STREAM_SEEK_SET, NULL));
      buffer.Free();
      buffer.SetCapacity(kAlign);
      Byte *buf = buffer;
      size_t processed = size;
      RINOK(ReadStream(stream, buf, &processed));
      size_t i;
      for (i = 0; i < processed; i++)
      {
        if (buf[i] != 0)
          break;
      }
      if (processed < size && processed < 100)
        _totalSize += (UInt32)processed;
      else if (((_totalSize + i) & 0x1FF) == 0 || processed < size)
        _totalSize += (UInt32)i;
    }
  }

  if (_header.NumSymbols > 0 && _header.PointerToSymbolTable >= 512)
  {
    if (_header.NumSymbols >= (1 << 24))
      return S_FALSE;
    CSection sect;
    sect.Name = "COFF_SYMBOLS";
    UInt32 size = _header.NumSymbols * 18;
    RINOK(stream->Seek((UInt64)_header.PointerToSymbolTable + size, STREAM_SEEK_SET, NULL));
    Byte buf[4];
    RINOK(ReadStream_FALSE(stream, buf, 4));
    UInt32 size2 = Get32(buf);
    if (size2 >= (1 << 28))
      return S_FALSE;
    size += size2;

    sect.Va = 0;
    sect.Pa = _header.PointerToSymbolTable;
    sect.PSize = sect.VSize = size;
    sect.UpdateTotalSize(_totalSize);
    _sections.Add(sect);
  }

  UInt64 fileSize;
  RINOK(stream->Seek(0, STREAM_SEEK_END, &fileSize));
  if (fileSize > _totalSize)
    return S_FALSE;
  _totalSizeLimited = (_totalSize < fileSize) ? _totalSize : (UInt32)fileSize;

  {
    CObjectVector<CSection> sections = _sections;
    sections.Sort();
    UInt32 limit = (1 << 12);
    int num = 0;
    int numSections = sections.Size();
    for (int i = 0; i < numSections; i++)
    {
      const CSection &s = sections[i];
      if (s.Pa > limit)
      {
        CSection s2;
        s2.Pa = s2.Va = limit;
        s2.PSize = s2.VSize = s.Pa - limit;
        s2.IsAdditionalSection = true;
        s2.Name = '[';
        s2.Name += GetDecString(num++);
        s2.Name += ']';
        _sections.Add(s2);
        limit = s.Pa;
      }
      UInt32 next = s.Pa + s.PSize;
      if (next < s.Pa)
        break;
      if (next >= limit)
        limit = next;
    }
  }

  _parseResources = true;

  UInt64 mainSize = 0, mainSize2 = 0;
  int i;
  for (i = 0; i < _sections.Size(); i++)
  {
    const CSection &sect = _sections[i];
    CMixItem mixItem;
    mixItem.SectionIndex = i;
    if (_parseResources && sect.Name == ".rsrc" && _items.IsEmpty())
    {
      HRESULT res = OpenResources(i, stream, callback);
      if (res == S_OK)
      {
        _resourceFileName = GetUnicodeString(sect.Name);
        for (int j = 0; j < _items.Size(); j++)
        {
          const CResItem &item = _items[j];
          if (item.Enabled)
          {
            mixItem.ResourceIndex = j;
            mixItem.StringIndex = -1;
            if (item.IsRcDataOrUnknown())
            {
              if (item.Size >= mainSize)
              {
                mainSize2 = mainSize;
                mainSize = item.Size;
                _mainSubfile = _mixItems.Size();
              }
              else if (item.Size >= mainSize2)
                mainSize2 = item.Size;
            }
            _mixItems.Add(mixItem);
          }
        }
        if (sect.PSize > sect.VSize)
        {
          int numBits = _optHeader.GetNumFileAlignBits();
          if (numBits >= 0)
          {
            UInt32 mask = (1 << numBits) - 1;
            UInt32 end = ((sect.VSize + mask) & ~mask);

            if (sect.PSize > end)
            {
              CSection sect2;
              sect2.Flags = 0;
              sect2.Pa = sect.Pa + end;
              sect2.Va = sect.Va + end;
              sect2.PSize = sect.PSize - end;
              sect2.VSize = sect2.PSize;
              sect2.Name = ".rsrc_2";
              sect2.Time = 0;
              sect2.IsAdditionalSection = true;
              _sections.Add(sect2);
            }
          }
        }
        continue;
      }
      if (res != S_FALSE)
        return res;
      CloseResources();
    }
    mixItem.StringIndex = -1;
    mixItem.ResourceIndex = -1;
    if (sect.IsAdditionalSection)
    {
      if (sect.PSize >= mainSize)
      {
        mainSize2 = mainSize;
        mainSize = sect.PSize;
        _mainSubfile = _mixItems.Size();
      }
      else
        mainSize2 = sect.PSize;
    }
    _mixItems.Add(mixItem);
  }
  
  if (mainSize2 >= (1 << 20) && mainSize < mainSize2 * 2)
    _mainSubfile = -1;

  for (i = 0; i < _mixItems.Size(); i++)
  {
    const CMixItem &mixItem = _mixItems[i];
    if (mixItem.StringIndex < 0 && mixItem.ResourceIndex < 0 && _sections[mixItem.SectionIndex].Name == "_winzip_")
    {
      _mainSubfile = i;
      break;
    }
  }

  return S_OK;
}

HRESULT CalcCheckSum(ISequentialInStream *stream, UInt32 size, UInt32 excludePos, UInt32 &res)
{
  // size &= ~1;
  const UInt32 kBufSize = 1 << 23;
  CByteBuffer buffer;
  buffer.SetCapacity(kBufSize);
  Byte *buf = buffer;

  UInt32 sum = 0;
  UInt32 pos = 0;
  for (;;)
  {
    UInt32 rem = size - pos;
    if (rem > kBufSize)
      rem = kBufSize;
    if (rem == 0)
      break;
    size_t processed = rem;
    RINOK(ReadStream(stream, buf, &processed));
    
    /*
    for (; processed < rem; processed++)
      buf[processed] = 0;
    */

    if ((processed & 1) != 0)
      buf[processed] = 0;

    for (int j = 0; j < 4; j++)
    {
      UInt32 p = excludePos + j;
      if (pos <= p && p < pos + processed)
        buf[p - pos] = 0;
    }

    for (size_t i = 0; i < processed; i += 2)
    {
      sum += Get16(buf + i);
      sum = (sum + (sum >> 16)) & 0xFFFF;
    }
    pos += (UInt32)processed;
    if (rem != processed)
      break;
  }
  sum += pos;
  res = sum;
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(inStream, callback));
  _stream = inStream;
  return S_OK;
  COM_TRY_END
}

void CHandler::CloseResources()
{
  _usedRes.Free();
  _items.Clear();
  _strings.Clear();
  _buf.SetCapacity(0);
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  _sections.Clear();
  _mixItems.Clear();
  CloseResources();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _mixItems.Size();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _mixItems.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const CMixItem &mixItem = _mixItems[allFilesMode ? i : indices[i]];
    if (mixItem.StringIndex >= 0)
      totalSize += _strings[mixItem.StringIndex].Size;
    else if (mixItem.ResourceIndex < 0)
      totalSize += _sections[mixItem.SectionIndex].GetPackSize();
    else
      totalSize += _items[mixItem.ResourceIndex].GetSize();
  }
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  bool checkSumOK = true;
  if (_optHeader.CheckSum != 0 && (int)numItems == _mixItems.Size())
  {
    UInt32 checkSum = 0;
    RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
    CalcCheckSum(_stream, _totalSizeLimited, _peOffset + kHeaderSize + 64, checkSum);
    checkSumOK = (checkSum == _optHeader.CheckSum);
  }

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_stream);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];

    CMyComPtr<ISequentialOutStream> outStream;
    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    const CMixItem &mixItem = _mixItems[index];

    const CSection &sect = _sections[mixItem.SectionIndex];
    bool isOk = true;
    if (mixItem.StringIndex >= 0)
    {
      const CStringItem &item = _strings[mixItem.StringIndex];
      currentItemSize = item.Size;
      if (!testMode && !outStream)
        continue;

      RINOK(extractCallback->PrepareOperation(askMode));
      if (outStream)
        RINOK(WriteStream(outStream, item.Buf, item.Size));
    }
    else if (mixItem.ResourceIndex < 0)
    {
      currentItemSize = sect.GetPackSize();
      if (!testMode && !outStream)
        continue;
      
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(_stream->Seek(sect.Pa, STREAM_SEEK_SET, NULL));
      streamSpec->Init(currentItemSize);
      RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
      isOk = (copyCoderSpec->TotalSize == currentItemSize);
    }
    else
    {
      const CResItem &item = _items[mixItem.ResourceIndex];
      currentItemSize = item.GetSize();
      if (!testMode && !outStream)
        continue;

      RINOK(extractCallback->PrepareOperation(askMode));
      size_t offset = item.Offset - sect.Va;
      if (!CheckItem(sect, item, offset))
        isOk = false;
      else if (outStream)
      {
        if (item.HeaderSize != 0)
          RINOK(WriteStream(outStream, item.Header, item.HeaderSize));
        RINOK(WriteStream(outStream, _buf + offset, item.Size));
      }
    }
    
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(isOk ?
      checkSumOK ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kCRCError:
        NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  *stream = 0;

  const CMixItem &mixItem = _mixItems[index];
  const CSection &sect = _sections[mixItem.SectionIndex];
  if (mixItem.IsSectionItem())
    return CreateLimitedInStream(_stream, sect.Pa, sect.PSize, stream);

  CBufInStream *inStreamSpec = new CBufInStream;
  CMyComPtr<ISequentialInStream> streamTemp = inStreamSpec;
  CReferenceBuf *referenceBuf = new CReferenceBuf;
  CMyComPtr<IUnknown> ref = referenceBuf;
  if (mixItem.StringIndex >= 0)
  {
    const CStringItem &item = _strings[mixItem.StringIndex];
    referenceBuf->Buf.SetCapacity(item.Size);
    memcpy(referenceBuf->Buf, item.Buf, item.Size);
  }
  else
  {
    const CResItem &item = _items[mixItem.ResourceIndex];
    size_t offset = item.Offset - sect.Va;
    if (!CheckItem(sect, item, offset))
      return S_FALSE;
    if (item.HeaderSize == 0)
    {
      CBufInStream *streamSpec = new CBufInStream;
      CMyComPtr<IInStream> streamTemp2 = streamSpec;
      streamSpec->Init(_buf + offset, item.Size, (IInArchive *)this);
      *stream = streamTemp2.Detach();
      return S_OK;
    }
    referenceBuf->Buf.SetCapacity(item.HeaderSize + item.Size);
    memcpy(referenceBuf->Buf, item.Header, item.HeaderSize);
    memcpy(referenceBuf->Buf + item.HeaderSize, _buf + offset, item.Size);
  }
  inStreamSpec->Init(referenceBuf);

  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"PE", L"exe dll sys", 0, 0xDD, { 'P', 'E', 0, 0 }, 4, false, CreateArc, 0 };

REGISTER_ARC(Pe)

}}
