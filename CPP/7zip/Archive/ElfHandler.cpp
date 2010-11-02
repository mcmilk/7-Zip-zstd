// ElfHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/PropVariantUtils.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

static UInt16 Get16(const Byte *p, int be) { if (be) return GetBe16(p); return GetUi16(p); }
static UInt32 Get32(const Byte *p, int be) { if (be) return GetBe32(p); return GetUi32(p); }
static UInt64 Get64(const Byte *p, int be) { if (be) return GetBe64(p); return GetUi64(p); }

using namespace NWindows;

namespace NArchive {
namespace NElf {

#define ELF_CLASS_32 1
#define ELF_CLASS_64 2

#define ELF_DATA_2LSB 1
#define ELF_DATA_2MSB 2

#define NUM_SCAN_SECTIONS_MAX (1 << 6)

struct CHeader
{
  bool Mode64;
  bool Be;
  Byte Os;
  Byte AbiVer;

  UInt16 Type;
  UInt16 Machine;
  // UInt32 Version;

  // UInt64 EntryVa;
  UInt64 ProgOffset;
  UInt64 SectOffset;
  UInt32 Flags;
  UInt16 ElfHeaderSize;
  UInt16 SegmentEntrySize;
  UInt16 NumSegments;
  UInt16 SectEntrySize;
  UInt16 NumSections;
  // UInt16 SectNameStringTableIndex;

  bool Parse(const Byte *buf);

  bool CheckSegmentEntrySize() const
  {
    return (Mode64 && SegmentEntrySize == 0x38) || (!Mode64 && SegmentEntrySize == 0x20);
  };

  UInt64 GetHeadersSize() const
    { return ElfHeaderSize +
      (UInt64)SegmentEntrySize * NumSegments +
      (UInt64)SectEntrySize * NumSections; }
    
};

bool CHeader::Parse(const Byte *p)
{
  switch(p[4])
  {
    case ELF_CLASS_32: Mode64 = false; break;
    case ELF_CLASS_64: Mode64 = true; break;
    default: return false;
  }
  bool be;
  switch(p[5])
  {
    case ELF_DATA_2LSB: be = false; break;
    case ELF_DATA_2MSB: be = true; break;
    default: return false;
  }
  Be = be;
  if (p[6] != 1) // Version
    return false;
  Os = p[7];
  AbiVer = p[8];
  for (int i = 9; i < 16; i++)
    if (p[i] != 0)
      return false;

  Type = Get16(p + 0x10, be);
  Machine = Get16(p + 0x12, be);
  if (Get32(p + 0x14, be) != 1) // Version
    return false;

  if (Mode64)
  {
    // EntryVa = Get64(p + 0x18, be);
    ProgOffset = Get64(p + 0x20, be);
    SectOffset = Get64(p + 0x28, be);
    p += 0x30;
  }
  else
  {
    // EntryVa = Get32(p + 0x18, be);
    ProgOffset = Get32(p + 0x1C, be);
    SectOffset = Get32(p + 0x20, be);
    p += 0x24;
  }

  Flags = Get32(p + 0, be);
  ElfHeaderSize = Get16(p + 4, be);
  SegmentEntrySize = Get16(p + 6, be);
  NumSegments = Get16(p + 8, be);
  SectEntrySize = Get16(p + 10, be);
  NumSections = Get16(p + 12, be);
  // SectNameStringTableIndex = Get16(p + 14, be);
  return CheckSegmentEntrySize();
}

struct CSegment
{
  UInt32 Type;
  UInt32 Flags;
  UInt64 Offset;
  UInt64 Va;
  // UInt64 Pa;
  UInt64 PSize;
  UInt64 VSize;
  // UInt64 Align;

