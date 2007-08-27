// List.cpp

#include "StdAfx.h"

#include "List.h"
#include "ConsoleClose.h"

#include "Common/StringConvert.h"
#include "Common/StdOutStream.h"
#include "Common/IntToString.h"
#include "Common/MyCom.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/FileDir.h"

#include "../../Archive/IArchive.h"

#include "../Common/PropIDUtils.h"
#include "../Common/OpenArchive.h"

#include "OpenCallbackConsole.h"

using namespace NWindows;

struct CPropIdToName
{
  PROPID PropID;
  const wchar_t *Name;
};

static CPropIdToName kPropIdToName[] =  
{
  { kpidPath, L"Path" },
  { kpidName, L"Name" },
  { kpidIsFolder, L"Folder" }, 
  { kpidSize, L"Size" },
  { kpidPackedSize, L"Packed Size" },
  { kpidAttributes, L"Attributes" },
  { kpidCreationTime, L"Created" },
  { kpidLastAccessTime, L"Accessed" },
  { kpidLastWriteTime, L"Modified" },
  { kpidSolid, L"Solid" },
  { kpidCommented, L"Commented" },
  { kpidEncrypted, L"Encrypted" },
  { kpidSplitBefore, L"Split Before" },
  { kpidSplitAfter, L"Split After" },
  { kpidDictionarySize, L"Dictionary Size" },
  { kpidCRC, L"CRC" },
  { kpidType, L"Type" },
  { kpidIsAnti, L"Anti" },
  { kpidMethod, L"Method" },
  { kpidHostOS, L"Host OS" },
  { kpidFileSystem, L"File System" },
  { kpidUser, L"User" },
  { kpidGroup, L"Group" },
  { kpidBlock, L"Block" },
  { kpidComment, L"Comment" },
  { kpidPosition, L"Position" },
  { kpidPrefix, L"Prefix" },
  { kpidNumSubFolders, L"Folders" },
  { kpidNumSubFiles, L"Files" },
  { kpidUnpackVer, L"Version" },
  { kpidVolume, L"Volume" },
  { kpidIsVolume, L"Multivolume" },
  { kpidOffset, L"Offset" },
  { kpidLinks, L"Links" },
  { kpidNumBlocks, L"Blocks" },
  { kpidNumVolumes, L"Volumes" }
};

static const char kEmptyAttributeChar = '.';
static const char kDirectoryAttributeChar = 'D';
static const char kReadonlyAttributeChar  = 'R';
static const char kHiddenAttributeChar    = 'H';
static const char kSystemAttributeChar    = 'S';
static const char kArchiveAttributeChar   = 'A';

static const char *kListing = "Listing archive: ";
static const wchar_t *kFilesMessage = L"files";
static const wchar_t *kDirsMessage = L"folders";

static void GetAttributesString(DWORD wa, bool directory, char *s)
{
  s[0] = ((wa & FILE_ATTRIBUTE_DIRECTORY) != 0 || directory) ? 
      kDirectoryAttributeChar: kEmptyAttributeChar;
  s[1] = ((wa & FILE_ATTRIBUTE_READONLY) != 0)? 
      kReadonlyAttributeChar: kEmptyAttributeChar;
  s[2] = ((wa & FILE_ATTRIBUTE_HIDDEN) != 0) ? 
      kHiddenAttributeChar: kEmptyAttributeChar;
  s[3] = ((wa & FILE_ATTRIBUTE_SYSTEM) != 0) ? 
      kSystemAttributeChar: kEmptyAttributeChar;
  s[4] = ((wa & FILE_ATTRIBUTE_ARCHIVE) != 0) ? 
      kArchiveAttributeChar: kEmptyAttributeChar;
  s[5] = '\0';
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
  UString Name;
  EAdjustment TitleAdjustment;
  EAdjustment TextAdjustment;
  int PrefixSpacesWidth;
  int Width;
};

