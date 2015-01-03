// List.cpp

#include "StdAfx.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/MyCom.h"
#include "../../../Common/StdOutStream.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/UTFConvert.h"

#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"

#include "../Common/OpenArchive.h"
#include "../Common/PropIDUtils.h"

#include "ConsoleClose.h"
#include "List.h"
#include "OpenCallbackConsole.h"

using namespace NWindows;
using namespace NCOM;



static const char *kPropIdToName[] =
{
    "0"
  , "1"
  , "2"
  , "Path"
  , "Name"
  , "Extension"
  , "Folder"
  , "Size"
  , "Packed Size"
  , "Attributes"
  , "Created"
  , "Accessed"
  , "Modified"
  , "Solid"
  , "Commented"
  , "Encrypted"
  , "Split Before"
  , "Split After"
  , "Dictionary Size"
  , "CRC"
  , "Type"
  , "Anti"
  , "Method"
  , "Host OS"
  , "File System"
  , "User"
  , "Group"
  , "Block"
  , "Comment"
  , "Position"
  , "Path Prefix"
  , "Folders"
  , "Files"
  , "Version"
  , "Volume"
  , "Multivolume"
  , "Offset"
  , "Links"
  , "Blocks"
  , "Volumes"
  , "Time Type"
  , "64-bit"
  , "Big-endian"
  , "CPU"
  , "Physical Size"
  , "Headers Size"
  , "Checksum"
  , "Characteristics"
  , "Virtual Address"
  , "ID"
  , "Short Name"
  , "Creator Application"
  , "Sector Size"
  , "Mode"
  , "Symbolic Link"
  , "Error"
  , "Total Size"
  , "Free Space"
  , "Cluster Size"
  , "Label"
  , "Local Name"
  , "Provider"
  , "NT Security"
  , "Alternate Stream"
  , "Aux"
  , "Deleted"
  , "Tree"
  , "SHA-1"
  , "SHA-256"
  , "Error Type"
  , "Errors"
  , "Errors"
  , "Warnings"
  , "Warning"
  , "Streams"
  , "Alternate Streams"
  , "Alternate Streams Size"
  , "Virtual Size"
  , "Unpack Size"
  , "Total Physical Size"
  , "Volume Index"
  , "SubType"
  , "Short Comment"
  , "Code Page"
  , "Is not archive type"
  , "Physical Size can't be detected"
  , "Zeros Tail Is Allowed"
  , "Tail Size"
  , "Embedded Stub Size"
  , "Link"
  , "Hard Link"
  , "iNode"
  , "Stream ID"
};

static const char kEmptyAttribChar = '.';

static const char *kListing = "Listing archive: ";

static const char *kString_Files = "files";
static const char *kString_Dirs = "folders";
static const char *kString_AltStreams = "alternate streams";
static const char *kString_Streams = "streams";

static void GetAttribString(UInt32 wa, bool isDir, bool allAttribs, char *s)
{
  if (isDir)
    wa |= FILE_ATTRIBUTE_DIRECTORY;
  if (allAttribs)
  {
    ConvertWinAttribToString(s, wa);
    return;
  }
  s[0] = ((wa & FILE_ATTRIBUTE_DIRECTORY) != 0) ? 'D': kEmptyAttribChar;
  s[1] = ((wa & FILE_ATTRIBUTE_READONLY)  != 0) ? 'R': kEmptyAttribChar;
  s[2] = ((wa & FILE_ATTRIBUTE_HIDDEN)    != 0) ? 'H': kEmptyAttribChar;
  s[3] = ((wa & FILE_ATTRIBUTE_SYSTEM)    != 0) ? 'S': kEmptyAttribChar;
  s[4] = ((wa & FILE_ATTRIBUTE_ARCHIVE)   != 0) ? 'A': kEmptyAttribChar;
  s[5] = 0;
}

enum EAdjustment
{
  kLeft,
  kCenter,
  kRight
};

struct CFieldInfo
{
  PROPID PropID;
  bool IsRawProp;
  UString NameU;
  AString NameA;
  EAdjustment TitleAdjustment;
  EAdjustment TextAdjustment;
  int PrefixSpacesWidth;
  int Width;
};

struct CFieldInfoInit
{
  PROPID PropID;
  const char *Name;
  EAdjustment TitleAdjustment;
  EAdjustment TextAdjustment;
  int PrefixSpacesWidth;
  int Width;
};

