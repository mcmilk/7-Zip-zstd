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

#include "../Common/PropIDUtils.h"

using namespace NWindows;

/*
static const char kEmptyFlag = '.';

static const char kPasswordFlag    = '*';
static const char kSolidFlag       = 'S';
static const char kSplitBeforeFlag = 'B';
static const char kSplitAfterFlag  = 'A';
static const char kCommentedFlag   = 'C';
*/

static const char kEmptyAttributeChar = '.';
//static const char kVolumeAttributeChar    = 'V';
static const char kDirectoryAttributeChar = 'D';
static const char kReadonlyAttributeChar  = 'R';
static const char kHiddenAttributeChar    = 'H';
static const char kSystemAttributeChar    = 'S';
static const char kArchiveAttributeChar   = 'A';

static AString GetAttributesString(DWORD winAttributes, bool directory)
{
  AString s;
  //  s  = ((winAttributes & kLabelFileAttribute) != 0) ? 
  //                                kVolumeAttributeChar: kEmptyAttributeChar;
  s += ((winAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || directory) ? 
                                kDirectoryAttributeChar: kEmptyAttributeChar;
  s += ((winAttributes & FILE_ATTRIBUTE_READONLY) != 0)? 
                                kReadonlyAttributeChar: kEmptyAttributeChar;
  s += ((winAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) ? 
                                kHiddenAttributeChar: kEmptyAttributeChar;
  s += ((winAttributes & FILE_ATTRIBUTE_SYSTEM) != 0) ? 
                                kSystemAttributeChar: kEmptyAttributeChar;
  s += ((winAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0) ? 
                                kArchiveAttributeChar: kEmptyAttributeChar;
  return s;
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
  { kpidPath, L"Name", kLeft, kLeft, 2, 12 }
};

void PrintSpaces(int numSpaces)
{
  for (int i = 0; i < numSpaces; i++)
    g_StdOut << ' ';
}

void PrintString(EAdjustment adjustment, int width, const UString &textString)
{
  const int numSpaces = width - textString.Length();
  int numLeftSpaces;
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
  void Init(const CFieldInfoInit *standardFieldTable, int numItems);
  void PrintTitle();
  void PrintTitleLines();
  HRESULT PrintItemInfo(IInArchive *archive, 
      const UString &defaultItemName,
      const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
      UINT32 index);
  HRESULT PrintSummaryInfo(UINT64 numFiles, const UINT64 *size, 
      const UINT64 *compressedSize);
};

void CFieldPrinter::Init(const CFieldInfoInit *standardFieldTable, int numItems)
{
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

  
void CFieldPrinter::PrintTitle()
{
  for (int i = 0; i < _fields.Size(); i++)
  {
    const CFieldInfo &fieldInfo = _fields[i];
    PrintSpaces(fieldInfo.PrefixSpacesWidth);
    PrintString(fieldInfo.TitleAdjustment, fieldInfo.Width, fieldInfo.Name);
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


const char *kEmptyTimeString = "                   ";
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
    SYSTEMTIME st;
    if (FileTimeToSystemTime(&localFileTime, &st))
    {
      char s[32];
      wsprintfA(s, "%04u-%02u-%02u %02u:%02u:%02u",
          st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
      g_StdOut << s;
    }
    else
      g_StdOut << kEmptyTimeString;
  }
}

HRESULT CFieldPrinter::PrintItemInfo(IInArchive *archive, 
    const UString &defaultItemName, 
    const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
    UINT32 index)
{
  for (int i = 0; i < _fields.Size(); i++)
  {
    const CFieldInfo &fieldInfo = _fields[i];
    PrintSpaces(fieldInfo.PrefixSpacesWidth);

    NCOM::CPropVariant propVariant;
    RINOK(archive->GetProperty(index, 
        fieldInfo.PropID, &propVariant));
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
          PrintSpaces(fieldInfo.Width);
          continue;
      }
    }

    if (fieldInfo.PropID == kpidLastWriteTime)
    {
      PrintTime(propVariant);
      continue;
    }
    if (fieldInfo.PropID == kpidAttributes)
    {
      if (propVariant.vt != VT_UI4)
        throw "incorrect item";
      UINT32 attributes = propVariant.ulVal;
      NCOM::CPropVariant propVariantIsFolder;
      RINOK(archive->GetProperty(index, 
          kpidIsFolder, &propVariantIsFolder));
      if(propVariantIsFolder.vt != VT_BOOL)
        return E_FAIL;
      g_StdOut << GetAttributesString(attributes, VARIANT_BOOLToBool(propVariantIsFolder.boolVal));
      continue;
    }

    if (propVariant.vt == VT_BSTR)
    {
      PrintString(fieldInfo.TextAdjustment, fieldInfo.Width, propVariant.bstrVal);
      continue;
    }
    PrintString(fieldInfo.TextAdjustment, fieldInfo.Width, 
        ConvertPropertyToString(propVariant, fieldInfo.PropID));
  }
  return S_OK;
}

void PrintNumberString(EAdjustment adjustment, int width, const UINT64 *value)
{
  wchar_t textString[32] = { 0 };
  if (value != NULL)
    ConvertUINT64ToString(*value, textString);
  PrintString(adjustment, width, textString);
}

static const wchar_t *kFilesMessage = L"files";

HRESULT CFieldPrinter::PrintSummaryInfo(UINT64 numFiles, 
    const UINT64 *size, const UINT64 *compressedSize)
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
      ConvertUINT64ToString(numFiles, textString);
      UString temp = textString;
      temp += L" ";
      temp += kFilesMessage;
      PrintString(fieldInfo.TextAdjustment, fieldInfo.Width, temp);
    }
    else 
      PrintString(fieldInfo.TextAdjustment, fieldInfo.Width, L"");
  }
  return S_OK;
}

