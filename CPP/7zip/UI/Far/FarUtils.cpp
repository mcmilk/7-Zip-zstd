// FarUtils.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#ifndef UNDER_CE
#include "Windows/Console.h"
#endif
#include "Windows/Defs.h"
#include "Windows/Error.h"

#include "FarUtils.h"

using namespace NWindows;

namespace NFar {

CStartupInfo g_StartupInfo;

void CStartupInfo::Init(const PluginStartupInfo &pluginStartupInfo,
    const CSysString &pluginNameForRegestry)
{
  m_Data = pluginStartupInfo;
  m_RegistryPath = GetSystemString(pluginStartupInfo.RootKey);
  m_RegistryPath += TEXT('\\');
  m_RegistryPath += pluginNameForRegestry;
}

const char *CStartupInfo::GetMsgString(int messageId)
{
  return (const char*)m_Data.GetMsg(m_Data.ModuleNumber, messageId);
}

int CStartupInfo::ShowMessage(unsigned int flags,
    const char *helpTopic, const char **items, int numItems, int numButtons)
{
  return m_Data.Message(m_Data.ModuleNumber, flags, (char *)helpTopic,
    (char **)items, numItems, numButtons);
}

namespace NMessageID
{
  enum
  {
    kOk,
    kCancel,
    kWarning,
    kError
  };
}

int CStartupInfo::ShowMessage(const char *message)
{
  const char *items[]= { GetMsgString(NMessageID::kError), message, GetMsgString(NMessageID::kOk) };
  return ShowMessage(FMSG_WARNING, NULL, items, sizeof(items) / sizeof(items[0]), 1);
}

static void SplitString(const AString &srcString, AStringVector &destStrings)
{
  destStrings.Clear();
  AString string;
  int len = srcString.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
  {
    char c = srcString[i];
    if (c == '\n')
    {
      if (!string.IsEmpty())
      {
        destStrings.Add(string);
        string.Empty();
      }
    }
    else
      string += c;
  }
  if (!string.IsEmpty())
    destStrings.Add(string);
}

int CStartupInfo::ShowMessageLines(const char *message)
{
  AStringVector strings;
  SplitString(message, strings);
  const int kNumStringsMax = 20;
  const char *items[kNumStringsMax + 1] = { GetMsgString(NMessageID::kError) };
  int pos = 1;
  for (int i = 0; i < strings.Size() && pos < kNumStringsMax; i++)
    items[pos++] = strings[i];
  items[pos++] = GetMsgString(NMessageID::kOk);

  return ShowMessage(FMSG_WARNING, NULL, items, pos, 1);
}

int CStartupInfo::ShowMessage(int messageId)
{
  return ShowMessage(GetMsgString(messageId));
}

int CStartupInfo::ShowDialog(int X1, int Y1, int X2, int Y2,
    const char *helpTopic, struct FarDialogItem *items, int numItems)
{
  return m_Data.Dialog(m_Data.ModuleNumber, X1, Y1, X2, Y2, (char *)helpTopic,
      items, numItems);
}

int CStartupInfo::ShowDialog(int sizeX, int sizeY,
    const char *helpTopic, struct FarDialogItem *items, int numItems)
{
  return ShowDialog(-1, -1, sizeX, sizeY, helpTopic, items, numItems);
}

inline static BOOL GetBOOLValue(bool v) { return (v? TRUE: FALSE); }

void CStartupInfo::InitDialogItems(const CInitDialogItem  *srcItems,
    FarDialogItem *destItems, int numItems)
{
  for (int i = 0; i < numItems; i++)
  {
    const CInitDialogItem &srcItem = srcItems[i];
    FarDialogItem &destItem = destItems[i];

    destItem.Type = srcItem.Type;
    destItem.X1 = srcItem.X1;
    destItem.Y1 = srcItem.Y1;
    destItem.X2 = srcItem.X2;
    destItem.Y2 = srcItem.Y2;
    destItem.Focus = GetBOOLValue(srcItem.Focus);
    if(srcItem.HistoryName != NULL)
      destItem.History = srcItem.HistoryName;
    else
      destItem.Selected = GetBOOLValue(srcItem.Selected);
    destItem.Flags = srcItem.Flags;
    destItem.DefaultButton = GetBOOLValue(srcItem.DefaultButton);

    if(srcItem.DataMessageId < 0)
      MyStringCopy(destItem.Data, srcItem.DataString);
    else
      MyStringCopy(destItem.Data, GetMsgString(srcItem.DataMessageId));

    /*
    if ((unsigned int)Init[i].Data < 0xFFF)
      MyStringCopy(destItem.Data, GetMsg((unsigned int)srcItem.Data));
    else
      MyStringCopy(destItem.Data,srcItem.Data);
    */
  }
}

// --------------------------------------------

HANDLE CStartupInfo::SaveScreen(int X1, int Y1, int X2, int Y2)
{
  return m_Data.SaveScreen(X1, Y1, X2, Y2);
}

HANDLE CStartupInfo::SaveScreen()
{
  return SaveScreen(0, 0, -1, -1);
}

void CStartupInfo::RestoreScreen(HANDLE handle)
{
  m_Data.RestoreScreen(handle);
}

const TCHAR kRegestryKeyDelimiter = TEXT('\'');

CSysString CStartupInfo::GetFullKeyName(const CSysString &keyName) const
{
  return (keyName.IsEmpty()) ? m_RegistryPath:
    (m_RegistryPath + kRegestryKeyDelimiter + keyName);
}


LONG CStartupInfo::CreateRegKey(HKEY parentKey,
    const CSysString &keyName, NRegistry::CKey &destKey) const
{
  return destKey.Create(parentKey, GetFullKeyName(keyName));
}

LONG CStartupInfo::OpenRegKey(HKEY parentKey,
    const CSysString &keyName, NRegistry::CKey &destKey) const
{
  return destKey.Open(parentKey, GetFullKeyName(keyName));
}

void CStartupInfo::SetRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, LPCTSTR value) const
{
  NRegistry::CKey regKey;
  CreateRegKey(parentKey, keyName, regKey);
  regKey.SetValue(valueName, value);
}