static const CFieldInfoInit kStandardFieldTable[] =
{
  { kpidMTime, "   Date      Time", kLeft, kLeft, 0, 19 },
  { kpidAttrib, "Attr", kRight, kCenter, 1, 5 },
  { kpidSize, "Size", kRight, kRight, 1, 12 },
  { kpidPackSize, "Compressed", kRight, kRight, 1, 12 },
  { kpidPath, "Name", kLeft, kLeft, 2, 24 }
};

const int kNumSpacesMax = 32; // it must be larger than max CFieldInfoInit.Width
static const char *g_Spaces =
"                                " ;

static void PrintSpaces(int numSpaces)
{
  if (numSpaces > 0 && numSpaces <= kNumSpacesMax)
    g_StdOut << g_Spaces + (kNumSpacesMax - numSpaces);
}

static void PrintSpacesToString(char *dest, int numSpaces)
{
  int i;
  for (i = 0; i < numSpaces; i++)
    dest[i] = ' ';
  dest[i] = 0;
}

static void PrintString(EAdjustment adj, int width, const UString &textString)
{
  const int numSpaces = width - textString.Len();
  int numLeftSpaces = 0;
  switch (adj)
  {
    case kLeft:   numLeftSpaces = 0; break;
    case kCenter: numLeftSpaces = numSpaces / 2; break;
    case kRight:  numLeftSpaces = numSpaces; break;
  }
  PrintSpaces(numLeftSpaces);
  g_StdOut << textString;
  PrintSpaces(numSpaces - numLeftSpaces);
}

static void PrintString(EAdjustment adj, int width, const char *textString)
{
  const int numSpaces = width - (int)strlen(textString);
  int numLeftSpaces = 0;
  switch (adj)
  {
    case kLeft:   numLeftSpaces = 0; break;
    case kCenter: numLeftSpaces = numSpaces / 2; break;
    case kRight:  numLeftSpaces = numSpaces; break;
  }
  PrintSpaces(numLeftSpaces);
  g_StdOut << textString;
  PrintSpaces(numSpaces - numLeftSpaces);
}

static void PrintStringToString(char *dest, EAdjustment adj, int width, const char *textString)
{
  int len = (int)strlen(textString);
  const int numSpaces = width - len;
  int numLeftSpaces = 0;
  switch (adj)
  {
    case kLeft:   numLeftSpaces = 0; break;
    case kCenter: numLeftSpaces = numSpaces / 2; break;
    case kRight:  numLeftSpaces = numSpaces; break;
  }
  PrintSpacesToString(dest, numLeftSpaces);
  if (numLeftSpaces > 0)
    dest += numLeftSpaces;
  memcpy(dest, textString, len);
  dest += len;
  PrintSpacesToString(dest, numSpaces - numLeftSpaces);
}

struct CListUInt64Def
{
  UInt64 Val;
  bool Def;

  CListUInt64Def(): Val(0), Def(false) {}
  void Add(UInt64 v) { Val += v; Def = true; }
  void Add(const CListUInt64Def &v) { if (v.Def) Add(v.Val); }
};

struct CListFileTimeDef
{
  FILETIME Val;
  bool Def;

  CListFileTimeDef(): Def(false) { Val.dwLowDateTime = 0; Val.dwHighDateTime = 0; }
  void Update(const CListFileTimeDef &t)
  {
    if (t.Def && (!Def || CompareFileTime(&Val, &t.Val) < 0))
    {
      Val = t.Val;
      Def = true;
    }
  }
};

struct CListStat
{
  CListUInt64Def Size;
  CListUInt64Def PackSize;
  CListFileTimeDef MTime;
  UInt64 NumFiles;

  CListStat(): NumFiles(0) {}
  void Update(const CListStat &stat)
  {
    Size.Add(stat.Size);
    PackSize.Add(stat.PackSize);
    MTime.Update(stat.MTime);
    NumFiles += stat.NumFiles;
  }
  void SetSizeDefIfNoFiles() { if (NumFiles == 0) Size.Def = true; }
};

struct CListStat2
{
  CListStat MainFiles;
  CListStat AltStreams;
  UInt64 NumDirs;

  CListStat2(): NumDirs(0) {}

  void Update(const CListStat2 &stat)
  {
    MainFiles.Update(stat.MainFiles);
    AltStreams.Update(stat.AltStreams);
    NumDirs += stat.NumDirs;
  }
  const UInt64 GetNumStreams() const { return MainFiles.NumFiles + AltStreams.NumFiles; }
  CListStat &GetStat(bool altStreamsMode) { return altStreamsMode ? AltStreams : MainFiles; }
};

