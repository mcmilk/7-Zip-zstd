// ListArchive.cpp

#include "StdAfx.h"

#include "ListArchive.h"
#include "ConsoleCloseUtils.h"

#include "Common/StringConvert.h"
#include "Common/StdOutStream.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

#include "Windows\PropVariantConversions.h"

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

static CSysString GetAttributesString(DWORD anWinAttributes, bool aDirectory)
{
  CSysString s;
  //  s  = ((anWinAttributes & kLabelFileAttribute) != 0) ? 
  //                                kVolumeAttributeChar: kEmptyAttributeChar;
  s += ((anWinAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || aDirectory) ? 
                                kDirectoryAttributeChar: kEmptyAttributeChar;
  s += ((anWinAttributes & FILE_ATTRIBUTE_READONLY) != 0)? 
                                kReadonlyAttributeChar: kEmptyAttributeChar;
  s += ((anWinAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) ? 
                                kHiddenAttributeChar: kEmptyAttributeChar;
  s += ((anWinAttributes & FILE_ATTRIBUTE_SYSTEM) != 0) ? 
                                kSystemAttributeChar: kEmptyAttributeChar;
  s += ((anWinAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0) ? 
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
  AString Name;
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

CFieldInfoInit kStandardFieldTable[] = 
{
  { kaipidLastWriteTime, "   Date      Time", kLeft, kLeft, 0, 19 },
  { kaipidAttributes, "Attr", kRight, kCenter, 1, 5 },
  { kaipidSize, "Size", kRight, kRight, 1, 12 },
  { kaipidPackedSize, "Compressed", kRight, kRight, 1, 12 },
  { kaipidPath, "Name", kLeft, kLeft, 2, 12 }
};

void PrintSpaces(int aNumSpaces)
{
  for (int i = 0; i < aNumSpaces; i++)
    g_StdOut << ' ';
}

void PrintString(EAdjustment anAdjustment, int aWidth, const AString &aString)
{
  const aNumSpaces = aWidth - aString.Length();
  int aNumLeftSpaces;
  switch (anAdjustment)
  {
    case kLeft:
      aNumLeftSpaces = 0;
      break;
    case kCenter:
      aNumLeftSpaces = aNumSpaces / 2;
      break;
    case kRight:
      aNumLeftSpaces = aNumSpaces;
      break;
  }
  PrintSpaces(aNumLeftSpaces);
  g_StdOut << aString;
  PrintSpaces(aNumSpaces - aNumLeftSpaces);
}

class CFieldPrinter
{
  CObjectVector<CFieldInfo> m_Fields;
public:
  void Init(const CFieldInfoInit *aStandardFieldTable, int aNumItems);
  void PrintTitle();
  void PrintTitleLines();
  HRESULT PrintItemInfo(IArchiveHandler200 *anArchive, 
      const UString &aDefaultItemName,
      const NWindows::NFile::NFind::CFileInfo &anArchiveFileInfo,
      UINT32 anIndex);
  HRESULT PrintSummaryInfo(UINT64 aNumFiles, const UINT64 *aSize, const UINT64 *aFCompressedSize);
};

void CFieldPrinter::Init(const CFieldInfoInit *aStandardFieldTable, int aNumItems)
{
  for (int i = 0; i < aNumItems; i++)
  {
    CFieldInfo aFieldInfo;
    const CFieldInfoInit &aFieldInfoInit = aStandardFieldTable[i];
    aFieldInfo.PropID = aFieldInfoInit.PropID;
    aFieldInfo.Name = aFieldInfoInit.Name;
    aFieldInfo.TitleAdjustment = aFieldInfoInit.TitleAdjustment;
    aFieldInfo.TextAdjustment = aFieldInfoInit.TextAdjustment;
    aFieldInfo.PrefixSpacesWidth = aFieldInfoInit.PrefixSpacesWidth;
    aFieldInfo.Width = aFieldInfoInit.Width;
    m_Fields.Add(aFieldInfo);
  }
}

  
void CFieldPrinter::PrintTitle()
{
  for (int i = 0; i < m_Fields.Size(); i++)
  {
    const CFieldInfo &aFieldInfo = m_Fields[i];
    PrintSpaces(aFieldInfo.PrefixSpacesWidth);
    PrintString(aFieldInfo.TitleAdjustment, aFieldInfo.Width, aFieldInfo.Name);
  }
}

void CFieldPrinter::PrintTitleLines()
{
  for (int i = 0; i < m_Fields.Size(); i++)
  {
    const CFieldInfo &aFieldInfo = m_Fields[i];
    PrintSpaces(aFieldInfo.PrefixSpacesWidth);
    for (int i = 0; i < aFieldInfo.Width; i++)
      g_StdOut << '-';
  }
}


BOOL IsFileTimeZero(CONST FILETIME *lpFileTime)
{
  return (lpFileTime->dwLowDateTime == 0) && (lpFileTime->dwHighDateTime == 0);
}

void PrintTime(const NCOM::CPropVariant &aPropVariant)
{
  if (aPropVariant.vt != VT_FILETIME)
    throw "incorrect item";
  if (IsFileTimeZero(&aPropVariant.filetime))
  {
    printf("                ");
  }
  else
  {
    FILETIME aLocalFileTime;
    if (!FileTimeToLocalFileTime(&aPropVariant.filetime, &aLocalFileTime))
      throw "FileTimeToLocalFileTime error";
    SYSTEMTIME st;
    FileTimeToSystemTime(&aLocalFileTime, &st);
    printf("%04u-%02u-%02u %02u:%02u:%02u",
      st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
  }
}

HRESULT CFieldPrinter::PrintItemInfo(IArchiveHandler200 *anArchive, 
    const UString &aDefaultItemName, 
    const NWindows::NFile::NFind::CFileInfo &anArchiveFileInfo,
    UINT32 anIndex)
{
  for (int i = 0; i < m_Fields.Size(); i++)
  {
    const CFieldInfo &aFieldInfo = m_Fields[i];
    PrintSpaces(aFieldInfo.PrefixSpacesWidth);

    NCOM::CPropVariant aPropVariant;
    RETURN_IF_NOT_S_OK (anArchive->GetProperty(anIndex, 
        aFieldInfo.PropID, &aPropVariant));
    if (aPropVariant.vt == VT_EMPTY)
    {
      switch(aFieldInfo.PropID)
      {
        case kaipidPath:
          aPropVariant = aDefaultItemName;
          break;
        case kaipidLastWriteTime:
          aPropVariant = anArchiveFileInfo.LastWriteTime;
          break;
        default:
          PrintSpaces(aFieldInfo.Width);
          continue;
      }
    }

    if (aFieldInfo.PropID == kaipidLastWriteTime)
    {
      PrintTime(aPropVariant);
      continue;
    }
    if (aFieldInfo.PropID == kaipidAttributes)
    {
      if (aPropVariant.vt != VT_UI4)
        throw "incorrect item";
      UINT32 anAttributes = aPropVariant.ulVal;
      NCOM::CPropVariant aPropVariantIsFolder;
      RETURN_IF_NOT_S_OK(anArchive->GetProperty(anIndex, 
          kaipidIsFolder, &aPropVariantIsFolder));
      if(aPropVariantIsFolder.vt != VT_BOOL)
        return E_FAIL;
      g_StdOut << GetAttributesString(anAttributes, VARIANT_BOOLToBool(aPropVariantIsFolder.boolVal));
      continue;
    }

    if (aPropVariant.vt == VT_BSTR)
    {
      PrintString(aFieldInfo.TextAdjustment, aFieldInfo.Width, 
          GetSystemString(aPropVariant.bstrVal, CP_OEMCP));
      continue;
    }
    PrintString(aFieldInfo.TextAdjustment, aFieldInfo.Width, 
        ConvertPropertyToString(aPropVariant, aFieldInfo.PropID));
  }
  return S_OK;
}

static const char *kOneUINT64Format = "%I64u";

AString NumberToString(UINT64 aValue)
{
  char aTmp[32];
  sprintf(aTmp, kOneUINT64Format, aValue);
  return aTmp;
}

void PrintNumberString(EAdjustment anAdjustment, int aWidth, const UINT64 *aValue)
{
  AString aString;
  if (aValue != NULL)
    aString = NumberToString(*aValue);
  PrintString(anAdjustment, aWidth, aString);
}

static const char *kFilesMessage = "files";

HRESULT CFieldPrinter::PrintSummaryInfo(UINT64 aNumFiles, 
    const UINT64 *aSize, const UINT64 *aCompressedSize)
{
  for (int i = 0; i < m_Fields.Size(); i++)
  {
    const CFieldInfo &aFieldInfo = m_Fields[i];
    PrintSpaces(aFieldInfo.PrefixSpacesWidth);
    NCOM::CPropVariant aPropVariant;
    if (aFieldInfo.PropID == kaipidSize)
      PrintNumberString(aFieldInfo.TextAdjustment, aFieldInfo.Width, aSize);
    else if (aFieldInfo.PropID == kaipidPackedSize)
      PrintNumberString(aFieldInfo.TextAdjustment, aFieldInfo.Width, aCompressedSize);
    else if (aFieldInfo.PropID == kaipidPath)
    {
      AString aTmp = NumberToString(aNumFiles);
      aTmp += " ";
      aTmp += kFilesMessage;
      PrintString(aFieldInfo.TextAdjustment, aFieldInfo.Width, aTmp);
    }
    else 
      PrintString(aFieldInfo.TextAdjustment, aFieldInfo.Width, "");
  }
  return S_OK;
}

bool GetUINT64Value(IArchiveHandler200 *anArchive, UINT32 anIndex, 
    PROPID aPropID, UINT64 &aValue)
{
  NCOM::CPropVariant aPropVariant;
  if (anArchive->GetProperty(anIndex, aPropID, &aPropVariant) != S_OK)
    throw "GetPropertyValue error";
  if (aPropVariant.vt == VT_EMPTY)
    return false;
  aValue = ConvertPropVariantToUINT64(aPropVariant);
  return true;
}

HRESULT ListArchive(IArchiveHandler200 *anArchive, 
    const UString &aDefaultItemName,
    const NWindows::NFile::NFind::CFileInfo &anArchiveFileInfo,
    const NWildcard::CCensor &aWildcardCensor/*, bool aFullPathMode,
    NListMode::EEnum aMode*/)
{
  CFieldPrinter aFieldPrinter;
  aFieldPrinter.Init(kStandardFieldTable, sizeof(kStandardFieldTable) / sizeof(kStandardFieldTable[0]));
  aFieldPrinter.PrintTitle();
  g_StdOut << endl;
  aFieldPrinter.PrintTitleLines();
  g_StdOut << endl;

  // bool aNameFirst = (aFullPathMode && (aMode != NListMode::kDefault)) || (aMode == NListMode::kAll);

  UINT64 aNumFiles = 0, aTotalPackSize = 0, aTotalUnPackSize = 0;
  UINT64 *aTotalPackSizePointer = 0, *aTotalUnPackSizePointer = 0;
  UINT32 aNumItems;
  RETURN_IF_NOT_S_OK(anArchive->GetNumberOfItems(&aNumItems));
  for(int i = 0; i < aNumItems; i++)
  {
    if (NConsoleClose::TestBreakSignal())
      return E_ABORT;
    NCOM::CPropVariant aPropVariant;
    RETURN_IF_NOT_S_OK(anArchive->GetProperty(i, kaipidPath, &aPropVariant));
    UString aFilePath;
    if(aPropVariant.vt == VT_EMPTY)
      aFilePath = aDefaultItemName;
    else
    {
      if(aPropVariant.vt != VT_BSTR)
        return E_FAIL;
      aFilePath = aPropVariant.bstrVal;
    }
    if (!aWildcardCensor.CheckName(aFilePath))
      continue;

    aFieldPrinter.PrintItemInfo(anArchive, aDefaultItemName, anArchiveFileInfo, i);

    UINT64 aPackSize, anUnpackSize;
    if (!GetUINT64Value(anArchive, i, kaipidSize, anUnpackSize))
      anUnpackSize = 0;
    else
      aTotalUnPackSizePointer = &aTotalUnPackSize;
    if (!GetUINT64Value(anArchive, i, kaipidPackedSize, aPackSize))
      aPackSize = 0;
    else
      aTotalPackSizePointer = &aTotalPackSize;

    g_StdOut << endl;

    aNumFiles++;
    aTotalPackSize += aPackSize;
    aTotalUnPackSize += anUnpackSize;
  }
  aFieldPrinter.PrintTitleLines();
  g_StdOut << endl;
  /*
    if(aNumFiles == 0)
    printf(kStringFormat, kNoFilesMessage);
  else
  */
  aFieldPrinter.PrintSummaryInfo(aNumFiles, aTotalUnPackSizePointer, aTotalPackSizePointer);
  g_StdOut << endl;
  return S_OK;
}



