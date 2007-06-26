// FarUtils.h

#ifndef __FARUTILS_H
#define __FARUTILS_H

#include "FarPlugin.h"
#include "Windows/Registry.h"

namespace NFar {

namespace NFileOperationReturnCode
{
  enum EEnum
  {
    kInterruptedByUser = -1,
    kError = 0,
    kSuccess = 1
  };
}

namespace NEditorReturnCode
{
  enum EEnum
  {
    kOpenError = 0,
    kFileWasChanged = 1,
    kFileWasNotChanged = 2,
    kInterruptedByUser = 3
  };
}

struct CInitDialogItem
{
  DialogItemTypes Type;
  int X1,Y1,X2,Y2;
  bool Focus;
  bool Selected;
  unsigned int Flags; //FarDialogItemFlags Flags;
  bool DefaultButton;
  int DataMessageId;
  const char *DataString;
  const char *HistoryName;
  // void InitToFarDialogItem(struct FarDialogItem &anItemDest);
};

class CStartupInfo
{
  PluginStartupInfo m_Data;
  CSysString m_RegistryPath;

  CSysString GetFullKeyName(const CSysString &keyName) const;
  LONG CreateRegKey(HKEY parentKey, 
    const CSysString &keyName, NWindows::NRegistry::CKey &destKey) const;
  LONG OpenRegKey(HKEY parentKey, 
    const CSysString &keyName, NWindows::NRegistry::CKey &destKey) const;

public:
  void Init(const PluginStartupInfo &pluginStartupInfo, 
      const CSysString &pliginNameForRegestry);
  const char *GetMsgString(int messageId);
  int ShowMessage(unsigned int flags, const char *helpTopic, 
      const char **items, int numItems, int numButtons);
  int ShowMessage(const char *message);
  int ShowMessage(int messageId);

  int ShowDialog(int X1, int Y1, int X2, int Y2, 
      const char *helpTopic, struct FarDialogItem *items, int numItems);
  int ShowDialog(int sizeX, int sizeY,
      const char *helpTopic, struct FarDialogItem *items, int numItems);

  void InitDialogItems(const CInitDialogItem *srcItems, 
      FarDialogItem *destItems, int numItems);
  
  HANDLE SaveScreen(int X1, int Y1, int X2, int Y2);
  HANDLE SaveScreen();
  void RestoreScreen(HANDLE handle);

  void SetRegKeyValue(HKEY parentKey, const CSysString &keyName, 
      const LPCTSTR valueName, LPCTSTR value) const;
  void SetRegKeyValue(HKEY hRoot, const CSysString &keyName, 
      const LPCTSTR valueName, UINT32 value) const;
  void SetRegKeyValue(HKEY hRoot, const CSysString &keyName, 
      const LPCTSTR valueName, bool value) const;

  CSysString QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
      LPCTSTR valueName, const CSysString &valueDefault) const;

  UINT32 QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
      LPCTSTR valueName, UINT32 valueDefault) const;

  bool QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
      LPCTSTR valueName, bool valueDefault) const;

  bool Control(HANDLE plugin, int command, void *param);
  bool ControlRequestActivePanel(int command, void *param);
  bool ControlGetActivePanelInfo(PanelInfo &panelInfo);
  bool ControlSetSelection(const PanelInfo &panelInfo);
  bool ControlGetActivePanelCurrentItemInfo(PluginPanelItem &pluginPanelItem);
  bool ControlGetActivePanelSelectedOrCurrentItems(
      CObjectVector<PluginPanelItem> &pluginPanelItems);

  bool ControlClearPanelSelection();

  int Menu(
      int x,
      int y,
      int maxHeight,
      unsigned int flags,
      const char *title,
      const char *aBottom,
      const char *helpTopic,
      int *breakKeys,
      int *breakCode,
      FarMenuItem *items,
      int numItems);
  int Menu(
      unsigned int flags,
      const char *title,
      const char *helpTopic,
      FarMenuItem *items,
      int numItems);

  int Menu(
      unsigned int flags,
      const char *title,
      const char *helpTopic,
      const CSysStringVector &items, 
      int selectedItem);

  int Editor(const char *fileName, const char *title, 
      int X1, int Y1, int X2, int Y2, DWORD flags, int startLine, int startChar)
      { return m_Data.Editor((char *)fileName, (char *)title, X1, Y1, X2, Y2, 
        flags, startLine, startChar); }
  int Editor(const char *fileName)
      { return Editor(fileName, NULL, 0, 0, -1, -1, 0, -1, -1); }

  int Viewer(const char *fileName, const char *title, 
      int X1, int Y1, int X2, int Y2, DWORD flags)
      { return m_Data.Viewer((char *)fileName, (char *)title, X1, Y1, X2, Y2, flags); }
  int Viewer(const char *fileName)
      { return Viewer(fileName, NULL, 0, 0, -1, -1, VF_NONMODAL); }

};

class CScreenRestorer
{
  bool m_Saved;
  HANDLE m_HANDLE;
public:
  CScreenRestorer(): m_Saved(false){};
  ~CScreenRestorer();
  void Save();
  void Restore();
};


extern CStartupInfo g_StartupInfo;

void PrintErrorMessage(const char *message, int code);
void PrintErrorMessage(const char *message, const char *aText);

#define  MY_TRY_BEGIN   try\
  {

#define  MY_TRY_END1(x)     }\
  catch(int n) { PrintErrorMessage(x, n);  return; }\
  catch(const CSysString &s) { PrintErrorMessage(x, s); return; }\
  catch(const char *s) { PrintErrorMessage(x, s); return; }\
  catch(...) { g_StartupInfo.ShowMessage(x);  return; }

#define  MY_TRY_END2(x, y)     }\
  catch(int n) { PrintErrorMessage(x, n); return y; }\
  catch(const CSysString &s) { PrintErrorMessage(x, s); return y; }\
  catch(const char *s) { PrintErrorMessage(x, s); return y; }\
  catch(...) { g_StartupInfo.ShowMessage(x); return y; }

bool WasEscPressed();
void ShowErrorMessage(DWORD errorCode);
void ShowLastErrorMessage();

}

#endif