class CFieldPrinter
{
  CObjectVector<CFieldInfo> _fields;

  void AddProp(BSTR name, PROPID propID, bool isRawProp);
public:
  const CArc *Arc;
  bool TechMode;
  UString FilePath;
  AString TempAString;
  UString TempWString;
  bool IsFolder;

  AString LinesString;

  void Clear() { _fields.Clear(); LinesString.Empty(); }
  void Init(const CFieldInfoInit *standardFieldTable, int numItems);

  HRESULT AddMainProps(IInArchive *archive);
  HRESULT AddRawProps(IArchiveGetRawProps *getRawProps);
  
  void PrintTitle();
  void PrintTitleLines();
  HRESULT PrintItemInfo(UInt32 index, const CListStat &stat);
  void PrintSum(const CListStat &stat, UInt64 numDirs, const char *str);
  void PrintSum(const CListStat2 &stat);
};

void CFieldPrinter::Init(const CFieldInfoInit *standardFieldTable, int numItems)
{
  Clear();
  for (int i = 0; i < numItems; i++)
  {
    CFieldInfo &f = _fields.AddNew();
    const CFieldInfoInit &fii = standardFieldTable[i];
    f.PropID = fii.PropID;
    f.IsRawProp = false;
    f.NameA = fii.Name;
    f.TitleAdjustment = fii.TitleAdjustment;
    f.TextAdjustment = fii.TextAdjustment;
    f.PrefixSpacesWidth = fii.PrefixSpacesWidth;
    f.Width = fii.Width;

    int k;
    for (k = 0; k < fii.PrefixSpacesWidth; k++)
      LinesString += ' ';
    for (k = 0; k < fii.Width; k++)
      LinesString += '-';
  }
}

static void GetPropName(PROPID propID, const wchar_t *name, AString &nameA, UString &nameU)
{
  if (propID < ARRAY_SIZE(kPropIdToName))
  {
    nameA = kPropIdToName[propID];
    return;
  }
  if (name)
    nameU = name;
  else
  {
    char s[16];
    ConvertUInt32ToString(propID, s);
    nameA = s;
  }
}

void CFieldPrinter::AddProp(BSTR name, PROPID propID, bool isRawProp)
{
  CFieldInfo f;
  f.PropID = propID;
  f.IsRawProp = isRawProp;
  GetPropName(propID, name, f.NameA, f.NameU);
  f.NameU += L" = ";
  if (!f.NameA.IsEmpty())
    f.NameA += " = ";
  else
  {
    const UString &s = f.NameU;
    AString sA;
    unsigned i;
    for (i = 0; i < s.Len(); i++)
    {
      wchar_t c = s[i];
      if (c >= 0x80)
        break;
      sA += (char)c;
    }
    if (i == s.Len())
      f.NameA = sA;
  }
  _fields.Add(f);
}

HRESULT CFieldPrinter::AddMainProps(IInArchive *archive)
{
  UInt32 numProps;
  RINOK(archive->GetNumberOfProperties(&numProps));
  for (UInt32 i = 0; i < numProps; i++)
  {
    CMyComBSTR name;
    PROPID propID;
    VARTYPE vt;
    RINOK(archive->GetPropertyInfo(i, &name, &propID, &vt));
    AddProp(name, propID, false);
  }
  return S_OK;
}

HRESULT CFieldPrinter::AddRawProps(IArchiveGetRawProps *getRawProps)
{
  UInt32 numProps;
  RINOK(getRawProps->GetNumRawProps(&numProps));
  for (UInt32 i = 0; i < numProps; i++)
  {
    CMyComBSTR name;
    PROPID propID;
    RINOK(getRawProps->GetRawPropInfo(i, &name, &propID));
    AddProp(name, propID, true);
  }
  return S_OK;
}

void CFieldPrinter::PrintTitle()
{
  FOR_VECTOR (i, _fields)
  {
    const CFieldInfo &f = _fields[i];
    PrintSpaces(f.PrefixSpacesWidth);
    PrintString(f.TitleAdjustment, ((f.PropID == kpidPath) ? 0: f.Width), f.NameA);
  }
}

void CFieldPrinter::PrintTitleLines()
{
  g_StdOut << LinesString;
}

