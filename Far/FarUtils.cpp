// FarUtils.cpp

#include "StdAfx.h"

#include "FarUtils.h"
#include "Common/DynamicBuffer.h"
#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/Console.h"
#include "Windows/Error.h"

using namespace NWindows;
using namespace std;

namespace NFar {

CStartupInfo g_StartupInfo;


void CStartupInfo::Init(const PluginStartupInfo &aPluginStartupInfo, 
    const CSysString &aPliginNameForRegestry)
{
  m_Data = aPluginStartupInfo;
  m_RegistryPath = aPluginStartupInfo.RootKey;
  m_RegistryPath += '\\';
  m_RegistryPath += aPliginNameForRegestry;
}

const char *CStartupInfo::GetMsgString(int aMessageId)
{
  return (const char*)m_Data.GetMsg(m_Data.ModuleNumber, aMessageId);
}

int CStartupInfo::ShowMessage(unsigned int aFlags, 
    const char *aHelpTopic, const char **anItems, int anItemsNumber, int aButtonsNumber)
{
  return m_Data.Message(m_Data.ModuleNumber, aFlags, (char *)aHelpTopic, 
    (char **)anItems, anItemsNumber, aButtonsNumber);
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

int CStartupInfo::ShowMessage(const char *aMessage)
{
  const char *aMessagesItems[]= { GetMsgString(NMessageID::kError), aMessage, 
      GetMsgString(NMessageID::kOk) };
  return ShowMessage(FMSG_WARNING, NULL, aMessagesItems, 
      sizeof(aMessagesItems) / sizeof(aMessagesItems[0]), 1);
}

int CStartupInfo::ShowMessage(int aMessageId)
{
  return ShowMessage(GetMsgString(aMessageId));
}

int CStartupInfo::ShowDialog(int X1, int Y1, int X2, int Y2, 
    const char *aHelpTopic, struct FarDialogItem *anItems, int aNumItems)
{
  return m_Data.Dialog(m_Data.ModuleNumber, X1, Y1, X2, Y2, (char *)aHelpTopic, 
      anItems, aNumItems);
}

int CStartupInfo::ShowDialog(int aSizeX, int aSizeY,
    const char *aHelpTopic, struct FarDialogItem *anItems, int aNumItems)
{
  return ShowDialog(-1, -1, aSizeX, aSizeY, aHelpTopic, anItems, aNumItems);
}

inline static BOOL GetBOOLValue(bool aBool)
{
  return (aBool? TRUE: FALSE);
}

void CStartupInfo::InitDialogItems(const CInitDialogItem  *aSrcItems, 
    FarDialogItem *aDestItems, int aNum)
{
  for (int i = 0; i < aNum; i++)
  {
    const CInitDialogItem &aSrcItem = aSrcItems[i];
    FarDialogItem &aDestItem = aDestItems[i];

    aDestItem.Type = aSrcItem.Type;
    aDestItem.X1 = aSrcItem.X1;
    aDestItem.Y1 = aSrcItem.Y1;
    aDestItem.X2 = aSrcItem.X2;
    aDestItem.Y2 = aSrcItem.Y2;
    aDestItem.Focus = GetBOOLValue(aSrcItem.Focus);
    if(aSrcItem.HistoryName != NULL)
      aDestItem.Selected = int(aSrcItem.HistoryName);
    else
      aDestItem.Selected = GetBOOLValue(aSrcItem.Selected);
    aDestItem.Flags = aSrcItem.Flags;
    aDestItem.DefaultButton = GetBOOLValue(aSrcItem.DefaultButton);

    if(aSrcItem.DataMessageId < 0)
      strcpy(aDestItem.Data, aSrcItem.DataString);
    else
      strcpy(aDestItem.Data, GetMsgString(aSrcItem.DataMessageId));

    /*
    if ((unsigned int)Init[i].Data < 0xFFF)
      strcpy(aDestItem.Data, GetMsg((unsigned int)aSrcItem.Data));
    else
      strcpy(aDestItem.Data,aSrcItem.Data);
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

void CStartupInfo::RestoreScreen(HANDLE aHandle)
{
  m_Data.RestoreScreen(aHandle);
}

const char kRegestryKeyDelimiter = '\'';

CSysString CStartupInfo::GetFullKeyName(const CSysString &aKeyName) const
{
  return (aKeyName.IsEmpty()) ? m_RegistryPath:
    (m_RegistryPath + kRegestryKeyDelimiter + aKeyName);
}


LONG CStartupInfo::CreateRegKey(HKEY aKeyParent, 
    const CSysString &aKeyName, NRegistry::CKey &aDestKey) const
{
  return aDestKey.Create(aKeyParent, GetFullKeyName(aKeyName));
}

LONG CStartupInfo::OpenRegKey(HKEY aKeyParent, 
    const CSysString &aKeyName, NRegistry::CKey &aDestKey) const 
{
  return aDestKey.Open(aKeyParent, GetFullKeyName(aKeyName));
}

void CStartupInfo::SetRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName, 
    LPCTSTR aValueName, LPCTSTR aValue) const
{
  NRegistry::CKey aRegKey;
  CreateRegKey(aKeyParent, aKeyName, aRegKey);
  aRegKey.SetValue(aValueName, aValue);
}

void CStartupInfo::SetRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName, 
    LPCTSTR aValueName, UINT32 aValue) const
{
  NRegistry::CKey aRegKey;
  CreateRegKey(aKeyParent, aKeyName, aRegKey);
  aRegKey.SetValue(aValueName, aValue);
}

void CStartupInfo::SetRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName, 
    LPCTSTR aValueName, bool aValue) const
{
  NRegistry::CKey aRegKey;
  CreateRegKey(aKeyParent, aKeyName, aRegKey);
  aRegKey.SetValue(aValueName, aValue);
}

CSysString CStartupInfo::QueryRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName,
    LPCTSTR aValueName, const CSysString &aValueDefault) const
{
  NRegistry::CKey aRegKey;
  if (OpenRegKey(aKeyParent, aKeyName, aRegKey) != ERROR_SUCCESS)
    return aValueDefault;
  
  CSysString aValue;
  if(aRegKey.QueryValue(aValueName, aValue) != ERROR_SUCCESS)
    return aValueDefault;
  
  return aValue;
}

UINT32 CStartupInfo::QueryRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName,
    LPCTSTR aValueName, UINT32 aValueDefault) const
{
  NRegistry::CKey aRegKey;
  if (OpenRegKey(aKeyParent, aKeyName, aRegKey) != ERROR_SUCCESS)
    return aValueDefault;
  
  UINT32 aValue;
  if(aRegKey.QueryValue(aValueName, aValue) != ERROR_SUCCESS)
    return aValueDefault;
  
  return aValue;
}

bool CStartupInfo::QueryRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName,
    LPCTSTR aValueName, bool aValueDefault) const
{
  NRegistry::CKey aRegKey;
  if (OpenRegKey(aKeyParent, aKeyName, aRegKey) != ERROR_SUCCESS)
    return aValueDefault;
  
  bool aValue;
  if(aRegKey.QueryValue(aValueName, aValue) != ERROR_SUCCESS)
    return aValueDefault;
  
  return aValue;
}

bool CStartupInfo::Control(HANDLE aPlugin, int aCommand, void *aParam)
{
  return BOOLToBool(m_Data.Control(aPlugin, aCommand, aParam));
}

bool CStartupInfo::ControlRequestActivePanel(int aCommand, void *aParam)
{
  return Control(INVALID_HANDLE_VALUE, aCommand, aParam);
}

bool CStartupInfo::ControlGetActivePanelInfo(PanelInfo &aPanelInfo)
{
  return ControlRequestActivePanel(FCTL_GETPANELINFO, &aPanelInfo);
}

bool CStartupInfo::ControlSetSelection(const PanelInfo &aPanelInfo)
{
  return ControlRequestActivePanel(FCTL_SETSELECTION, (void *)&aPanelInfo);
}

bool CStartupInfo::ControlGetActivePanelCurrentItemInfo(
    PluginPanelItem &aPluginPanelItem)
{
  PanelInfo aPanelInfo;
  if(!ControlGetActivePanelInfo(aPanelInfo))
    return false;
  if(aPanelInfo.ItemsNumber <= 0)
    throw "There are no items";
  aPluginPanelItem = aPanelInfo.PanelItems[aPanelInfo.CurrentItem];
  return true;
}

bool CStartupInfo::ControlGetActivePanelSelectedOrCurrentItems(
    CObjectVector<PluginPanelItem> &aPluginPanelItems)
{
  aPluginPanelItems.Clear();
  PanelInfo aPanelInfo;
  if(!ControlGetActivePanelInfo(aPanelInfo))
    return false;
  if(aPanelInfo.ItemsNumber <= 0)
    throw "There are no items";
  if (aPanelInfo.SelectedItemsNumber == 0)
    aPluginPanelItems.Add(aPanelInfo.PanelItems[aPanelInfo.CurrentItem]);
  else
    for (int i = 0; i < aPanelInfo.SelectedItemsNumber; i++)
      aPluginPanelItems.Add(aPanelInfo.SelectedItems[i]);
  return true;
}

bool CStartupInfo::ControlClearPanelSelection()
{
  PanelInfo aPanelInfo;
  if(!ControlGetActivePanelInfo(aPanelInfo))
    return false;
  for (int i = 0; i < aPanelInfo.ItemsNumber; i++)
    aPanelInfo.PanelItems[i].Flags &= ~PPIF_SELECTED;
  return ControlSetSelection(aPanelInfo);
}

////////////////////////////////////////////////
// menu function

int CStartupInfo::Menu(
    int x,
    int y,
    int aMaxHeight,
    unsigned int aFlags,
    const char *aTitle,
    const char *aBottom,
    const char *aHelpTopic,
    int *aBreakKeys,
    int *aBreakCode,
    struct FarMenuItem *anItems,
    int aNumItems)
{
  return m_Data.Menu(m_Data.ModuleNumber, x, y, aMaxHeight, aFlags, (char *)aTitle, 
      (char *)aBottom, (char *)aHelpTopic, aBreakKeys, aBreakCode, anItems, aNumItems);
}

int CStartupInfo::Menu(
    unsigned int aFlags,
    const char *aTitle,
    const char *aHelpTopic,
    struct FarMenuItem *anItems,
    int aNumItems)
{
  return Menu(-1, -1, 0, aFlags, aTitle, NULL, aHelpTopic, NULL,
      NULL, anItems, aNumItems);
}

int CStartupInfo::Menu(
    unsigned int aFlags,
    const char *aTitle,
    const char *aHelpTopic,
    const CSysStringVector &anItems, 
    int aSelectedItem)
{
  vector<FarMenuItem> aFarMenuItems;
  for(int i = 0; i < anItems.Size(); i++)
  {
    FarMenuItem anItem;
    anItem.Checked = 0;
    anItem.Separator = 0;
    anItem.Selected = (i == aSelectedItem);
    CSysString aReducedString = 
        anItems[i].Left(sizeof(anItem.Text) / sizeof(anItem.Text[0]) - 1);
    strcpy(anItem.Text, aReducedString);
    aFarMenuItems.push_back(anItem);
  }
  return Menu(aFlags, aTitle, aHelpTopic, &aFarMenuItems.front(), 
      aFarMenuItems.size());
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

static CSysString DWORDToString(DWORD aNumber)
{
  char aBuffer[32];
  ultoa(aNumber, aBuffer, 10);
  return aBuffer;
}

void PrintErrorMessage(const char *aMessage, int anCode)
{
  CSysString aTmp = aMessage;
  aTmp += " #";
  aTmp += DWORDToString(anCode);
  g_StartupInfo.ShowMessage(aTmp);
}

void PrintErrorMessage(const char *aMessage, const char *aText)
{
  CSysString aTmp = aMessage;
  aTmp += ": ";
  aTmp += aText;
  g_StartupInfo.ShowMessage(aTmp);
}

bool WasEscPressed()
{
  NConsole::CIn anInConsole;
  HANDLE aHandle = ::GetStdHandle(STD_INPUT_HANDLE);
  if(aHandle == INVALID_HANDLE_VALUE)
    return true;
  anInConsole.Attach(aHandle);
  while (true)
  {
    DWORD aNumberOfEvents;
    if(!anInConsole.GetNumberOfEvents(aNumberOfEvents))
      return true;
    if(aNumberOfEvents == 0)
      return false;

    INPUT_RECORD anEvent;
    if(!anInConsole.ReadEvent(anEvent, aNumberOfEvents))
      return true;
    if (anEvent.EventType == KEY_EVENT &&
        anEvent.Event.KeyEvent.bKeyDown &&
        anEvent.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
      return true;
  }
}

void ShowErrorMessage(DWORD aError)
{
  CSysString aMessage;
  NError::MyFormatMessage(aError, aMessage);
  g_StartupInfo.ShowMessage(SystemStringToOemString(aMessage));

}

void ShowLastErrorMessage()
{
  ShowErrorMessage(::GetLastError());
}

}