struct CFieldInfoInit
{
  PROPID PropID;
  const wchar_t *Name;
  EAdjustment TitleAdjustment;
  EAdjustment TextAdjustment;
  int PrefixSpacesWidth;
  int Width;
};

CFieldInfoInit kStandardFieldTable[] = 
{
  { kpidLastWriteTime, L"   Date      Time", kLeft, kLeft, 0, 19 },
  { kpidAttributes, L"Attr", kRight, kCenter, 1, 5 },
  { kpidSize, L"Size", kRight, kRight, 1, 12 },
  { kpidPackedSize, L"Compressed", kRight, kRight, 1, 12 },
  { kpidPath, L"Name", kLeft, kLeft, 2, 24 }
};

void PrintSpaces(int numSpaces)
{
  for (int i = 0; i < numSpaces; i++)
    g_StdOut << ' ';
}

void PrintString(EAdjustment adjustment, int width, const UString &textString)
{
  const int numSpaces = width - textString.Length();
  int numLeftSpaces = 0;
  switch (adjustment)
  {
    case kLeft:
      numLeftSpaces = 0;
      break;
    case kCenter:
      numLeftSpaces = numSpaces / 2;
      break;
    case kRight:
      numLeftSpaces = numSpaces;
      break;
  }
  PrintSpaces(numLeftSpaces);
  g_StdOut << textString;
  PrintSpaces(numSpaces - numLeftSpaces);
}

class CFieldPrinter
{
  CObjectVector<CFieldInfo> _fields;
public:
  void Clear() { _fields.Clear(); }
  void Init(const CFieldInfoInit *standardFieldTable, int numItems);
  HRESULT Init(IInArchive *archive);
  void PrintTitle();
  void PrintTitleLines();
  HRESULT PrintItemInfo(IInArchive *archive, 
      const UString &defaultItemName,
      const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
      UInt32 index,
      bool techMode);
  HRESULT PrintSummaryInfo(UInt64 numFiles, UInt64 numDirs, 
      const UInt64 *size, const UInt64 *compressedSize);
};

void CFieldPrinter::Init(const CFieldInfoInit *standardFieldTable, int numItems)
{
  Clear();
  for (int i = 0; i < numItems; i++)
  {
    CFieldInfo fieldInfo;
    const CFieldInfoInit &fieldInfoInit = standardFieldTable[i];
    fieldInfo.PropID = fieldInfoInit.PropID;
    fieldInfo.Name = fieldInfoInit.Name;
    fieldInfo.TitleAdjustment = fieldInfoInit.TitleAdjustment;
    fieldInfo.TextAdjustment = fieldInfoInit.TextAdjustment;
    fieldInfo.PrefixSpacesWidth = fieldInfoInit.PrefixSpacesWidth;
    fieldInfo.Width = fieldInfoInit.Width;
    _fields.Add(fieldInfo);
  }
}

static UString GetPropName(PROPID propID, BSTR name)
{
  for (int i = 0; i < sizeof(kPropIdToName) / sizeof(kPropIdToName[0]); i++)
  {
    const CPropIdToName &propIdToName = kPropIdToName[i];
    if (propIdToName.PropID == propID)
      return propIdToName.Name;
  }
  if (name)
    return name;
  return L"?";
}

HRESULT CFieldPrinter::Init(IInArchive *archive)
{
  Clear();
  UInt32 numProps;
  RINOK(archive->GetNumberOfProperties(&numProps));
  for (UInt32 i = 0; i < numProps; i++)
  {
    CMyComBSTR name;
    PROPID propID;
    VARTYPE vt;
    RINOK(archive->GetPropertyInfo(i, &name, &propID, &vt));
    CFieldInfo fieldInfo;
    fieldInfo.PropID = propID;
    fieldInfo.Name = GetPropName(propID, name);
    _fields.Add(fieldInfo);
  }
  return S_OK;
}