static void PrintTime(char *dest, const FILETIME *ft)
{
  *dest = 0;
  if (ft->dwLowDateTime == 0 && ft->dwHighDateTime == 0)
    return;
  FILETIME locTime;
  if (!FileTimeToLocalFileTime(ft, &locTime))
    throw 20121211;
  ConvertFileTimeToString(locTime, dest, true, true);
}

#ifndef _SFX

static inline char GetHex(Byte value)
{
  return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}

static void HexToString(char *dest, const Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
  {
    Byte b = data[i];
    dest[0] = GetHex((Byte)((b >> 4) & 0xF));
    dest[1] = GetHex((Byte)(b & 0xF));
    dest += 2;
  }
  *dest = 0;
}

#endif

#define MY_ENDL '\n'

HRESULT CFieldPrinter::PrintItemInfo(UInt32 index, const CListStat &stat)
{
  char temp[128];
  size_t tempPos = 0;

  bool techMode = this->TechMode;
  /*
  if (techMode)
  {
    g_StdOut << "Index = ";
    g_StdOut << (UInt64)index;
    g_StdOut << endl;
  }
  */
  FOR_VECTOR (i, _fields)
  {
    const CFieldInfo &f = _fields[i];

    if (!techMode)
    {
      PrintSpacesToString(temp + tempPos, f.PrefixSpacesWidth);
      tempPos += f.PrefixSpacesWidth;
    }

    if (techMode)
    {
      if (!f.NameA.IsEmpty())
        g_StdOut << f.NameA;
      else
        g_StdOut << f.NameU;
    }
    
    if (f.PropID == kpidPath)
    {
      if (!techMode)
        g_StdOut << temp;
      g_StdOut.PrintUString(FilePath, TempAString);
      if (techMode)
        g_StdOut << MY_ENDL;
      continue;
    }

    int width = f.Width;
    
    if (f.IsRawProp)
    {
      #ifndef _SFX
      
      const void *data;
      UInt32 dataSize;
      UInt32 propType;
      RINOK(Arc->GetRawProps->GetRawProp(index, f.PropID, &data, &dataSize, &propType));
      
      if (dataSize != 0)
      {
        bool needPrint = true;
        
        if (f.PropID == kpidNtSecure)
        {
          if (propType != NPropDataType::kRaw)
            return E_FAIL;
          #ifndef _SFX
          ConvertNtSecureToString((const Byte *)data, dataSize, TempAString);
          g_StdOut << TempAString;
          needPrint = false;
          #endif
        }
        else if (f.PropID == kpidNtReparse)
        {
          UString s;
          if (ConvertNtReparseToString((const Byte *)data, dataSize, s))
          {
            needPrint = false;
            g_StdOut << s;
          }
        }
      
        if (needPrint)
        {
          if (propType != NPropDataType::kRaw)
            return E_FAIL;
          
          const UInt32 kMaxDataSize = 64;
          
          if (dataSize > kMaxDataSize)
          {
            g_StdOut << "data:";
            g_StdOut << dataSize;
          }
          else
          {
            char hexStr[kMaxDataSize * 2 + 4];
            HexToString(hexStr, (const Byte *)data, dataSize);
            g_StdOut << hexStr;
          }
        }
      }
      
      #endif
    }
    else
    {
      CPropVariant prop;
      switch (f.PropID)
      {
        case kpidSize: if (stat.Size.Def) prop = stat.Size.Val; break;
        case kpidPackSize: if (stat.PackSize.Def) prop = stat.PackSize.Val; break;
        case kpidMTime: if (stat.MTime.Def) prop = stat.MTime.Val; break;
        default:
          RINOK(Arc->Archive->GetProperty(index, f.PropID, &prop));
      }
      if (f.PropID == kpidAttrib && (prop.vt == VT_EMPTY || prop.vt == VT_UI4))
      {
        GetAttribString((prop.vt == VT_EMPTY) ? 0 : prop.ulVal, IsFolder, techMode, temp + tempPos);
        if (techMode)
          g_StdOut << temp + tempPos;
        else
          tempPos += strlen(temp + tempPos);
      }
      else if (prop.vt == VT_EMPTY)
      {
        if (!techMode)
        {
          PrintSpacesToString(temp + tempPos, width);
          tempPos += width;
        }
      }
      else if (prop.vt == VT_FILETIME)
      {
        PrintTime(temp + tempPos, &prop.filetime);
        if (techMode)
          g_StdOut << temp + tempPos;
        else
        {
          size_t len = strlen(temp + tempPos);
          tempPos += len;
          if (len < (unsigned)f.Width)
          {
            len = (size_t)f.Width - len;
            PrintSpacesToString(temp + tempPos, (int)len);
            tempPos += len;
          }
        }
      }
      else if (prop.vt == VT_BSTR)
      {
        if (techMode)
        {
          int len = (int)wcslen(prop.bstrVal);
          MyStringCopy(TempWString.GetBuffer(len), prop.bstrVal);
          // replace CR/LF here.
          TempWString.ReleaseBuffer(len);
          g_StdOut.PrintUString(TempWString, TempAString);
        }
        else
          PrintString(f.TextAdjustment, width, prop.bstrVal);
      }
      else
      {
        char s[64];
        ConvertPropertyToShortString(s, prop, f.PropID);
        if (techMode)
          g_StdOut << s;
        else
        {
          PrintStringToString(temp + tempPos, f.TextAdjustment, width, s);
          tempPos += strlen(temp + tempPos);
        }
      }
    }
    if (techMode)
      g_StdOut << MY_ENDL;
  }
  g_StdOut << MY_ENDL;
  return S_OK;
}

