// ViewSettings.h

#include "StdAfx.h"
 
#include "Common/IntToString.h"

#include "ViewSettings.h"
#include "Windows/Registry.h"
#include "Windows/Synchronization.h"

using namespace NWindows;
using namespace NRegistry;

static const TCHAR *kCUBasePath = TEXT("Software\\7-Zip\\FM");

static const TCHAR *kCulumnsKeyName = _T("Columns");

static const TCHAR *kPositionValueName = _T("Position");
static const TCHAR *kPanelsInfoValueName = _T("Panels");

static const TCHAR *kPanelPathValueName = _T("PanelPath");
static const TCHAR *kFolderHistoryValueName = _T("FolderHistory");
static const TCHAR *kFastFoldersValueName = _T("FolderShortcuts");
static const TCHAR *kCopyHistoryValueName = _T("CopyHistory");


#pragma pack( push, PragmaColumnInfoSpec)
#pragma pack( push, 1)

class CColumnInfoSpec
{
  UINT32 PropID;
  BYTE IsVisible;
  UINT32 Width;
public:
  void GetFromColumnInfo(const CColumnInfo &aSrc) 
  {
    PropID = aSrc.PropID;
    IsVisible = aSrc.IsVisible ? 1: 0;
    Width = aSrc.Width;
  }
  void PutColumnInfo(CColumnInfo &aDest) 
  {
    aDest.PropID = PropID;
    aDest.IsVisible = (IsVisible != 0);
    aDest.Width = Width;
  }
};

struct CColumnHeader
{
  UINT32 Version;
  // UINT32 SortIndex;
  UINT32 SortID;
  BYTE Ascending;
};

#pragma pack(pop)
#pragma pack(pop, PragmaColumnInfoSpec)

static const UINT32 kColumnInfoVersion = 0;

static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;

void SaveListViewInfo(const CSysString &id, const CListViewInfo &viewInfo)
{
  const CObjectVector<CColumnInfo> &columns = viewInfo.Columns;
  CByteBuffer buffer;
  UINT32 dataSize = sizeof(CColumnHeader) + sizeof(CColumnInfoSpec) * columns.Size();
  buffer.SetCapacity(dataSize);
  BYTE *dataPointer = (BYTE *)buffer;
  CColumnHeader &columnHeader = *(CColumnHeader *)dataPointer;
  columnHeader.Version = kColumnInfoVersion;

  // columnHeader.SortIndex = viewInfo.SortIndex;
  columnHeader.SortID = viewInfo.SortID;

  columnHeader.Ascending = viewInfo.Ascending ? 1 : 0;
  CColumnInfoSpec *destItems = (CColumnInfoSpec *)(dataPointer + sizeof(CColumnHeader));
  for(int i = 0; i < columns.Size(); i++)
  {
    CColumnInfoSpec &columnInfoSpec = destItems[i];
    columnInfoSpec.GetFromColumnInfo(columns[i]);
  }
  {
    NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
    CSysString keyName = kCUBasePath;
    keyName += kKeyNameDelimiter;
    keyName += kCulumnsKeyName;
    CKey key;
    key.Create(HKEY_CURRENT_USER, keyName);
    key.SetValue(id, dataPointer, dataSize);
  }
}

void ReadListViewInfo(const CSysString &id, CListViewInfo &viewInfo)
{
  viewInfo.Clear();
  CObjectVector<CColumnInfo> &columns = viewInfo.Columns;
  CByteBuffer buffer;
  UINT32 size;
  {
    NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
    CSysString keyName = kCUBasePath;
    keyName += kKeyNameDelimiter;
    keyName += kCulumnsKeyName;
    CKey key;
    if(key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) != ERROR_SUCCESS)
      return;
    if (key.QueryValue(id, buffer, size) != ERROR_SUCCESS)
      return;
  }
  if (size < sizeof(CColumnHeader))
    return;
  BYTE *dataPointer = (BYTE *)buffer;
  const CColumnHeader &columnHeader = *(CColumnHeader*)dataPointer;
  if (columnHeader.Version != kColumnInfoVersion)
    return;
  viewInfo.Ascending = (columnHeader.Ascending != 0);

  // viewInfo.SortIndex = columnHeader.SortIndex;
  viewInfo.SortID = columnHeader.SortID;

  size -= sizeof(CColumnHeader);
  if (size % sizeof(CColumnHeader) != 0)
    return;
  int numItems = size / sizeof(CColumnInfoSpec);
  CColumnInfoSpec *specItems = (CColumnInfoSpec *)(dataPointer + sizeof(CColumnHeader));;
  columns.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
  {
    CColumnInfo columnInfo;
    specItems[i].PutColumnInfo(columnInfo);
    columns.Add(columnInfo);
  }
}

#pragma pack( push, PragmaWindowPosition)
#pragma pack( push, 1)

struct CWindowPosition
{
  RECT Rect;
  UINT32 Maximized;
};

struct CPanelsInfo
{
  UINT32 NumPanels;
  UINT32 CurrentPanel;
  UINT32 SplitterPos;
};

#pragma pack(pop)
#pragma pack(pop, PragmaWindowPosition)