void CFieldPrinter::PrintTitle()
{
  for (int i = 0; i < _fields.Size(); i++)
  {
    const CFieldInfo &fieldInfo = _fields[i];
    PrintSpaces(fieldInfo.PrefixSpacesWidth);
    PrintString(fieldInfo.TitleAdjustment, 
      ((fieldInfo.PropID == kpidPath) ? 0: fieldInfo.Width), fieldInfo.Name);
  }
}

void CFieldPrinter::PrintTitleLines()
{
  for (int i = 0; i < _fields.Size(); i++)
  {
    const CFieldInfo &fieldInfo = _fields[i];
    PrintSpaces(fieldInfo.PrefixSpacesWidth);
    for (int i = 0; i < fieldInfo.Width; i++)
      g_StdOut << '-';
  }
}


BOOL IsFileTimeZero(CONST FILETIME *lpFileTime)
{
  return (lpFileTime->dwLowDateTime == 0) && (lpFileTime->dwHighDateTime == 0);
}

static const char *kEmptyTimeString = "                   ";
void PrintTime(const NCOM::CPropVariant &propVariant)
{
  if (propVariant.vt != VT_FILETIME)
    throw "incorrect item";
  if (IsFileTimeZero(&propVariant.filetime))
    g_StdOut << kEmptyTimeString;
  else
  {
    FILETIME localFileTime;
    if (!FileTimeToLocalFileTime(&propVariant.filetime, &localFileTime))
      throw "FileTimeToLocalFileTime error";
    char s[32];
    if (ConvertFileTimeToString(localFileTime, s, true, true))
      g_StdOut << s;
    else
      g_StdOut << kEmptyTimeString;
  }
}

HRESULT CFieldPrinter::PrintItemInfo(IInArchive *archive, 
    const UString &defaultItemName, 
    const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
    UInt32 index,
    bool techMode)
{
  /*
  if (techMode)
  {
    g_StdOut << "Index = ";
    g_StdOut << (UInt64)index;
    g_StdOut << endl;
  }
  */
  for (int i = 0; i < _fields.Size(); i++)
  {
    const CFieldInfo &fieldInfo = _fields[i];
    if (!techMode)
      PrintSpaces(fieldInfo.PrefixSpacesWidth);

    NCOM::CPropVariant propVariant;
    RINOK(archive->GetProperty(index, fieldInfo.PropID, &propVariant));
    if (techMode)
    {
      g_StdOut << fieldInfo.Name << " = ";
    }
    int width = (fieldInfo.PropID == kpidPath) ? 0: fieldInfo.Width;
    if (propVariant.vt == VT_EMPTY)
    {
      switch(fieldInfo.PropID)
      {
        case kpidPath:
          propVariant = defaultItemName;
          break;
        case kpidLastWriteTime:
          propVariant = archiveFileInfo.LastWriteTime;
          break;
        default:
          if (techMode)
            g_StdOut << endl;
          else
            PrintSpaces(width);
          continue;
      }
    }
    if (fieldInfo.PropID == kpidLastWriteTime)
    {
      PrintTime(propVariant);
    }
    else if (fieldInfo.PropID == kpidAttributes)
    {
      if (propVariant.vt != VT_UI4)
        throw "incorrect item";
      UInt32 attributes = propVariant.ulVal;
      bool isFolder;
      RINOK(IsArchiveItemFolder(archive, index, isFolder));
      char s[8];
      GetAttributesString(attributes, isFolder, s);
      g_StdOut << s;
    }
    else if (propVariant.vt == VT_BSTR)
    {
      if (techMode)
        g_StdOut << propVariant.bstrVal;
      else
        PrintString(fieldInfo.TextAdjustment, width, propVariant.bstrVal);
    }
    else
    {
      UString s = ConvertPropertyToString(propVariant, fieldInfo.PropID);
      s.Replace(wchar_t(0xA), L' '); 
      s.Replace(wchar_t(0xD), L' '); 

      if (techMode)
        g_StdOut << s;
      else
        PrintString(fieldInfo.TextAdjustment, width, s);
    }
    if (techMode)
      g_StdOut << endl;
  }
  return S_OK;
}