  void UpdateTotalSize(UInt64 &totalSize)
  {
    UInt64 t = Offset + PSize;
    if (t > totalSize)
      totalSize = t;
  }
  void Parse(const Byte *p, bool mode64, bool be);
};

void CSegment::Parse(const Byte *p, bool mode64, bool be)
{
  Type = Get32(p, be);
  if (mode64)
  {
    Flags = Get32(p + 4, be);
    Offset = Get64(p + 8, be);
    Va = Get64(p + 0x10, be);
    // Pa = Get64(p + 0x18, be);
    PSize = Get64(p + 0x20, be);
    VSize = Get64(p + 0x28, be);
    // Align = Get64(p + 0x30, be);
  }
  else
  {
    Offset = Get32(p + 4, be);
    Va = Get32(p + 8, be);
    // Pa = Get32(p + 12, be);
    PSize = Get32(p + 16, be);
    VSize = Get32(p + 20, be);
    Flags = Get32(p + 24, be);
    // Align = Get32(p + 28, be);
  }
}

static const CUInt32PCharPair g_MachinePairs[] =
{
  { 0, "None" },
  { 1, "AT&T WE 32100" },
  { 2, "SPARC" },
  { 3, "Intel 386" },
  { 4, "Motorola 68000" },
  { 5, "Motorola 88000" },
  { 6, "Intel 486" },
  { 7, "Intel i860" },
  { 8, "MIPS" },
  { 9, "IBM S/370" },
  { 10, "MIPS RS3000 LE" },
  { 11, "RS6000" },

  { 15, "PA-RISC" },
  { 16, "nCUBE" },
  { 17, "Fujitsu VPP500" },
  { 18, "SPARC 32+" },
  { 19, "Intel i960" },
  { 20, "PowerPC" },
  { 21, "PowerPC 64-bit" },
  { 22, "IBM S/390" },

  { 36, "NEX v800" },
  { 37, "Fujitsu FR20" },
  { 38, "TRW RH-32" },
  { 39, "Motorola RCE" },
  { 40, "ARM" },
  { 41, "Alpha" },
  { 42, "Hitachi SH" },
  { 43, "SPARC-V9" },
  { 44, "Siemens Tricore" },
  { 45, "ARC" },
  { 46, "H8/300" },
  { 47, "H8/300H" },
  { 48, "H8S" },
  { 49, "H8/500" },
  { 50, "IA-64" },
  { 51, "Stanford MIPS-X" },
  { 52, "Motorola ColdFire" },
  { 53, "M68HC12" },
  { 54, "Fujitsu MMA" },
  { 55, "Siemens PCP" },
  { 56, "Sony nCPU" },
  { 57, "Denso NDR1" },
  { 58, "Motorola StarCore" },
  { 59, "Toyota ME16" },
  { 60, "ST100" },
  { 61, "Advanced Logic TinyJ" },
  { 62, "AMD64" },
  { 63, "Sony DSP" },

  { 66, "Siemens FX66" },
  { 67, "ST9+" },
  { 68, "ST7" },
  { 69, "MC68HC16" },
  { 70, "MC68HC11" },
  { 71, "MC68HC08" },
  { 72, "MC68HC05" },
  { 73, "Silicon Graphics SVx" },
  { 74, "ST19" },
  { 75, "Digital VAX" },
  { 76, "Axis CRIS" },
  { 77, "Infineon JAVELIN" },
  { 78, "Element 14 FirePath" },
  { 79, "LSI ZSP" },
  { 80, "MMIX" },
  { 81, "HUANY" },
  { 82, "SiTera Prism" },
  { 83, "Atmel AVR" },
  { 84, "Fujitsu FR30" },
  { 85, "Mitsubishi D10V" },
  { 86, "Mitsubishi D30V" },
  { 87, "NEC v850" },
  { 88, "Mitsubishi M32R" },
  { 89, "Matsushita MN10300" },
  { 90, "Matsushita MN10200" },
  { 91, "picoJava" },
  { 92, "OpenRISC" },
  { 93, "ARC Tangent-A5" },
  { 94, "Tensilica Xtensa" },
  { 0x9026, "Alpha" }
};

static const CUInt32PCharPair g_AbiOS[] =
{
  { 0, "None" },
  { 1, "HP-UX" },
  { 2, "NetBSD" },
  { 3, "Linux" },

  { 6, "Solaris" },
  { 7, "AIX" },
  { 8, "IRIX" },
  { 9, "FreeBSD" },
  { 10, "TRU64" },
  { 11, "Novell Modesto" },
  { 12, "OpenBSD" },
  { 13, "OpenVMS" },
  { 14, "HP NSK" },
  { 15, "AROS" },
  { 97, "ARM" },
  { 255, "Standalone" }
};

static const CUInt32PCharPair g_SegmentFlags[] =
{
  { 0, "Execute" },
  { 1, "Write" },
  { 2, "Read" }
};

static const char *g_Types[] =
{
  "None",
  "Relocatable file",
  "Executable file",
  "Shared object file",
  "Core file"
};

static const char *g_SegnmentTypes[] =
{
  "Unused",
  "Loadable segment",
  "Dynamic linking tables",
  "Program interpreter path name",
  "Note section",
  "SHLIB",
  "Program header table",
  "TLS"
};

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _inStream;
  CObjectVector<CSegment> _sections;
  UInt32 _peOffset;
  CHeader _header;
  UInt64 _totalSize;
  HRESULT Open2(IInStream *stream);
  bool Parse(const Byte *buf, UInt32 size);
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

#define ELF_PT_PHDR 6

bool CHandler::Parse(const Byte *buf, UInt32 size)
{
  if (size < 64)
    return false;
  if (!_header.Parse(buf))
    return false;
  if (_header.ProgOffset > size ||
      _header.ProgOffset + (UInt64)_header.SegmentEntrySize * _header.NumSegments > size ||
      _header.NumSegments > NUM_SCAN_SECTIONS_MAX)
    return false;
  const Byte *p = buf + _header.ProgOffset;
  _totalSize = _header.ProgOffset;
  
  for (int i = 0; i < _header.NumSegments; i++, p += _header.SegmentEntrySize)
  {
    CSegment sect;
    sect.Parse(p, _header.Mode64, _header.Be);
    sect.UpdateTotalSize(_totalSize);
    if (sect.Type != ELF_PT_PHDR)
      _sections.Add(sect);
  }
  UInt64 total2 = _header.SectOffset + (UInt64)_header.SectEntrySize * _header.NumSections;
  if (total2 > _totalSize)
    _totalSize = total2;
  return true;
}

STATPROPSTG kArcProps[] =
{
  { NULL, kpidCpu, VT_BSTR},
  { NULL, kpidBit64, VT_BOOL},
  { NULL, kpidBigEndian, VT_BOOL},
  { NULL, kpidHostOS, VT_BSTR},
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidPhySize, VT_UI8},
  { NULL, kpidHeadersSize, VT_UI8}
 };

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidType, VT_BSTR},
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidOffset, VT_UI8},
  { NULL, kpidVa, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPhySize:  prop = _totalSize; break;
    case kpidHeadersSize:  prop = _header.GetHeadersSize(); break;
    case kpidBit64:  if (_header.Mode64) prop = _header.Mode64; break;
    case kpidBigEndian:  if (_header.Be) prop = _header.Be; break;
    case kpidCpu:  PAIR_TO_PROP(g_MachinePairs, _header.Machine, prop); break;
    case kpidHostOS:  PAIR_TO_PROP(g_AbiOS, _header.Os, prop); break;
    case kpidCharacts:  TYPE_TO_PROP(g_Types, _header.Type, prop); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CSegment &item = _sections[index];
  switch(propID)
  {
    case kpidPath:
    {
      wchar_t sz[32];
      ConvertUInt64ToString(index, sz);
      prop = sz;
      break;
    }
    case kpidSize:  prop = (UInt64)item.VSize; break;
    case kpidPackSize:  prop = (UInt64)item.PSize; break;
    case kpidOffset:  prop = item.Offset; break;
    case kpidVa:  prop = item.Va; break;
    case kpidType:  TYPE_TO_PROP(g_SegnmentTypes, item.Type, prop); break;
    case kpidCharacts:  FLAGS_TO_PROP(g_SegmentFlags, item.Flags, prop); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *stream)
{
  const UInt32 kBufSize = 1 << 18;
  const UInt32 kSigSize = 4;

  CByteBuffer buffer;
  buffer.SetCapacity(kBufSize);
  Byte *buf = buffer;

  size_t processed = kSigSize;
  RINOK(ReadStream_FALSE(stream, buf, processed));
  if (buf[0] != 0x7F || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F')
    return S_FALSE;
  processed = kBufSize - kSigSize;
  RINOK(ReadStream(stream, buf + kSigSize, &processed));
  processed += kSigSize;
  if (!Parse(buf, (UInt32)processed))
    return S_FALSE;
  UInt64 fileSize;
  RINOK(stream->Seek(0, STREAM_SEEK_END, &fileSize));
  return (fileSize == _totalSize) ? S_OK : S_FALSE;
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

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _sections.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _sections[allFilesMode ? i : indices[i]].PSize;
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_inStream);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    const CSegment &item = _sections[index];
    currentItemSize = item.PSize;
    
    CMyComPtr<ISequentialOutStream> outStream;
    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    if (!testMode && !outStream)
      continue;
      
    RINOK(extractCallback->PrepareOperation(askMode));
    RINOK(_inStream->Seek(item.Offset, STREAM_SEEK_SET, NULL));
    streamSpec->Init(currentItemSize);
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(copyCoderSpec->TotalSize == currentItemSize ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"ELF", L"", 0, 0xDE, { 0 }, 0, false, CreateArc, 0 };

REGISTER_ARC(Elf)

}}
