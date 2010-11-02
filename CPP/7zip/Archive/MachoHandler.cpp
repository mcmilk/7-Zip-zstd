// MachoHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"

#include "Windows/PropVariantUtils.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

static UInt32 Get32(const Byte *p, int be) { if (be) return GetBe32(p); return GetUi32(p); }
static UInt64 Get64(const Byte *p, int be) { if (be) return GetBe64(p); return GetUi64(p); }

using namespace NWindows;

namespace NArchive {
namespace NMacho {

#define MACH_ARCH_ABI64 (1 << 24)
#define MACH_MACHINE_386 7
#define MACH_MACHINE_ARM 12
#define MACH_MACHINE_SPARC 14
#define MACH_MACHINE_PPC 18

#define MACH_MACHINE_PPC64 (MACH_ARCH_ABI64 | MACH_MACHINE_PPC)
#define MACH_MACHINE_AMD64 (MACH_ARCH_ABI64 | MACH_MACHINE_386)

#define MACH_CMD_SEGMENT_32 1
#define MACH_CMD_SEGMENT_64 0x19

#define MACH_SECT_TYPE_MASK 0x000000FF
#define MACH_SECT_ATTR_MASK 0xFFFFFF00

#define MACH_SECT_ATTR_ZEROFILL 1

static const char *g_SectTypes[] =
{
  "REGULAR",
  "ZEROFILL",
  "CSTRINGS",
  "4BYTE_LITERALS",
  "8BYTE_LITERALS",
  "LITERAL_POINTERS",
  "NON_LAZY_SYMBOL_POINTERS",
  "LAZY_SYMBOL_POINTERS",
  "SYMBOL_STUBS",
  "MOD_INIT_FUNC_POINTERS",
  "MOD_TERM_FUNC_POINTERS",
  "COALESCED",
  "GB_ZEROFILL",
  "INTERPOSING",
  "16BYTE_LITERALS"
};

static const char *g_FileTypes[] =
{
  "0",
  "OBJECT",
  "EXECUTE",
  "FVMLIB",
  "CORE",
  "PRELOAD",
  "DYLIB",
  "DYLINKER",
  "BUNDLE",
  "DYLIB_STUB",
  "DSYM"
};

static const CUInt32PCharPair g_Flags[] =
{
  { 31, "PURE_INSTRUCTIONS" },
  { 30, "NO_TOC" },
  { 29, "STRIP_STATIC_SYMS" },
  { 28, "NO_DEAD_STRIP" },
  { 27, "LIVE_SUPPORT" },
  { 26, "SELF_MODIFYING_CODE" },
  { 25, "DEBUG" },
  { 10, "SOME_INSTRUCTIONS" },
  {  9, "EXT_RELOC" },
  {  8, "LOC_RELOC" }
};

static const CUInt32PCharPair g_MachinePairs[] =
{
  { MACH_MACHINE_386, "x86" },
  { MACH_MACHINE_ARM, "ARM" },
  { MACH_MACHINE_SPARC, "SPARC" },
  { MACH_MACHINE_PPC, "PowerPC" },
  { MACH_MACHINE_PPC64, "PowerPC 64-bit" },
  { MACH_MACHINE_AMD64, "x64" }
};

static const int kNameSize = 16;

struct CSegment
{
  char Name[kNameSize];
};

struct CSection
{
  char Name[kNameSize];
  char SegName[kNameSize];
  UInt64 Va;
  UInt64 Pa;
  UInt64 VSize;
  UInt64 PSize;

  UInt32 Flags;
  int SegmentIndex;

  bool IsDummy;