void PrintNumberString(EAdjustment adjustment, int width, const UInt64 *value)
{
  wchar_t textString[32] = { 0 };
  if (value != NULL)
    ConvertUInt64ToString(*value, textString);
  PrintString(adjustment, width, textString);
}


HRESULT CFieldPrinter::PrintSummaryInfo(UInt64 numFiles, UInt64 numDirs, 
    const UInt64 *size, const UInt64 *compressedSize)
{
  for (int i = 0; i < _fields.Size(); i++)
  {
    const CFieldInfo &fieldInfo = _fields[i];
    PrintSpaces(fieldInfo.PrefixSpacesWidth);
    NCOM::CPropVariant propVariant;
    if (fieldInfo.PropID == kpidSize)
      PrintNumberString(fieldInfo.TextAdjustment, fieldInfo.Width, size);
    else if (fieldInfo.PropID == kpidPackedSize)
      PrintNumberString(fieldInfo.TextAdjustment, fieldInfo.Width, compressedSize);
    else if (fieldInfo.PropID == kpidPath)
    {
      wchar_t textString[32];
      ConvertUInt64ToString(numFiles, textString);
      UString temp = textString;
      temp += L" ";
      temp += kFilesMessage;
      temp += L", ";
      ConvertUInt64ToString(numDirs, textString);
      temp += textString;
      temp += L" ";
      temp += kDirsMessage;
      PrintString(fieldInfo.TextAdjustment, 0, temp);
    }
    else 
      PrintString(fieldInfo.TextAdjustment, fieldInfo.Width, L"");
  }
  return S_OK;
}

bool GetUInt64Value(IInArchive *archive, UInt32 index, PROPID propID, UInt64 &value)
{
  NCOM::CPropVariant propVariant;
  if (archive->GetProperty(index, propID, &propVariant) != S_OK)
    throw "GetPropertyValue error";
  if (propVariant.vt == VT_EMPTY)
    return false;
  value = ConvertPropVariantToUInt64(propVariant);
  return true;
}