static void PrintNumber(EAdjustment adj, int width, const CListUInt64Def &value)
{
  wchar_t s[32];
  s[0] = 0;
  if (value.Def)
    ConvertUInt64ToString(value.Val, s);
  PrintString(adj, width, s);
}

static void PrintNumberAndString(AString &s, UInt64 value, const char *text)
{
  char t[32];
  ConvertUInt64ToString(value, t);
  s += t;
  s += ' ';
  s += text;
}

void CFieldPrinter::PrintSum(const CListStat &stat, UInt64 numDirs, const char *str)
{
  FOR_VECTOR (i, _fields)
  {
    const CFieldInfo &f = _fields[i];
    PrintSpaces(f.PrefixSpacesWidth);
    if (f.PropID == kpidSize)
      PrintNumber(f.TextAdjustment, f.Width, stat.Size);
    else if (f.PropID == kpidPackSize)
      PrintNumber(f.TextAdjustment, f.Width, stat.PackSize);
    else if (f.PropID == kpidMTime)
    {
      char s[64];
      s[0] = 0;
      if (stat.MTime.Def)
        PrintTime(s, &stat.MTime.Val);
      PrintString(f.TextAdjustment, f.Width, s);
    }
    else if (f.PropID == kpidPath)
    {
      AString s;
      PrintNumberAndString(s, stat.NumFiles, str);
      if (numDirs != 0)
      {
        s += ", ";
        PrintNumberAndString(s, numDirs, kString_Dirs);
      }
      PrintString(f.TextAdjustment, 0, s);
    }
    else
      PrintString(f.TextAdjustment, f.Width, L"");
  }
  g_StdOut << endl;
}

void CFieldPrinter::PrintSum(const CListStat2 &stat2)
{
  PrintSum(stat2.MainFiles, stat2.NumDirs, kString_Files);
  if (stat2.AltStreams.NumFiles != 0)
  {
    PrintSum(stat2.AltStreams, 0, kString_AltStreams);;
    CListStat stat = stat2.MainFiles;
    stat.Update(stat2.AltStreams);
    PrintSum(stat, 0, kString_Streams);
  }
}

static HRESULT GetUInt64Value(IInArchive *archive, UInt32 index, PROPID propID, CListUInt64Def &value)
{
  value.Val = 0;
  value.Def = false;
  CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop));
  value.Def = ConvertPropVariantToUInt64(prop, value.Val);
  return S_OK;
}

static HRESULT GetItemMTime(IInArchive *archive, UInt32 index, CListFileTimeDef &t)
{
  t.Val.dwLowDateTime = 0;
  t.Val.dwHighDateTime = 0;
  t.Def = false;
  CPropVariant prop;
  RINOK(archive->GetProperty(index, kpidMTime, &prop));
  if (prop.vt == VT_FILETIME)
  {
    t.Val = prop.filetime;
    t.Def = true;
  }
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}

static void PrintPropNameAndNumber(const char *name, UInt64 val)
{
  g_StdOut << name << ": " << val << endl;
}

static void PrintPropName_and_Eq(PROPID propID)
{
  const char *s;
  char temp[16];
  if (propID < ARRAY_SIZE(kPropIdToName))
    s = kPropIdToName[propID];
  else
  {
    ConvertUInt32ToString(propID, temp);
    s = temp;
  }
  g_StdOut << s << " = ";
}

