// PeHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/PropVariantUtils.h"
#include "Windows/Time.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/Copy/CopyCoder.h"

#include "Common/DummyOutStream.h"

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
};

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
  // UInt64 ImageBase;

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
  // BaseOfData32 = Get32(p + 24);
  // ImageBase = hdr64 ? GetUi64(p + 24) :Get32(p + 28);

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

  CSection(): IsRealSect(false), IsDebug(false) {}
  UInt64 GetPackSize() const { return PSize; }

  void UpdateTotalSize(UInt32 &totalSize)
  {
    UInt32 t = Pa + PSize;
    if (t > totalSize)
      totalSize = t;
  }
  void Parse(const Byte *p);
};

static bool operator <(const CSection &a1, const CSection &a2) { return (a1.Pa < a2.Pa); }
static bool operator ==(const CSection &a1, const CSection &a2) { return (a1.Pa == a2.Pa); }

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
  { 1 << 1, "Executable" },
  { 1 << 13, "DLL" },
  { 1 << 8, "32-bit" },
  { 1 << 5, "LargeAddress" },
  { 1 << 0, "NoRelocs" },
  { 1 << 2, "NoLineNums" },
  { 1 << 3, "NoLocalSyms" },
  { 1 << 4, "AggressiveWsTrim" },
  { 1 << 9, "NoDebugInfo" },
  { 1 << 10, "RemovableRun" },
  { 1 << 11, "NetRun" },
  { 1 << 12, "System" },
  { 1 << 14, "UniCPU" },
  { 1 << 7, "Little-Endian" },
  { 1 << 15, "Big-Endian" }
};

static const CUInt32PCharPair g_DllCharacts[] =
{
  { 1 << 6, "Relocated" },
  { 1 << 7, "Integrity" },
  { 1 << 8, "NX-Compatible" },
  { 1 << 9, "NoIsolation" },
  { 1 << 10, "NoSEH" },
  { 1 << 11, "NoBind" },
  { 1 << 13, "WDM" },
  { 1 << 15, "TerminalServerAware" }
};