HRESULT ListArchives(
    CCodecs *codecs,
    UStringVector &archivePaths, UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    bool enableHeaders, bool techMode, bool &passwordEnabled, UString &password, UInt64 &numErrors)
{
  numErrors = 0;
  CFieldPrinter fieldPrinter;
  if (!techMode)
    fieldPrinter.Init(kStandardFieldTable, sizeof(kStandardFieldTable) / sizeof(kStandardFieldTable[0]));

  UInt64 numFiles2 = 0, numDirs2 = 0, totalPackSize2 = 0, totalUnPackSize2 = 0;
  UInt64 *totalPackSizePointer2 = 0, *totalUnPackSizePointer2 = 0;
  for (int i = 0; i < archivePaths.Size(); i++)
  {
    const UString &archiveName = archivePaths[i];
    NFile::NFind::CFileInfoW archiveFileInfo;
    if (!NFile::NFind::FindFile(archiveName, archiveFileInfo) || archiveFileInfo.IsDirectory())
    {
      g_StdOut << endl << "Error: " << archiveName << " is not archive" << endl;
      numErrors++;
      continue;
    }
    if (archiveFileInfo.IsDirectory())
    {
      g_StdOut << endl << "Error: " << archiveName << " is not file" << endl;
      numErrors++;
      continue;
    }

    CArchiveLink archiveLink;

    COpenCallbackConsole openCallback;
    openCallback.OutStream = &g_StdOut;
    openCallback.PasswordIsDefined = passwordEnabled;
    openCallback.Password = password;

    HRESULT result = MyOpenArchive(codecs, archiveName, archiveLink, &openCallback);
    if (result != S_OK)
    {
      g_StdOut << endl << "Error: " << archiveName << " is not supported archive" << endl;
      numErrors++;
      continue;
    }

    for (int v = 0; v < archiveLink.VolumePaths.Size(); v++)
    {
      int index = archivePathsFull.FindInSorted(archiveLink.VolumePaths[v]);
      if (index >= 0 && index > i)
      {
        archivePaths.Delete(index);
        archivePathsFull.Delete(index);
      }
    }

    IInArchive *archive = archiveLink.GetArchive();
    const UString defaultItemName = archiveLink.GetDefaultItemName();

    if (enableHeaders)
    {
      g_StdOut << endl << kListing << archiveName << endl << endl;

      UInt32 numProps;
      if (archive->GetNumberOfArchiveProperties(&numProps) == S_OK)
      {
        for (UInt32 i = 0; i < numProps; i++)
        {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE vt;
          if (archive->GetArchivePropertyInfo(i, &name, &propID, &vt) != S_OK)
            continue;
          NCOM::CPropVariant prop;
          if (archive->GetArchiveProperty(propID, &prop) != S_OK)
            continue;
          UString s = ConvertPropertyToString(prop, propID);
          if (!s.IsEmpty())
            g_StdOut << GetPropName(propID, name) << " = " << s << endl;
        }
      }
      if (techMode)
        g_StdOut << "----------\n";
      if (numProps > 0)
        g_StdOut << endl;
    }

    if (enableHeaders && !techMode)
    {
      fieldPrinter.PrintTitle();
      g_StdOut << endl;
      fieldPrinter.PrintTitleLines();
      g_StdOut << endl;
    }

    if (techMode)
    {
      RINOK(fieldPrinter.Init(archive));
    }
    UInt64 numFiles = 0, numDirs = 0, totalPackSize = 0, totalUnPackSize = 0;
    UInt64 *totalPackSizePointer = 0, *totalUnPackSizePointer = 0;
    UInt32 numItems;
    RINOK(archive->GetNumberOfItems(&numItems));
    for(UInt32 i = 0; i < numItems; i++)
    {
      if (NConsoleClose::TestBreakSignal())
        return E_ABORT;

      UString filePath;
      RINOK(GetArchiveItemPath(archive, i, defaultItemName, filePath));

      bool isFolder;
      RINOK(IsArchiveItemFolder(archive, i, isFolder));
      if (!wildcardCensor.CheckPath(filePath, !isFolder))
        continue;
      
      fieldPrinter.PrintItemInfo(archive, defaultItemName, archiveFileInfo, i, techMode);
      
      UInt64 packSize, unpackSize;
      if (!GetUInt64Value(archive, i, kpidSize, unpackSize))
        unpackSize = 0;
      else
        totalUnPackSizePointer = &totalUnPackSize;
      if (!GetUInt64Value(archive, i, kpidPackedSize, packSize))
        packSize = 0;
      else
        totalPackSizePointer = &totalPackSize;
      
      g_StdOut << endl;

      if (isFolder)
        numDirs++;
      else
        numFiles++;
      totalPackSize += packSize;
      totalUnPackSize += unpackSize;
    }
    if (enableHeaders && !techMode)
    {
      fieldPrinter.PrintTitleLines();
      g_StdOut << endl;
      fieldPrinter.PrintSummaryInfo(numFiles, numDirs, totalUnPackSizePointer, totalPackSizePointer);
      g_StdOut << endl;
    }
    if (totalPackSizePointer != 0)
    {
      totalPackSizePointer2 = &totalPackSize2;
      totalPackSize2 += totalPackSize;
    }
    if (totalUnPackSizePointer != 0)
    {
      totalUnPackSizePointer2 = &totalUnPackSize2;
      totalUnPackSize2 += totalUnPackSize;
    }
    numFiles2 += numFiles;
    numDirs2 += numDirs;
  }
  if (enableHeaders && !techMode && archivePaths.Size() > 1)
  {
    g_StdOut << endl;
    fieldPrinter.PrintTitleLines();
    g_StdOut << endl;
    fieldPrinter.PrintSummaryInfo(numFiles2, numDirs2, totalUnPackSizePointer2, totalPackSizePointer2);
    g_StdOut << endl;
    g_StdOut << "Archives: " << archivePaths.Size() << endl;
  }
  return S_OK;
}
