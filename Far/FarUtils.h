// FarUtils.h

#pragma once 

#ifndef __FARUTILS_H
#define __FARUTILS_H

#include "FarPlugin.h"
#include "Common/String.h"
#include "Common/Vector.h"
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

  CSysString GetFullKeyName(const CSysString &aKeyName) const;
  LONG CreateRegKey(HKEY aKeyParent, 
    const CSysString &aKeyName, NWindows::NRegistry::CKey &aDestKey) const;
  LONG OpenRegKey(HKEY aKeyParent, 
    const CSysString &aKeyName, NWindows::NRegistry::CKey &aDestKey) const;

public:
  void Init(const PluginStartupInfo &aPluginStartupInfo, 
      const CSysString &aPliginNameForRegestry);
  const char *GetMsgString(int aMessageId);
  int ShowMessage(unsigned int aFlags, const char *aHelpTopic, 
      const char **anItems, int anItemsNumber, int aButtonsNumber);
  int ShowMessage(const char *aMessage);
  int ShowMessage(int aMessageId);

  int ShowDialog(int X1, int Y1, int X2, int Y2, 
      const char *aHelpTopic, struct FarDialogItem *anItems, int aNumItems);
  int ShowDialog(int aSizeX, int aSizeY,
      const char *aHelpTopic, struct FarDialogItem *anItems, int aNumItems);

  void InitDialogItems(const CInitDialogItem  *aSrcItems, 
      FarDialogItem *aDestItems, int aNum);
  
  HANDLE SaveScreen(int X1, int Y1, int X2, int Y2);
  HANDLE SaveScreen();
  void RestoreScreen(HANDLE aHandle);

  void SetRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName, 
      const LPCTSTR aValueName, LPCTSTR aValue) const;
  void SetRegKeyValue(HKEY hRoot, const CSysString &aKeyName, 
      const LPCTSTR aValueName, UINT32 aValue) const;
  void SetRegKeyValue(HKEY hRoot, const CSysString &aKeyName, 
      const LPCTSTR aValueName, bool aValue) const;

  CSysString QueryRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName,
      LPCTSTR aValueName, const CSysString &aValueDefault) const;

  UINT32 QueryRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName,
      LPCTSTR aValueName, UINT32 aValueDefault) const;

  bool QueryRegKeyValue(HKEY aKeyParent, const CSysString &aKeyName,
      LPCTSTR aValueName, bool aValueDefault) const;

  bool Control(HANDLE aPlugin, int aCommand, void *aParam);
  bool ControlRequestActivePanel(int aCommand, void *aParam);
  bool ControlGetActivePanelInfo(PanelInfo &aPanelInfo);
  bool ControlSetSelection(const PanelInfo &aPanelInfo);
  bool ControlGetActivePanelCurrentItemInfo(PluginPanelItem &aPluginPanelItem);
  bool ControlGetActivePanelSelectedOrCurrentItems(
      CObjectVector<PluginPanelItem> &aPluginPanelItems);

  bool ControlClearPanelSelection();

  int Menu(
      int x,
      int y,
      int aMaxHeight,
      unsigned int aFlags,
      const char *aTitle,
      const char *aBottom,
      const char *aHelpTopic,
      int *aBreakKeys,
      int *aBreakCode,
      FarMenuItem *anItems,
      int aNumItems);
  int Menu(
      unsigned int aFlags,
      const char *aTitle,
      const char *aHelpTopic,
      FarMenuItem *anItems,
      int aNumItems);

  int Menu(
      unsigned int aFlags,
      const char *aTitle,
      const char *aHelpTopic,
      const CSysStringVector &anItems, 
      int aSelectedItem);

  int Editor(const char *aFileName, const char *aTitle, 
      int X1, int Y1, int X2, int Y2, DWORD aFlags, int aStartLine, int aStartChar)
      { return m_Data.Editor((char *)aFileName, (char *)aTitle, X1, Y1, X2, Y2, 
        aFlags, aStartLine, aStartChar); }
  int Editor(const char *aFileName)
      { return Editor(aFileName, NULL, 0, 0, -1, -1, 0, -1, -1); }

  int Viewer(const char *aFileName, const char *aTitle, 
      int X1, int Y1, int X2, int Y2, DWORD aFlags)
      { return m_Data.Viewer((char *)aFileName, (char *)aTitle, X1, Y1, X2, Y2, aFlags); }
  int Viewer(const char *aFileName)
      { return Viewer(aFileName, NULL, 0, 0, -1, -1, VF_NONMODAL); }

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

void PrintErrorMessage(const char *aMessage, int anCode);
void PrintErrorMessage(const char *aMessage, const char *aText);

#define  MY_TRY_BEGIN   try\
  {

#define  MY_TRY_END1(x)     }\
  catch(int n)\
  {\
    PrintErrorMessage(x, n);\
    return;\
  }\
  catch(const CSysString &aText)\
  {\
    PrintErrorMessage(x, aText);\
    return;\
  }\
  catch(const char *aText)\
  {\
    PrintErrorMessage(x, aText);\
    return;\
  }\
  catch(...)\
  {\
    g_StartupInfo.ShowMessage(x);\
    return;\
  }

#define  MY_TRY_END2(x, y)     }\
  catch(int n)\
  {\
    PrintErrorMessage(x, n);\
    return y;\
  }\
  catch(const CSysString &aText)\
  {\
    PrintErrorMessage(x, aText);\
    return y;\
  }\
  catch(const char *aText)\
  {\
    PrintErrorMessage(x, aText);\
    return y;\
  }\
  catch(...)\
  {\
    g_StartupInfo.ShowMessage(x);\
    return y;\
  }

bool WasEscPressed();

void ShowErrorMessage(DWORD aError);
void ShowLastErrorMessage();

}

#endif