  CSection(): IsDummy(false) {}
  // UInt64 GetPackSize() const { return Flags == MACH_SECT_ATTR_ZEROFILL ? 0 : Size; }
  UInt64 GetPackSize() const { return PSize; }
};


class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _inStream;
  CObjectVector<CSegment> _segments;
  CObjectVector<CSection> _sections;
  bool _mode64;
  bool _be;
  UInt32 _machine;
  UInt32 _type;
  UInt32 _headersSize;
  UInt64 _totalSize;
  HRESULT Open2(ISequentialInStream *stream);
  bool Parse(const Byte *buf, UInt32 size);
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

bool CHandler::Parse(const Byte *buf, UInt32 size)
{
  bool mode64 = _mode64;
  bool be = _be;

  const Byte *bufStart = buf;
  bool reduceCommands = false;
  if (size < 512)
    return false;

  _machine = Get32(buf + 4, be);
  _type = Get32(buf + 0xC, be);

  UInt32 numCommands = Get32(buf + 0x10, be);
  UInt32 commandsSize = Get32(buf + 0x14, be);
  if (commandsSize > size)
    return false;

  if (commandsSize > (1 << 24) || numCommands > (1 << 18))
    return false;

  if (numCommands > 16)
  {
    reduceCommands = true;
    numCommands = 16;
  }

  _headersSize = 0;

  buf += 0x1C;
  size -= 0x1C;

  if (mode64)
  {
    buf += 4;
    size -= 4;
  }

  _totalSize = (UInt32)(buf - bufStart);
  if (commandsSize < size)
    size = commandsSize;

  for (UInt32 cmdIndex = 0; cmdIndex < numCommands; cmdIndex++)
  {
    if (size < 8)
      return false;
    UInt32 cmd = Get32(buf, be);
    UInt32 cmdSize = Get32(buf + 4, be);
    if (size < cmdSize)
      return false;
    if (cmd == MACH_CMD_SEGMENT_32 || cmd == MACH_CMD_SEGMENT_64)
    {
      UInt32 offs = (cmd == MACH_CMD_SEGMENT_64) ? 0x48 : 0x38;
      if (cmdSize < offs)
        break;

      UInt64 vmAddr, vmSize, phAddr, phSize;

      {
        if (cmd == MACH_CMD_SEGMENT_64)
        {
          vmAddr = Get64(buf + 0x18, be);
          vmSize = Get64(buf + 0x20, be);
          phAddr = Get64(buf + 0x28, be);
          phSize = Get64(buf + 0x30, be);
        }
        else
        {
          vmAddr = Get32(buf + 0x18, be);
          vmSize = Get32(buf + 0x1C, be);
          phAddr = Get32(buf + 0x20, be);
          phSize = Get32(buf + 0x24, be);
        }
        {
          UInt64 totalSize = phAddr + phSize;
          if (totalSize > _totalSize)
            _totalSize = totalSize;
        }
      }
      
      CSegment seg;
      memcpy(seg.Name, buf + 8, kNameSize);
      _segments.Add(seg);

      UInt32 numSections = Get32(buf + offs - 8, be);
      if (numSections > (1 << 8))
        return false;

      if (numSections == 0)
      {
        CSection section;
        section.IsDummy = true;
        section.SegmentIndex = _segments.Size() - 1;
          section.Va = vmAddr;
          section.PSize = phSize;
          section.VSize = vmSize;
          section.Pa = phAddr;
          section.Flags = 0;
        _sections.Add(section);
      }
      else do
      {
        CSection section;
        UInt32 headerSize = (cmd == MACH_CMD_SEGMENT_64) ? 0x50 : 0x44;
        const Byte *p = buf + offs;
        if (cmdSize - offs < headerSize)
          break;
        if (cmd == MACH_CMD_SEGMENT_64)
        {
          section.Va = Get64(p + 0x20, be);
          section.VSize = Get64(p + 0x28, be);
          section.Pa = Get32(p + 0x30, be);
          section.Flags = Get32(p + 0x40, be);
        }
        else
        {
          section.Va = Get32(p + 0x20, be);
          section.VSize = Get32(p + 0x24, be);
          section.Pa = Get32(p + 0x28, be);
          section.Flags = Get32(p + 0x38, be);
        }
        if (section.Flags == MACH_SECT_ATTR_ZEROFILL)
          section.PSize = 0;
        else
          section.PSize = section.VSize;
        memcpy(section.Name, p, kNameSize);
        memcpy(section.SegName, p + kNameSize, kNameSize);
        section.SegmentIndex = _segments.Size() - 1;
        _sections.Add(section);
        offs += headerSize;
      }
      while (--numSections);

      if (offs != cmdSize)
        return false;
    }
    buf += cmdSize;
    size -= cmdSize;
  }
  _headersSize = (UInt32)(buf - bufStart);
  return reduceCommands || (size == 0);
}

static STATPROPSTG kArcProps[] =
{
  { NULL, kpidCpu, VT_BSTR},
  { NULL, kpidBit64, VT_BOOL},
  { NULL, kpidBigEndian, VT_BOOL},
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidPhySize, VT_UI8},
  { NULL, kpidHeadersSize, VT_UI4}
};

static STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
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
    case kpidCpu:  PAIR_TO_PROP(g_MachinePairs, _machine, prop); break;
    case kpidCharacts:  TYPE_TO_PROP(g_FileTypes, _type, prop); break;
    case kpidPhySize:  prop = _totalSize; break;
    case kpidHeadersSize:  prop = _headersSize; break;
    case kpidBit64:  if (_mode64) prop = _mode64; break;
    case kpidBigEndian:  if (_be) prop = _be; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

static AString GetName(const char *name)
{
  char res[kNameSize + 1];
  memcpy(res, name, kNameSize);
  res[kNameSize] = 0;
  return res;
}

static AString SectFlagsToString(UInt32 flags)
{
  AString res = TypeToString(g_SectTypes, sizeof(g_SectTypes) / sizeof(g_SectTypes[0]),
      flags & MACH_SECT_TYPE_MASK);
  AString s = FlagsToString(g_Flags, sizeof(g_Flags) / sizeof(g_Flags[0]),
      flags & MACH_SECT_ATTR_MASK);
  if (!s.IsEmpty())
  {
    res += ' ';
    res += s;
  }
  return res;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CSection &item = _sections[index];
  switch(propID)
  {
    case kpidPath:
    {
      AString s = GetName(_segments[item.SegmentIndex].Name);
      if (!item.IsDummy)
        s += GetName(item.Name);
      StringToProp(s, prop);
      break;
    }
    case kpidSize:  /* prop = (UInt64)item.VSize; break; */
    case kpidPackSize:  prop = (UInt64)item.GetPackSize(); break;
    case kpidCharacts:  if (!item.IsDummy) StringToProp(SectFlagsToString(item.Flags), prop); break;
    case kpidOffset:  prop = item.Pa; break;
    case kpidVa:  prop = item.Va; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::Open2(ISequentialInStream *stream)
{
  const UInt32 kBufSize = 1 << 18;
  const UInt32 kSigSize = 4;

  CByteBuffer buffer;
  buffer.SetCapacity(kBufSize);
  Byte *buf = buffer;

  size_t processed = kSigSize;
  RINOK(ReadStream_FALSE(stream, buf, processed));
  UInt32 sig = GetUi32(buf);
  bool be, mode64;
  switch(sig)
  {
    case 0xCEFAEDFE:  be = true; mode64 = false; break;
    case 0xCFFAEDFE:  be = true; mode64 = true; break;
    case 0xFEEDFACE:  be = false; mode64 = false; break;
    case 0xFEEDFACF:  be = false; mode64 = true; break;
    default: return S_FALSE;
  }
  processed = kBufSize - kSigSize;
  RINOK(ReadStream(stream, buf + kSigSize, &processed));
  _mode64 = mode64;
  _be = be;
  return Parse(buf, (UInt32)processed + kSigSize) ? S_OK : S_FALSE;
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
  _segments.Clear();
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
    totalSize += _sections[allFilesMode ? i : indices[i]].GetPackSize();
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
    const CSection &item = _sections[index];
    currentItemSize = item.GetPackSize();

    CMyComPtr<ISequentialOutStream> outStream;
    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    if (!testMode && !outStream)
      continue;
    
    RINOK(extractCallback->PrepareOperation(askMode));
    RINOK(_inStream->Seek(item.Pa, STREAM_SEEK_SET, NULL));
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
  { L"MachO", L"", 0, 0xDF, { 0 }, 0, false, CreateArc, 0 };

REGISTER_ARC(Macho)

}}
