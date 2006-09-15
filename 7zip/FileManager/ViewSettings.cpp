// ViewSettings.h

#include "StdAfx.h"
 
#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "ViewSettings.h"
#include "Windows/Registry.h"
#include "Windows/Synchronization.h"

using namespace NWindows;
using namespace NRegistry;

static const TCHAR *kCUBasePath = TEXT("Software\\7-Zip\\FM");

static const TCHAR *kCulumnsKeyName = TEXT("Columns");

static const TCHAR *kPositionValueName = TEXT("Position");
static const TCHAR *kPanelsInfoValueName = TEXT("Panels");
static const TCHAR *kToolbars = TEXT("Toolbars");

static const WCHAR *kPanelPathValueName = L"PanelPath";
static const TCHAR *kListMode = TEXT("ListMode");
static const TCHAR *kFolderHistoryValueName = TEXT("FolderHistory");
static const TCHAR *kFastFoldersValueName = TEXT("FolderShortcuts");
static const TCHAR *kCopyHistoryValueName = TEXT("CopyHistory");

/*
class CColumnInfoSpec
{
  UInt32 PropID;
  Byte IsVisible;
  UInt32 Width;
};

struct CColumnHeader
{
  UInt32 Version;
  UInt32 SortID;
  Byte Ascending;
};
*/

static const UInt32 kColumnInfoSpecHeader = 12;
static const UInt32 kColumnHeaderSize = 12;

static const UInt32 kColumnInfoVersion = 1;

static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;

class CTempOutBufferSpec
{
  CByteBuffer Buffer;
  UInt32 Size;
  UInt32 Pos;
public:
  operator const Byte *() const { return (const Byte *)Buffer; }
  void Init(UInt32 dataSize)
  {
    Buffer.SetCapacity(dataSize);
    Size = dataSize;
    Pos = 0;
  }
  void WriteByte(Byte value)
  {
    if (Pos >= Size)
      throw "overflow";
    ((Byte *)Buffer)[Pos++] = value;
  }
  void WriteUInt32(UInt32 value)
  {
    for (int i = 0; i < 4; i++)
    {
      WriteByte((Byte)value);
      value >>= 8;
    }
  }
  void WriteBool(bool value)
  {
    WriteUInt32(value ? 1 : 0);
  }
};

class CTempInBufferSpec
{
public:
  Byte *Buffer;
  UInt32 Size;
  UInt32 Pos;
  Byte ReadByte()
  {
    if (Pos >= Size)
      throw "overflow";
    return Buffer[Pos++];
  }
  UInt32 ReadUInt32()
  {
    UInt32 value = 0;
    for (int i = 0; i < 4; i++)
      value |= (((UInt32)ReadByte()) << (8 * i));
    return value;
  }
  bool ReadBool()
  {
    return (ReadUInt32() != 0);
  }
};

void SaveListViewInfo(const UString &id, const CListViewInfo &viewInfo)
{
  const CObjectVector<CColumnInfo> &columns = viewInfo.Columns;
  CTempOutBufferSpec buffer;
  UInt32 dataSize = kColumnHeaderSize + kColumnInfoSpecHeader * columns.Size();
  buffer.Init(dataSize);

  buffer.WriteUInt32(kColumnInfoVersion);
  buffer.WriteUInt32(viewInfo.SortID);
  buffer.WriteBool(viewInfo.Ascending);
  for(int i = 0; i < columns.Size(); i++)
  {
    const CColumnInfo &column = columns[i];
    buffer.WriteUInt32(column.PropID);
    buffer.WriteBool(column.IsVisible);
    buffer.WriteUInt32(column.Width);
  }
  {
    NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
    CSysString keyName = kCUBasePath;
    keyName += kKeyNameDelimiter;
    keyName += kCulumnsKeyName;
    CKey key;
    key.Create(HKEY_CURRENT_USER, keyName);
    key.SetValue(GetSystemString(id), (const Byte *)buffer, dataSize);
  }
}