void CStartupInfo::SetRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, UINT32 value) const
{
  NRegistry::CKey regKey;
  CreateRegKey(parentKey, keyName, regKey);
  regKey.SetValue(valueName, value);
}

void CStartupInfo::SetRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, bool value) const
{
  NRegistry::CKey regKey;
  CreateRegKey(parentKey, keyName, regKey);
  regKey.SetValue(valueName, value);
}

CSysString CStartupInfo::QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, const CSysString &valueDefault) const
{
  NRegistry::CKey regKey;
  if (OpenRegKey(parentKey, keyName, regKey) != ERROR_SUCCESS)
    return valueDefault;
  
  CSysString value;
  if(regKey.QueryValue(valueName, value) != ERROR_SUCCESS)
    return valueDefault;
  
  return value;
}

UINT32 CStartupInfo::QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, UINT32 valueDefault) const
{
  NRegistry::CKey regKey;
  if (OpenRegKey(parentKey, keyName, regKey) != ERROR_SUCCESS)
    return valueDefault;
  
  UINT32 value;
  if(regKey.QueryValue(valueName, value) != ERROR_SUCCESS)
    return valueDefault;
  
  return value;
}

bool CStartupInfo::QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, bool valueDefault) const
{
  NRegistry::CKey regKey;
  if (OpenRegKey(parentKey, keyName, regKey) != ERROR_SUCCESS)
    return valueDefault;
  
  bool value;
  if(regKey.QueryValue(valueName, value) != ERROR_SUCCESS)
    return valueDefault;
  
  return value;
}

bool CStartupInfo::Control(HANDLE pluginHandle, int command, void *param)
{
  return BOOLToBool(m_Data.Control(pluginHandle, command, param));
}

bool CStartupInfo::ControlRequestActivePanel(int command, void *param)
{
  return Control(INVALID_HANDLE_VALUE, command, param);
}

bool CStartupInfo::ControlGetActivePanelInfo(PanelInfo &panelInfo)
{
  return ControlRequestActivePanel(FCTL_GETPANELINFO, &panelInfo);
}

bool CStartupInfo::ControlSetSelection(const PanelInfo &panelInfo)
{
  return ControlRequestActivePanel(FCTL_SETSELECTION, (void *)&panelInfo);
}

bool CStartupInfo::ControlGetActivePanelCurrentItemInfo(
    PluginPanelItem &pluginPanelItem)
{
  PanelInfo panelInfo;
  if(!ControlGetActivePanelInfo(panelInfo))
    return false;
  if(panelInfo.ItemsNumber <= 0)
    throw "There are no items";
  pluginPanelItem = panelInfo.PanelItems[panelInfo.CurrentItem];
  return true;
}

bool CStartupInfo::ControlGetActivePanelSelectedOrCurrentItems(
    CObjectVector<PluginPanelItem> &pluginPanelItems)
{
  pluginPanelItems.Clear();
  PanelInfo panelInfo;
  if(!ControlGetActivePanelInfo(panelInfo))
    return false;
  if(panelInfo.ItemsNumber <= 0)
    throw "There are no items";
  if (panelInfo.SelectedItemsNumber == 0)
    pluginPanelItems.Add(panelInfo.PanelItems[panelInfo.CurrentItem]);
  else
    for (int i = 0; i < panelInfo.SelectedItemsNumber; i++)
      pluginPanelItems.Add(panelInfo.SelectedItems[i]);
  return true;
}