bool GetUINT64Value(IInArchive *archive, UINT32 index, 
    PROPID propID, UINT64 &value)
{
  NCOM::CPropVariant propVariant;
  if (archive->GetProperty(index, propID, &propVariant) != S_OK)
    throw "GetPropertyValue error";
  if (propVariant.vt == VT_EMPTY)
    return false;
  value = ConvertPropVariantToUINT64(propVariant);
  return true;
}

HRESULT ListArchive(IInArchive *archive, 
    const UString &defaultItemName,
    const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
    const NWildcard::CCensor &wildcardCensor/*, bool fullPathMode,
    NListMode::EEnum mode*/)
{
  CFieldPrinter fieldPrinter;
  fieldPrinter.Init(kStandardFieldTable, sizeof(kStandardFieldTable) / sizeof(kStandardFieldTable[0]));
  fieldPrinter.PrintTitle();
  g_StdOut << endl;
  fieldPrinter.PrintTitleLines();
  g_StdOut << endl;

  // bool nameFirst = (fullPathMode && (mode != NListMode::kDefault)) || (mode == NListMode::kAll);

  UINT64 numFiles = 0, totalPackSize = 0, totalUnPackSize = 0;
  UINT64 *totalPackSizePointer = 0, *totalUnPackSizePointer = 0;
  UINT32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));
  for(UINT32 i = 0; i < numItems; i++)
  {
    if (NConsoleClose::TestBreakSignal())
      return E_ABORT;
    NCOM::CPropVariant propVariant;
    RINOK(archive->GetProperty(i, kpidPath, &propVariant));
    UString filePath;
    if(propVariant.vt == VT_EMPTY)
      filePath = defaultItemName;
    else
    {
      if(propVariant.vt != VT_BSTR)
        return E_FAIL;
      filePath = propVariant.bstrVal;
    }
    if (!wildcardCensor.CheckName(filePath))
      continue;

    fieldPrinter.PrintItemInfo(archive, defaultItemName, archiveFileInfo, i);

    UINT64 packSize, unpackSize;
    if (!GetUINT64Value(archive, i, kpidSize, unpackSize))
      unpackSize = 0;
    else
      totalUnPackSizePointer = &totalUnPackSize;
    if (!GetUINT64Value(archive, i, kpidPackedSize, packSize))
      packSize = 0;
    else
      totalPackSizePointer = &totalPackSize;

    g_StdOut << endl;

    numFiles++;
    totalPackSize += packSize;
    totalUnPackSize += unpackSize;
  }
  fieldPrinter.PrintTitleLines();
  g_StdOut << endl;
  /*
  if(numFiles == 0)
      g_StdOut << kNoFilesMessage);
  else
  */
  fieldPrinter.PrintSummaryInfo(numFiles, totalUnPackSizePointer, totalPackSizePointer);
  g_StdOut << endl;
  return S_OK;
}