void ReadListViewInfo(const UString &id, CListViewInfo &viewInfo)
{
  viewInfo.Clear();
  CObjectVector<CColumnInfo> &columns = viewInfo.Columns;
  CByteBuffer buffer;
  UInt32 size;
  {
    NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
    CSysString keyName = kCUBasePath;
    keyName += kKeyNameDelimiter;
    keyName += kCulumnsKeyName;
    CKey key;
    if(key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) != ERROR_SUCCESS)
      return;
    if (key.QueryValue(GetSystemString(id), buffer, size) != ERROR_SUCCESS)
      return;
  }
  if (size < kColumnHeaderSize)
    return;
  CTempInBufferSpec inBuffer;
  inBuffer.Size = size;
  inBuffer.Buffer = (Byte *)buffer;
  inBuffer.Pos = 0;


  UInt32 version = inBuffer.ReadUInt32();
  if (version != kColumnInfoVersion)
    return;
  viewInfo.SortID = inBuffer.ReadUInt32();
  viewInfo.Ascending = inBuffer.ReadBool();

  size -= kColumnHeaderSize;
  if (size % kColumnInfoSpecHeader != 0)
    return;
  int numItems = size / kColumnInfoSpecHeader;
  columns.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
  {
    CColumnInfo columnInfo;
    columnInfo.PropID = inBuffer.ReadUInt32();
    columnInfo.IsVisible = inBuffer.ReadBool();
    columnInfo.Width = inBuffer.ReadUInt32();
    columns.Add(columnInfo);
  }
}

static const UInt32 kWindowPositionHeaderSize = 5 * 4;
static const UInt32 kPanelsInfoHeaderSize = 3 * 4;

/*
struct CWindowPosition
{
  RECT Rect;
  UInt32 Maximized;
};

struct CPanelsInfo
{
  UInt32 NumPanels;
  UInt32 CurrentPanel;
  UInt32 SplitterPos;
};
*/

void SaveWindowSize(const RECT &rect, bool maximized)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  key.Create(HKEY_CURRENT_USER, keyName);
  // CWindowPosition position;
  CTempOutBufferSpec buffer;
  buffer.Init(kWindowPositionHeaderSize);
  buffer.WriteUInt32(rect.left);
  buffer.WriteUInt32(rect.top);
  buffer.WriteUInt32(rect.right);
  buffer.WriteUInt32(rect.bottom);
  buffer.WriteBool(maximized);
  key.SetValue(kPositionValueName, (const Byte *)buffer, kWindowPositionHeaderSize);
}

bool ReadWindowSize(RECT &rect, bool &maximized)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  if(key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;
  CByteBuffer buffer;
  UInt32 size;
  if (key.QueryValue(kPositionValueName, buffer, size) != ERROR_SUCCESS)
    return false;
  if (size != kWindowPositionHeaderSize)
    return false;
  CTempInBufferSpec inBuffer;
  inBuffer.Size = size;
  inBuffer.Buffer = (Byte *)buffer;
  inBuffer.Pos = 0;
  rect.left = inBuffer.ReadUInt32();
  rect.top = inBuffer.ReadUInt32();
  rect.right = inBuffer.ReadUInt32();
  rect.bottom = inBuffer.ReadUInt32();
  maximized = inBuffer.ReadBool();
  return true;
}

void SavePanelsInfo(UInt32 numPanels, UInt32 currentPanel, UInt32 splitterPos)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  key.Create(HKEY_CURRENT_USER, keyName);

  CTempOutBufferSpec buffer;
  buffer.Init(kPanelsInfoHeaderSize);
  buffer.WriteUInt32(numPanels);
  buffer.WriteUInt32(currentPanel);
  buffer.WriteUInt32(splitterPos);
  key.SetValue(kPanelsInfoValueName, (const Byte *)buffer, kPanelsInfoHeaderSize);
}