void SaveWindowSize(const RECT &rect, bool maximized)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  key.Create(HKEY_CURRENT_USER, keyName);
  CWindowPosition position;
  position.Rect = rect;
  position.Maximized = maximized ? 1: 0;
  key.SetValue(kPositionValueName, &position, sizeof(position));
}

bool ReadWindowSize(RECT &rect, bool &maximized)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  if(key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;
  CByteBuffer buffer;
  UINT32 size;
  if (key.QueryValue(kPositionValueName, buffer, size) != ERROR_SUCCESS)
    return false;
  if (size != sizeof(CWindowPosition))
    return false;
  const CWindowPosition &position = *(const CWindowPosition *)(const BYTE *)buffer;
  rect = position.Rect;
  maximized = (position.Maximized != 0);
  return true;
}

void SavePanelsInfo(UINT32 numPanels, UINT32 currentPanel, UINT32 splitterPos)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  key.Create(HKEY_CURRENT_USER, keyName);
  CPanelsInfo block;
  block.NumPanels = numPanels;
  block.CurrentPanel = currentPanel;
  block.SplitterPos = splitterPos;
  key.SetValue(kPanelsInfoValueName, &block, sizeof(block));
}

bool ReadPanelsInfo(UINT32 &numPanels, UINT32 &currentPanel, UINT32 &splitterPos)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  if(key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;
  CByteBuffer buffer;
  UINT32 size;
  if (key.QueryValue(kPanelsInfoValueName, buffer, size) != ERROR_SUCCESS)
    return false;
  if (size != sizeof(CPanelsInfo))
    return false;
  const CPanelsInfo &block = *(const CPanelsInfo *)(const BYTE *)buffer;
  numPanels = block.NumPanels;
  currentPanel = block.CurrentPanel;
  splitterPos = block.SplitterPos;
  return true;
}


static CSysString GetPanelPathName(UINT32 panelIndex)
{
  TCHAR panelString[32];
  _itot(panelIndex, panelString, 10);
  // ConvertUINT64ToString(panelIndex, panelString);
  return CSysString(kPanelPathValueName) + panelString;
}


void SavePanelPath(UINT32 panel, const CSysString &path)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  key.Create(HKEY_CURRENT_USER, keyName);
  key.SetValue(GetPanelPathName(panel), path);
}

bool ReadPanelPath(UINT32 panel, CSysString &path)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  if(key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;
  return (key.QueryValue(GetPanelPathName(panel), path) == ERROR_SUCCESS);
}

void SaveStringList(LPCTSTR valueName, const UStringVector &folders)
{
  UINT32 sizeInChars = 0;
  int i;
  for (i = 0; i < folders.Size(); i++)
    sizeInChars += folders[i].Length() + 1;
  CBuffer<wchar_t> buffer;
  buffer.SetCapacity(sizeInChars);
  int aPos = 0;
  for (i = 0; i < folders.Size(); i++)
  {
    wcscpy(buffer + aPos, folders[i]);
    aPos += folders[i].Length() + 1;
  }
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  key.Create(HKEY_CURRENT_USER, kCUBasePath);
  key.SetValue(valueName, buffer, sizeInChars * sizeof(wchar_t));
}

void ReadStringList(LPCTSTR valueName, UStringVector &folders)
{
  folders.Clear();
  CKey key;
  NSynchronization::CSingleLock lock(&g_RegistryOperationsCriticalSection, true);
  if(key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) != ERROR_SUCCESS)
    return;
  CByteBuffer buffer;
  UINT32 dataSize;
  if (key.QueryValue(valueName, buffer, dataSize) != ERROR_SUCCESS)
    return;
  if (dataSize % sizeof(wchar_t) != 0)
    return;
  const wchar_t *data = (const wchar_t *)(const BYTE  *)buffer;
  UINT32 sizeInChars = dataSize / sizeof(wchar_t);
  UString string;
  for (int i = 0; i < sizeInChars; i++)
  {
    wchar_t aChar = data[i];
    if (aChar == L'\0')
    {
      folders.Add(string);
      string.Empty();
    }
    else
      string += aChar;
  }
}

void SaveFolderHistory(const UStringVector &folders)
  { SaveStringList(kFolderHistoryValueName, folders); }
void ReadFolderHistory(UStringVector &folders)
  { ReadStringList(kFolderHistoryValueName, folders); }

void SaveFastFolders(const UStringVector &folders)
  { SaveStringList(kFastFoldersValueName, folders); }
void ReadFastFolders(UStringVector &folders)
  { ReadStringList(kFastFoldersValueName, folders); }

void SaveCopyHistory(const UStringVector &folders)
  { SaveStringList(kCopyHistoryValueName, folders); }
void ReadCopyHistory(UStringVector &folders)
  { ReadStringList(kCopyHistoryValueName, folders); }

void AddUniqueStringToHeadOfList(UStringVector &list, 
    const UString &string)
{
  for(int i = 0; i < list.Size();)
    if (string.CollateNoCase(list[i]) == 0)
      list.Delete(i);
    else
      i++;
  list.Insert(0, string);
}