static void PrintPropNameAndNumber(PROPID propID, UInt64 val)
{
  PrintPropName_and_Eq(propID);
  g_StdOut << val << endl;
}

static void PrintPropNameAndNumber_Signed(PROPID propID, Int64 val)
{
  PrintPropName_and_Eq(propID);
  g_StdOut << val << endl;
}

static void PrintPropPair(const char *name, const wchar_t *val)
{
  g_StdOut << name << " = " << val << endl;
}


static void PrintPropertyPair2(PROPID propID, const wchar_t *name, const CPropVariant &prop)
{
  UString s;
  ConvertPropertyToString(s, prop, propID);
  if (!s.IsEmpty())
  {
    AString nameA;
    UString nameU;
    GetPropName(propID, name, nameA, nameU);
    if (!nameA.IsEmpty())
      PrintPropPair(nameA, s);
    else
      g_StdOut << nameU << " = " << s << endl;
  }
}

static HRESULT PrintArcProp(IInArchive *archive, PROPID propID, const wchar_t *name)
{
  CPropVariant prop;
  RINOK(archive->GetArchiveProperty(propID, &prop));
  PrintPropertyPair2(propID, name, prop);
  return S_OK;
}

static void PrintArcTypeError(const UString &type, bool isWarning)
{
  g_StdOut << "Open " << (isWarning ? "Warning" : "Error")
    << ": Can not open the file as ["
    << type
    << "] archive"
    << endl;
}

int Find_FileName_InSortedVector(const UStringVector &fileName, const UString& name);

AString GetOpenArcErrorMessage(UInt32 errorFlags);

static void PrintErrorFlags(const char *s, UInt32 errorFlags)
{
  g_StdOut << s << endl << GetOpenArcErrorMessage(errorFlags) << endl;
}

static void ErrorInfo_Print(const CArcErrorInfo &er)
{
  if (er.AreThereErrors())
    PrintErrorFlags("Errors:", er.GetErrorFlags());
  if (!er.ErrorMessage.IsEmpty())
    PrintPropPair("Error", er.ErrorMessage);
  if (er.AreThereWarnings())
    PrintErrorFlags("Warnings:", er.GetWarningFlags());
  if (!er.WarningMessage.IsEmpty())
    PrintPropPair("Warning", er.WarningMessage);
}