bool ReadPanelsInfo(UInt32 &numPanels, UInt32 &currentPanel, UInt32 &splitterPos)
{
  CSysString keyName = kCUBasePath;
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  if(key.Open(HKEY_CURRENT_USER, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;
  CByteBuffer buffer;
  UInt32 size;
  if (key.QueryValue(kPanelsInfoValueName, buffer, size) != ERROR_SUCCESS)
    return false;
  if (size != kPanelsInfoHeaderSize)
    return false;
  CTempInBufferSpec inBuffer;
  inBuffer.Size = size;
  inBuffer.Buffer = (Byte *)buffer;
  inBuffer.Pos = 0;
  numPanels = inBuffer.ReadUInt32();
  currentPanel = inBuffer.ReadUInt32();
  splitterPos = inBuffer.ReadUInt32();
  return true;
}

void SaveToolbarsMask(UInt32 toolbarMask)
{
  CKey key;
  key.Create(HKEY_CURRENT_USER, kCUBasePath);
  key.SetValue(kToolbars, toolbarMask);
}

static const UInt32 kDefaultToolbarMask = 8 | 4 | 1;

UInt32 ReadToolbarsMask()
{
  CKey key;
  if(key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) != ERROR_SUCCESS)
    return kDefaultToolbarMask;
  UInt32 mask;
  if (key.QueryValue(kToolbars, mask) != ERROR_SUCCESS)
    return kDefaultToolbarMask;
  return mask;
}


static UString GetPanelPathName(UInt32 panelIndex)
{
  WCHAR panelString[32];
  ConvertUInt64ToString(panelIndex, panelString);
  return UString(kPanelPathValueName) + panelString;
}


void SavePanelPath(UInt32 panel, const UString &path)
{
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  key.Create(HKEY_CURRENT_USER, kCUBasePath);
  key.SetValue(GetPanelPathName(panel), path);
}

bool ReadPanelPath(UInt32 panel, UString &path)
{
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  if(key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) != ERROR_SUCCESS)
    return false;
  return (key.QueryValue(GetPanelPathName(panel), path) == ERROR_SUCCESS);
}

void SaveListMode(const CListMode &listMode)
{
  CKey key;
  key.Create(HKEY_CURRENT_USER, kCUBasePath);
  UInt32 t = 0;
  for (int i = 0; i < 2; i++)
    t |= ((listMode.Panels[i]) & 0xFF) << (i * 8);
  key.SetValue(kListMode, t);
}

void ReadListMode(CListMode &listMode)
{
  CKey key;
  listMode.Init();
  if(key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) != ERROR_SUCCESS)
    return;
  UInt32 t;
  if (key.QueryValue(kListMode, t) != ERROR_SUCCESS)
    return;
  for (int i = 0; i < 2; i++)
  {
    listMode.Panels[i] = (t & 0xFF);
    t >>= 8;
  }
}


void SaveStringList(LPCTSTR valueName, const UStringVector &folders)
{
  UInt32 sizeInChars = 0;
  int i;
  for (i = 0; i < folders.Size(); i++)
    sizeInChars += folders[i].Length() + 1;
  CBuffer<wchar_t> buffer;
  buffer.SetCapacity(sizeInChars);
  int pos = 0;
  for (i = 0; i < folders.Size(); i++)
  {
    MyStringCopy((wchar_t *)buffer + pos, (const wchar_t *)folders[i]);
    pos += folders[i].Length() + 1;
  }
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  key.Create(HKEY_CURRENT_USER, kCUBasePath);
  key.SetValue(valueName, buffer, sizeInChars * sizeof(wchar_t));
}

void ReadStringList(LPCTSTR valueName, UStringVector &folders)
{
  folders.Clear();
  CKey key;
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  if(key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) != ERROR_SUCCESS)
    return;
  CByteBuffer buffer;
  UInt32 dataSize;
  if (key.QueryValue(valueName, buffer, dataSize) != ERROR_SUCCESS)
    return;
  if (dataSize % sizeof(wchar_t) != 0)
    return;
  const wchar_t *data = (const wchar_t *)(const Byte  *)buffer;
  int sizeInChars = dataSize / sizeof(wchar_t);
  UString string;
  for (int i = 0; i < sizeInChars; i++)
  {
    wchar_t c = data[i];
    if (c == L'\0')
    {
      folders.Add(string);
      string.Empty();
    }
    else
      string += c;
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
    if (string.CompareNoCase(list[i]) == 0)
      list.Delete(i);
    else
      i++;
  list.Insert(0, string);
}