bool CStartupInfo::ControlClearPanelSelection()
{
  PanelInfo panelInfo;
  if(!ControlGetActivePanelInfo(panelInfo))
    return false;
  for (int i = 0; i < panelInfo.ItemsNumber; i++)
    panelInfo.PanelItems[i].Flags &= ~PPIF_SELECTED;
  return ControlSetSelection(panelInfo);
}

////////////////////////////////////////////////
// menu function

int CStartupInfo::Menu(
    int x,
    int y,
    int maxHeight,
    unsigned int flags,
    const char *title,
    const char *aBottom,
    const char *helpTopic,
    int *breakKeys,
    int *breakCode,
    struct FarMenuItem *items,
    int numItems)
{
  return m_Data.Menu(m_Data.ModuleNumber, x, y, maxHeight, flags, (char *)title,
      (char *)aBottom, (char *)helpTopic, breakKeys, breakCode, items, numItems);
}

int CStartupInfo::Menu(
    unsigned int flags,
    const char *title,
    const char *helpTopic,
    struct FarMenuItem *items,
    int numItems)
{
  return Menu(-1, -1, 0, flags, title, NULL, helpTopic, NULL,
      NULL, items, numItems);
}

int CStartupInfo::Menu(
    unsigned int flags,
    const char *title,
    const char *helpTopic,
    const CSysStringVector &items,
    int selectedItem)
{
  CRecordVector<FarMenuItem> farMenuItems;
  for(int i = 0; i < items.Size(); i++)
  {
    FarMenuItem item;
    item.Checked = 0;
    item.Separator = 0;
    item.Selected = (i == selectedItem);
    CSysString reducedString = items[i].Left(sizeof(item.Text) / sizeof(item.Text[0]) - 1);
    MyStringCopy(item.Text, (const char *)GetOemString(reducedString));
    farMenuItems.Add(item);
  }
  return Menu(flags, title, helpTopic, &farMenuItems.Front(), farMenuItems.Size());
}


//////////////////////////////////
// CScreenRestorer

CScreenRestorer::~CScreenRestorer()
{
  Restore();
}
void CScreenRestorer::Save()
{
  if(m_Saved)
    return;
  m_HANDLE = g_StartupInfo.SaveScreen();
  m_Saved = true;
}

void CScreenRestorer::Restore()
{
  if(m_Saved)
  {
    g_StartupInfo.RestoreScreen(m_HANDLE);
    m_Saved = false;
  }
};

static AString DWORDToString(DWORD number)
{
  char buffer[32];
  _ultoa(number, buffer, 10);
  return buffer;
}

void PrintErrorMessage(const char *message, int code)
{
  AString tmp = message;
  tmp += " #";
  tmp += DWORDToString(code);
  g_StartupInfo.ShowMessage(tmp);
}

void PrintErrorMessage(const char *message, const char *text)
{
  AString tmp = message;
  tmp += ":\n";
  tmp += text;
  g_StartupInfo.ShowMessageLines(tmp);
}

void PrintErrorMessage(const char *message, const wchar_t *text)
{
  PrintErrorMessage(message, UnicodeStringToMultiByte(text, CP_OEMCP));
}

bool WasEscPressed()
{
  #ifdef UNDER_CE
  return false;
  #else
  NConsole::CIn inConsole;
  HANDLE handle = ::GetStdHandle(STD_INPUT_HANDLE);
  if(handle == INVALID_HANDLE_VALUE)
    return true;
  inConsole.Attach(handle);
  for (;;)
  {
    DWORD numEvents;
    if(!inConsole.GetNumberOfEvents(numEvents))
      return true;
    if(numEvents == 0)
      return false;

    INPUT_RECORD event;
    if(!inConsole.ReadEvent(event, numEvents))
      return true;
    if (event.EventType == KEY_EVENT &&
        event.Event.KeyEvent.bKeyDown &&
        event.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
      return true;
  }
  #endif
}

void ShowErrorMessage(DWORD errorCode)
{
  UString message;
  NError::MyFormatMessage(errorCode, message);
  message.Replace(L"\x0D", L"");
  message.Replace(L"\x0A", L" ");
  g_StartupInfo.ShowMessage(UnicodeStringToMultiByte(message, CP_OEMCP));
}

void ShowLastErrorMessage()
{
  ShowErrorMessage(::GetLastError());
}

}