HRESULT ListArchives(CCodecs *codecs,
    const CObjectVector<COpenType> &types,
    const CIntVector &excludedFormats,
    bool stdInMode,
    UStringVector &arcPaths, UStringVector &arcPathsFull,
    bool processAltStreams, bool showAltStreams,
    const NWildcard::CCensorNode &wildcardCensor,
    bool enableHeaders, bool techMode,
    #ifndef _NO_CRYPTO
    bool &passwordEnabled, UString &password,
    #endif
    #ifndef _SFX
    const CObjectVector<CProperty> *props,
    #endif
    UInt64 &numErrors,
    UInt64 &numWarnings)
{
  bool AllFilesAreAllowed = wildcardCensor.AreAllAllowed();

  numErrors = 0;
  numWarnings = 0;

  CFieldPrinter fp;
  if (!techMode)
    fp.Init(kStandardFieldTable, ARRAY_SIZE(kStandardFieldTable));

  CListStat2 stat2;
  
  CBoolArr skipArcs(arcPaths.Size());
  unsigned arcIndex;
  for (arcIndex = 0; arcIndex < arcPaths.Size(); arcIndex++)
    skipArcs[arcIndex] = false;
  UInt64 numVolumes = 0;
  UInt64 numArcs = 0;
  UInt64 totalArcSizes = 0;

  for (arcIndex = 0; arcIndex < arcPaths.Size(); arcIndex++)
  {
    if (skipArcs[arcIndex])
      continue;
    const UString &archiveName = arcPaths[arcIndex];
    UInt64 arcPackSize = 0;
    if (!stdInMode)
    {
      NFile::NFind::CFileInfo fi;
      if (!fi.Find(us2fs(archiveName)) || fi.IsDir())
      {
        g_StdOut << endl << "Error: " << archiveName << " is not file" << endl;
        numErrors++;
        continue;
      }
      arcPackSize = fi.Size;
      totalArcSizes += arcPackSize;
    }

    CArchiveLink arcLink;

    COpenCallbackConsole openCallback;
    openCallback.OutStream = &g_StdOut;

    #ifndef _NO_CRYPTO

    openCallback.PasswordIsDefined = passwordEnabled;
    openCallback.Password = password;

    #endif

    /*
    CObjectVector<COptionalOpenProperties> optPropsVector;
    COptionalOpenProperties &optProps = optPropsVector.AddNew();
    optProps.Props = *props;
    */
    
    COpenOptions options;
    #ifndef _SFX
    options.props = props;
    #endif
    options.codecs = codecs;
    options.types = &types;
    options.excludedFormats = &excludedFormats;
    options.stdInMode = stdInMode;
    options.stream = NULL;
    options.filePath = archiveName;
    HRESULT result = arcLink.Open2(options, &openCallback);

    if (result != S_OK)
    {
      if (result == E_ABORT)
        return result;
      g_StdOut << endl << "Error: " << archiveName << ": ";
      if (result == S_FALSE)
      {
        #ifndef _NO_CRYPTO
        if (openCallback.Open_WasPasswordAsked())
          g_StdOut << "Can not open encrypted archive. Wrong password?";
        else
        #endif
        {
          if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
          {
            PrintArcTypeError(codecs->Formats[arcLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name, false);
          }
          else
            g_StdOut << "Can not open the file as archive";
        }
        g_StdOut << endl;
        ErrorInfo_Print(arcLink.NonOpen_ErrorInfo);
      }
      else if (result == E_OUTOFMEMORY)
        g_StdOut << "Can't allocate required memory";
      else
        g_StdOut << NError::MyFormatMessage(result);
      g_StdOut << endl;
      numErrors++;
      continue;
    }
    {
      if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
        numErrors++;
      
      FOR_VECTOR (r, arcLink.Arcs)
      {
        const CArcErrorInfo &arc = arcLink.Arcs[r].ErrorInfo;
        if (!arc.WarningMessage.IsEmpty())
          numWarnings++;
        if (arc.AreThereWarnings())
          numWarnings++;
        if (arc.ErrorFormatIndex >= 0)
          numWarnings++;
        if (arc.AreThereErrors())
        {
          numErrors++;
          // break;
        }
        if (!arc.ErrorMessage.IsEmpty())
          numErrors++;
      }
    }

    numArcs++;
    numVolumes++;

    if (!stdInMode)
    {
      numVolumes += arcLink.VolumePaths.Size();
      totalArcSizes += arcLink.VolumesSize;
      FOR_VECTOR (v, arcLink.VolumePaths)
      {
        int index = Find_FileName_InSortedVector(arcPathsFull, arcLink.VolumePaths[v]);
        if (index >= 0 && (unsigned)index > arcIndex)
          skipArcs[index] = true;
      }
    }


    if (enableHeaders)
    {
      g_StdOut << endl << kListing << archiveName << endl << endl;

      FOR_VECTOR (r, arcLink.Arcs)
      {
        const CArc &arc = arcLink.Arcs[r];
        const CArcErrorInfo &er = arc.ErrorInfo;
        
        g_StdOut << "--\n";
        PrintPropPair("Path", arc.Path);
        if (er.ErrorFormatIndex >= 0)
        {
          if (er.ErrorFormatIndex == arc.FormatIndex)
            g_StdOut << "Warning: The archive is open with offset" << endl;
          else
            PrintArcTypeError(codecs->GetFormatNamePtr(er.ErrorFormatIndex), true);
        }
        PrintPropPair("Type", codecs->GetFormatNamePtr(arc.FormatIndex));

        ErrorInfo_Print(er);

        Int64 offset = arc.GetGlobalOffset();
        if (offset != 0)
          PrintPropNameAndNumber_Signed(kpidOffset, offset);
        IInArchive *archive = arc.Archive;
        RINOK(PrintArcProp(archive, kpidPhySize, NULL));
        if (er.TailSize != 0)
          PrintPropNameAndNumber(kpidTailSize, er.TailSize);
        UInt32 numProps;
        RINOK(archive->GetNumberOfArchiveProperties(&numProps));
        {
          for (UInt32 j = 0; j < numProps; j++)
          {
            CMyComBSTR name;
            PROPID propID;
            VARTYPE vt;
            RINOK(archive->GetArchivePropertyInfo(j, &name, &propID, &vt));
            RINOK(PrintArcProp(archive, propID, name));
          }
        }
        if (r != arcLink.Arcs.Size() - 1)
        {
          UInt32 numProps;
          g_StdOut << "----\n";
          if (archive->GetNumberOfProperties(&numProps) == S_OK)
          {
            UInt32 mainIndex = arcLink.Arcs[r + 1].SubfileIndex;
            for (UInt32 j = 0; j < numProps; j++)
            {
              CMyComBSTR name;
              PROPID propID;
              VARTYPE vt;
              RINOK(archive->GetPropertyInfo(j, &name, &propID, &vt));
              CPropVariant prop;
              RINOK(archive->GetProperty(mainIndex, propID, &prop));
              PrintPropertyPair2(propID, name, prop);
            }
          }
        }
      }
      g_StdOut << endl;
      if (techMode)
        g_StdOut << "----------\n";
    }

    if (enableHeaders && !techMode)
    {
      fp.PrintTitle();
      g_StdOut << endl;
      fp.PrintTitleLines();
      g_StdOut << endl;
    }

    const CArc &arc = arcLink.Arcs.Back();
    fp.Arc = &arc;
    fp.TechMode = techMode;
    IInArchive *archive = arc.Archive;
    if (techMode)
    {
      fp.Clear();
      RINOK(fp.AddMainProps(archive));
      if (arc.GetRawProps)
      {
        RINOK(fp.AddRawProps(arc.GetRawProps));
      }
    }
    
    CListStat2 stat;
    
    UInt32 numItems;
    RINOK(archive->GetNumberOfItems(&numItems));
    for (UInt32 i = 0; i < numItems; i++)
    {
      if (NConsoleClose::TestBreakSignal())
        return E_ABORT;

      HRESULT res = arc.GetItemPath2(i, fp.FilePath);

      if (stdInMode && res == E_INVALIDARG)
        break;
      RINOK(res);

      if (arc.Ask_Aux)
      {
        bool isAux;
        RINOK(Archive_IsItem_Aux(archive, i, isAux));
        if (isAux)
          continue;
      }

      bool isAltStream = false;
      if (arc.Ask_AltStream)
      {
        RINOK(Archive_IsItem_AltStream(archive, i, isAltStream));
        if (isAltStream && !processAltStreams)
          continue;
      }

      RINOK(Archive_IsItem_Folder(archive, i, fp.IsFolder));

      if (!AllFilesAreAllowed)
      {
        if (!wildcardCensor.CheckPath(isAltStream, fp.FilePath, !fp.IsFolder))
          continue;
      }
      
      CListStat st;
      
      RINOK(GetUInt64Value(archive, i, kpidSize, st.Size));
      RINOK(GetUInt64Value(archive, i, kpidPackSize, st.PackSize));
      RINOK(GetItemMTime(archive, i, st.MTime));

      if (fp.IsFolder)
        stat.NumDirs++;
      else
        st.NumFiles = 1;
      stat.GetStat(isAltStream).Update(st);

      if (isAltStream && !showAltStreams)
        continue;
      RINOK(fp.PrintItemInfo(i, st));
    }

    UInt64 numStreams = stat.GetNumStreams();
    if (!stdInMode
        && !stat.MainFiles.PackSize.Def
        && !stat.AltStreams.PackSize.Def)
    {
      if (arcLink.VolumePaths.Size() != 0)
        arcPackSize += arcLink.VolumesSize;
      stat.MainFiles.PackSize.Add((numStreams == 0) ? 0 : arcPackSize);
    }
    stat.MainFiles.SetSizeDefIfNoFiles();
    stat.AltStreams.SetSizeDefIfNoFiles();
    if (enableHeaders && !techMode)
    {
      fp.PrintTitleLines();
      g_StdOut << endl;
      fp.PrintSum(stat);
    }

    if (enableHeaders)
    {
      if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
      {
        g_StdOut << "----------\n";
        PrintPropPair("Path", arcLink.NonOpen_ArcPath);
        PrintArcTypeError(codecs->Formats[arcLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name, false);
      }
    }
    stat2.Update(stat);
    fflush(stdout);
  }
  if (enableHeaders && !techMode && (arcPaths.Size() > 1 || numVolumes > 1))
  {
    g_StdOut << endl;
    fp.PrintTitleLines();
    g_StdOut << endl;
    fp.PrintSum(stat2);
    g_StdOut << endl;
    PrintPropNameAndNumber("Archives", numArcs);
    PrintPropNameAndNumber("Volumes", numVolumes);
    PrintPropNameAndNumber("Total archives size", totalArcSizes);
  }
  return S_OK;
}