static const CUInt32PCharPair g_SectFlags[] =
{
  { 1 << 3, "NoPad" },
  { 1 << 5, "Code" },
  { 1 << 6, "InitializedData" },
  { 1 << 7, "UninitializedData" },
  { 1 << 9, "Comments" },
  { 1 << 11, "Remove" },
  { 1 << 12, "COMDAT" },
  { 1 << 15, "GP" },
  { 1 << 24, "ExtendedRelocations" },
  { 1 << 25, "Discardable" },
  { 1 << 26, "NotCached" },
  { 1 << 27, "NotPaged" },
  { 1 << 28, "Shared" },
  { 1 << 29, "Execute" },
  { 1 << 30, "Read" },
  { (UInt32)1 << 31, "Write" }
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

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _inStream;
  CObjectVector<CSection> _sections;
  UInt32 _peOffset;
  CHeader _header;
  COptHeader _optHeader;
  UInt32 _totalSize;
  UInt32 _totalSizeLimited;
  HRESULT LoadDebugSections(IInStream *stream, bool &thereIsSection);
  HRESULT Open2(IInStream *stream);
  bool Parse(const Byte *buf, UInt32 size);
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
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
      VerToProp(v, prop); break;
      break;
    }
  
    case kpidOsVer:  VerToProp(_optHeader.OsVer, prop); break;
    case kpidImageVer:  VerToProp(_optHeader.ImageVer, prop); break;
    case kpidSubsysVer:  VerToProp(_optHeader.SubsysVer, prop); break;
    case kpidCodeSize:  prop = _optHeader.CodeSize; break;
    case kpidInitDataSize:  prop = _optHeader.InitDataSize; break;
    case kpidUnInitDataSize:  prop = _optHeader.UninitDataSize; break;
    case kpidImageSize:  prop = _optHeader.ImageSize; break;
    case kpidPhySize:  prop = _totalSize; break;
    case kpidHeadersSize:  prop = _optHeader.HeadersSize; break;
    case kpidChecksum:  prop = _optHeader.CheckSum; break;
      
    case kpidCpu:  PAIR_TO_PROP(g_MachinePairs, _header.Machine, prop); break;
    case kpidBit64:  if (_optHeader.Is64Bit()) prop = true; break;
    case kpidSubSystem:  PAIR_TO_PROP(g_SubSystems, _optHeader.SubSystem, prop); break;

    case kpidMTime:
    case kpidCTime:  TimeToProp(_header.Time, prop); break;
    case kpidCharacts:  FLAGS_TO_PROP(g_HeaderCharacts, _header.Flags, prop); break;
    case kpidDllCharacts:  FLAGS_TO_PROP(g_DllCharacts, _optHeader.DllCharacts, prop); break;
    case kpidStackReserve: prop = _optHeader.StackReserve; break;
    case kpidStackCommit: prop = _optHeader.StackCommit; break;
    case kpidHeapReserve: prop = _optHeader.HeapReserve; break;
    case kpidHeapCommit: prop = _optHeader.HeapCommit; break;

    /*
    if (_optHeader.Is64Bit())
      s += " 64-bit";
    */
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CSection &item = _sections[index];
  switch(propID)
  {
    case kpidPath:  StringToProp(item.Name, prop); break;
    case kpidSize:  prop = (UInt64)item.VSize; break;
    case kpidPackSize:  prop = (UInt64)item.GetPackSize(); break;
    case kpidOffset:  prop = item.Pa; break;
    case kpidVa:  if (item.IsRealSect) prop = item.Va; break;
    case kpidMTime:
    case kpidCTime:
      TimeToProp(item.IsDebug ? item.Time : _header.Time, prop); break;
    case kpidCharacts:  if (item.IsRealSect) FLAGS_TO_PROP(g_SectFlags, item.Flags, prop); break;
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
    return S_FALSE;
  
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

HRESULT CHandler::Open2(IInStream *stream)
{
  const UInt32 kBufSize = 1 << 18;
  const UInt32 kSigSize = 2;

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
    for (int i = 0; i < sections.Size(); i++)
    {
      const CSection &s = sections[i];
      if (s.Pa > limit)
      {
        CSection s2;
        s2.Pa = s2.Va = limit;
        s2.PSize = s2.VSize = s.Pa - limit;
        char sz[32];
        ConvertUInt64ToString(++num, sz);
        s2.Name = "[data-";
        s2.Name += sz;
        s2.Name += "]";
        _sections.Add(s2);
      }
      UInt32 next = s.Pa + s.PSize;
      if (next < limit)
        break;
      limit = next;
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
  UInt32 pos;
  for(pos = 0;;)
  {
    UInt32 rem = size - pos;
    if (rem > kBufSize)
      rem = kBufSize;
    if (rem == 0)
      break;
    size_t processed = rem;
    RINOK(ReadStream(stream, buf, &processed));
    
    /*
    */
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

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(inStream));
  _inStream = inStream;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _inStream.Release();
  _sections.Clear();

  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _sections.Size();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (_aTestMode != 0);
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = _sections.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _sections[allFilesMode ? i : indices[i]].GetPackSize();
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  bool checkSumOK = true;
  if (_optHeader.CheckSum != 0 && (int)numItems == _sections.Size())
  {
    UInt32 checkSum = 0;
    RINOK(_inStream->Seek(0, STREAM_SEEK_SET, NULL));
    CalcCheckSum(_inStream, _totalSizeLimited, _peOffset + kHeaderSize + 64, checkSum);
    checkSumOK = (checkSum == _optHeader.CheckSum);
  }

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_inStream);

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    Int32 askMode = testMode ?
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    const CSection &item = _sections[index];
    currentItemSize = item.GetPackSize();
    {
      CMyComPtr<ISequentialOutStream> realOutStream;
      RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
      if (!testMode && (!realOutStream))
        continue;
      outStreamSpec->SetStream(realOutStream);
      outStreamSpec->Init();
    }
      
    RINOK(extractCallback->PrepareOperation(askMode));
    RINOK(_inStream->Seek(item.Pa, STREAM_SEEK_SET, NULL));
    streamSpec->Init(currentItemSize);
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
    outStreamSpec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult((copyCoderSpec->TotalSize == currentItemSize) ?
      checkSumOK ?
        NArchive::NExtract::NOperationResult::kOK:
        NArchive::NExtract::NOperationResult::kCRCError:
        NArchive::NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"PE", L"", 0, 0xDD, { 0 }, 0, false, CreateArc, 0 };

REGISTER_ARC(Pe)

}}
