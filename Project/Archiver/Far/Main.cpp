// Test Align for updating !!!!!!!!!!!!!!!!!!

#include "StdAfx.h"

#include <locale.h>
#include <initguid.h>

#include "Plugin.h"

#include "Common/WildCard.h"
#include "Common/DynamicBuffer.h"

#include "Windows/FileFind.h"
#include "Windows/FileIO.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"

#include "Far/FarUtils.h"
#include "Far/ProgressBox.h"

#include "Messages.h"

#include "Common/Defs.h"

#include "../Common/DefaultName.h"
#include "../Common/OpenEngine2.h"
#include "../../Compress/Interface/CompressInterface.h"


using namespace NWindows;
using namespace NFar;

static const char *kCommandPrefix = "7-zip";

static const kDescriptionMaxSize = 256;

static const char *kRegisrtryMainKeyName = "";

static const char *kRegisrtryValueNameEnabled = "UsedByDefault3";
static bool kPluginEnabledDefault = true;

static const char *kHelpTopicConfig =  "Config";

extern "C"
{
  void WINAPI SetStartupInfo(struct PluginStartupInfo *anInfo);
  HANDLE WINAPI OpenFilePlugin(char *Name, const unsigned char *Data, 
      unsigned int DataSize);
  HANDLE WINAPI OpenPlugin(int anOpenFrom, int anItem);
  void WINAPI ClosePlugin(HANDLE aPlugin);
  int WINAPI GetFindData(HANDLE aPlugin, struct PluginPanelItem **aPanelItems, 
      int *anItemsNumber, int OpMode);
  void WINAPI FreeFindData(HANDLE aPlugin, struct PluginPanelItem *aPanelItems,
    int anItemsNumber);
  int WINAPI GetFiles(HANDLE aPlugin, struct PluginPanelItem *aPanelItems,
    int anItemsNumber, int aMove, char *aDestPath, int anOpMode);
  int WINAPI SetDirectory(HANDLE aPlugin, char *aDir, int anOpMode);
  void WINAPI GetPluginInfo(struct PluginInfo *anInfo);
  int WINAPI Configure(int anItemNumber);
  void WINAPI GetOpenPluginInfo(HANDLE aPlugin, struct OpenPluginInfo *anInfo);
  int WINAPI PutFiles(HANDLE aPlugin, struct PluginPanelItem *aPanelItems,
      int anItemsNumber, int aMove, int anOpMode);
  int WINAPI DeleteFiles(HANDLE aPlugin, PluginPanelItem *aPanelItems,
      int anItemsNumber, int anOpMode);
  int WINAPI ProcessKey(HANDLE aPlugin, int aKey, unsigned int aControlState);
};

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    setlocale(LC_COLLATE, ".OCP");
  }
  return TRUE;
}

static struct COptions
{
  bool Enabled;
} g_Options;

static const char *kPliginNameForRegestry = "7-ZIP";

#define  MY_TRY_BEGIN_COM_INIT  MY_TRY_BEGIN NCOM::CComInitializer aComInitializer;

void WINAPI SetStartupInfo(struct PluginStartupInfo *anInfo)
{
  MY_TRY_BEGIN_COM_INIT;
  g_StartupInfo.Init(*anInfo, kPliginNameForRegestry);
  g_Options.Enabled = g_StartupInfo.QueryRegKeyValue(
      HKEY_CURRENT_USER, kRegisrtryMainKeyName, 
      kRegisrtryValueNameEnabled, kPluginEnabledDefault);
  MY_TRY_END1("SetStartupInfo");
}

class COpenArchive2CallBack: 
  public IOpenArchive2CallBack,
  public IProgress,
  public CComObjectRoot
{
  DWORD m_StartTickValue;
  bool m_MessageBoxIsShown;
  CMessageBox *m_MessageBox;
  UINT64 m_NumFiles;
  UINT64 m_NumFilesMax;
  UINT64 m_NumFilesPrev;
  bool m_NumFilesDefined;
  UINT64 m_NumBytes;
  bool m_NumBytesDefined;
  UINT32 m_PrevTickCount;
public:
BEGIN_COM_MAP(COpenArchive2CallBack)
  COM_INTERFACE_ENTRY(IOpenArchive2CallBack)
  COM_INTERFACE_ENTRY(IProgress)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COpenArchive2CallBack)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aTotal);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IOpenArchive2CallBack
  STDMETHOD(SetTotal)(const UINT64 *aFiles, const UINT64 *aBytes);
  STDMETHOD(SetCompleted)(const UINT64 *aFiles, const UINT64 *aBytes);

  void Init(CMessageBox *aMessageBox)
  {
    m_NumFilesMax = 0;
    m_MessageBoxIsShown = false;
    m_PrevTickCount = GetTickCount();
    m_MessageBox = aMessageBox;
  }
  void ShowMessage(const UINT64 *aCompleted);
};

void COpenArchive2CallBack::ShowMessage(const UINT64 *aCompleted)
{
  UINT32 aCurrentTime = GetTickCount();
  if (!m_MessageBoxIsShown)
  {
    if (aCurrentTime - m_PrevTickCount < 400)
      return;
    m_MessageBox->Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle), 
      g_StartupInfo.GetMsgString(NMessageID::kReading), 2, 30);
    m_MessageBoxIsShown = true;
  }
  else
  {
    if (aCurrentTime - m_PrevTickCount < 200)
      return;
  }
  m_PrevTickCount = aCurrentTime;
  char aMessage[256];
  sprintf(aMessage, "%5I64u", m_NumFilesMax);
  char aMessage2[256];
  aMessage2[0] = '\0';
  if (aCompleted != NULL)
    sprintf(aMessage2, "%5I64u", *aCompleted);
  const char *aMessages[2] = 
     {aMessage, aMessage2 };
  m_MessageBox->ShowProcessMessages(aMessages);
}

STDMETHODIMP COpenArchive2CallBack::SetTotal(const UINT64 *aFiles, const UINT64 *aBytes)
{
  if (WasEscPressed())
    return E_ABORT;
  m_NumFilesDefined = (aFiles != NULL);
  if (m_NumFilesDefined)
    m_NumFiles = *aFiles;
  m_NumBytesDefined = (aBytes != NULL);
  if (m_NumBytesDefined)
    m_NumBytes = *aBytes;
  return S_OK;
}

STDMETHODIMP COpenArchive2CallBack::SetCompleted(const UINT64 *aFiles, const UINT64 *aBytes)
{
  if (WasEscPressed())
    return E_ABORT;
  if (aFiles == NULL)
    return S_OK;
  m_NumFilesMax = *aFiles;
  // if (*aFiles % 100 != 0)
  //   return S_OK;
  ShowMessage(NULL);
  return S_OK;
}


STDMETHODIMP COpenArchive2CallBack::SetTotal(const UINT64 aTotal)
{
  if (WasEscPressed())
    return E_ABORT;
  /*
  aNumFilesDefined = (aFiles != NULL);
  if (aNumFilesDefined)
    aNumFiles = *aFiles;
  aNumBytesDefined = (aBytes != NULL);
  if (aNumBytesDefined)
    aNumBytes = *aBytes;
  */
  return S_OK;
}

STDMETHODIMP COpenArchive2CallBack::SetCompleted(const UINT64 *aCompleted)
{
  if (WasEscPressed())
    return E_ABORT;
  if (aCompleted == NULL)
    return S_OK;
  // if (*aCompleted % 100 != 0)
  //   return S_OK;
  ShowMessage(aCompleted);
  return S_OK;
}

static HANDLE MyOpenFilePlugin(const char *aName)
{
  CSysString aNormalizedName = aName;
  aNormalizedName.Trim();
  CSysString aFullName;
  NFile::NDirectory::MyGetFullPathName(aNormalizedName, aFullName);
  NFile::NFind::CFileInfo aFileInfo;
  if (!NFile::NFind::FindFile(aFullName, aFileInfo))
    return INVALID_HANDLE_VALUE;
  if (aFileInfo.IsDirectory())
     return INVALID_HANDLE_VALUE;

  CComPtr<IArchiveHandler100> anArchiveHandler;

  NZipRootRegistry::CArchiverInfo anArchiverInfoResult;
  // ::OutputDebugString("before OpenArchive\n");
  
  CComObjectNoLock<COpenArchive2CallBack> *anOpenArchive2CallBackSpec =
    new CComObjectNoLock<COpenArchive2CallBack>;
  
  CComPtr<IOpenArchive2CallBack> anOpenArchive2CallBack = anOpenArchive2CallBackSpec;

  // if ((anOpMode & OPM_SILENT) == 0 && (anOpMode & OPM_FIND ) == 0)
  CScreenRestorer aScreenRestorer;
  CMessageBox m_MessageBox;
  {
    aScreenRestorer.Save();
    anOpenArchive2CallBackSpec->Init(&m_MessageBox);
  }
  
  // ::OutputDebugString("before OpenArchive\n");
  
  UString aDefaultName;
  HRESULT aResult = OpenArchive(aFullName, &anArchiveHandler, 
      anArchiverInfoResult, aDefaultName, anOpenArchive2CallBack);
  if (aResult != S_OK)
     return INVALID_HANDLE_VALUE;

  // ::OutputDebugString("after OpenArchive\n");

  /*
  std::auto_ptr<CProxyHandler> aProxyHandler(new CProxyHandler());

  if(aProxyHandler->Init(anArchiveHandler, 
      aFileInfo,
      GetDefaultName(aFullName, anArchiverInfoResult.Extension), 
      anOpenArchive2CallBackSpec) != S_OK)
    return INVALID_HANDLE_VALUE;

  // ::OutputDebugString("after Init\n");
  */

  CPlugin *aPlugin = new CPlugin(aFullName, aDefaultName, 
      anArchiveHandler, anArchiverInfoResult);
  if (aPlugin == NULL)
    return(INVALID_HANDLE_VALUE);
  return (HANDLE)(aPlugin);
}

HANDLE WINAPI OpenFilePlugin(char *aName, 
    const unsigned char *aData, unsigned int aDataSize)
{
  MY_TRY_BEGIN_COM_INIT;
  if (aName == NULL || (!g_Options.Enabled))
  {
    // if (!Opt.ProcessShiftF1)
      return(INVALID_HANDLE_VALUE);
  }
  return MyOpenFilePlugin(aName);
  MY_TRY_END2("OpenFilePlugin", INVALID_HANDLE_VALUE);
}

HANDLE WINAPI OpenPlugin(int anOpenFrom, int anItem)
{
  MY_TRY_BEGIN_COM_INIT;
  if(anOpenFrom == OPEN_COMMANDLINE)
  {
    CSysString aFileName = (const char *)anItem;
    if(aFileName.IsEmpty())
      return INVALID_HANDLE_VALUE;
    if (aFileName.Length() >= 2 && 
        aFileName[0] == '\"' && aFileName[aFileName.Length() - 1] == '\"')
      aFileName = aFileName.Mid(1, aFileName.Length() - 2);

    return MyOpenFilePlugin(aFileName);
  }
  if(anOpenFrom == OPEN_PLUGINSMENU)
  {
    switch(anItem)
    {
      case 0:
      {
        PluginPanelItem aPluginPanelItem;
        if(!g_StartupInfo.ControlGetActivePanelCurrentItemInfo(aPluginPanelItem))
          throw 142134;
        return MyOpenFilePlugin(aPluginPanelItem.FindData.cFileName);
      }
      case 1:
      {
        CObjectVector<PluginPanelItem> aPluginPanelItems;
        if(!g_StartupInfo.ControlGetActivePanelSelectedOrCurrentItems(aPluginPanelItems))
          throw 142134;
        if (CompressFiles(aPluginPanelItems) == S_OK)
        {
          int t = g_StartupInfo.ControlClearPanelSelection();
          g_StartupInfo.ControlRequestActivePanel(FCTL_UPDATEPANEL, NULL);
          g_StartupInfo.ControlRequestActivePanel(FCTL_REDRAWPANEL, NULL);
          g_StartupInfo.ControlRequestActivePanel(FCTL_UPDATEANOTHERPANEL, NULL);
          g_StartupInfo.ControlRequestActivePanel(FCTL_REDRAWANOTHERPANEL, NULL);
        }
        return INVALID_HANDLE_VALUE;
      }
      default:
        throw 4282215;
    }
  }
  return INVALID_HANDLE_VALUE;
  MY_TRY_END2("OpenPlugin", INVALID_HANDLE_VALUE);
}

void WINAPI ClosePlugin(HANDLE aPlugin)
{
  MY_TRY_BEGIN_COM_INIT;
  delete (CPlugin *)aPlugin;
  MY_TRY_END1("ClosePlugin");
}

int WINAPI GetFindData(HANDLE aPlugin, struct PluginPanelItem **aPanelItems,
    int *anItemsNumber,int anOpMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)aPlugin)->GetFindData(aPanelItems, anItemsNumber, anOpMode));
  MY_TRY_END2("GetFindData", FALSE);
}

void WINAPI FreeFindData(HANDLE aPlugin, struct PluginPanelItem *aPanelItems,
    int anItemsNumber)
{
  MY_TRY_BEGIN_COM_INIT;
  ((CPlugin *)aPlugin)->FreeFindData(aPanelItems, anItemsNumber);
  MY_TRY_END1("FreeFindData");      
}

int WINAPI GetFiles(HANDLE aPlugin, struct PluginPanelItem *aPanelItems,
    int anItemsNumber, int aMove, char *aDestPath, int anOpMode)
{
  MY_TRY_BEGIN_COM_INIT;  
  return(((CPlugin *)aPlugin)->GetFiles(aPanelItems, anItemsNumber, aMove, aDestPath, anOpMode));
  MY_TRY_END2("GetFiles", NFileOperationReturnCode::kError);
}

int WINAPI SetDirectory(HANDLE aPlugin, char *aDir, int anOpMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)aPlugin)->SetDirectory(aDir, anOpMode));
  MY_TRY_END2("SetDirectory", FALSE);
}

void WINAPI GetPluginInfo(struct PluginInfo *anInfo)
{
  MY_TRY_BEGIN_COM_INIT;

  anInfo->StructSize = sizeof(*anInfo);
  anInfo->Flags = 0;
  anInfo->DiskMenuStrings = NULL;
  anInfo->DiskMenuNumbers = NULL;
  anInfo->DiskMenuStringsNumber = 0;
  static const char *aPluginMenuStrings[2];
  aPluginMenuStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  aPluginMenuStrings[1] = g_StartupInfo.GetMsgString(NMessageID::kCreateArchiveMenuString);
  anInfo->PluginMenuStrings = (char **)aPluginMenuStrings;
  anInfo->PluginMenuStringsNumber = 2;
  static const char *PluginCfgStrings[1];
  PluginCfgStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  anInfo->PluginConfigStrings = (char **)PluginCfgStrings;
  anInfo->PluginConfigStringsNumber = sizeof(PluginCfgStrings) / sizeof(PluginCfgStrings[0]);
  anInfo->CommandPrefix = (char *)kCommandPrefix;
  MY_TRY_END1("GetPluginInfo");      
}

int WINAPI Configure(int anItemNumber)
{
  MY_TRY_BEGIN_COM_INIT;

  const kEnabledCheckBoxIndex = 1;

  const kYSize = 7;

  struct CInitDialogItem anInitItems[]=
  {
    { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kConfigTitle, NULL, NULL },
    { DI_CHECKBOX, 5, 2, 0, 0, true, g_Options.Enabled, 0, false, NMessageID::kConfigPluginEnabled, NULL, NULL },
    { DI_TEXT, 5, 3, 0, 0, false, false, DIF_BOXCOLOR | DIF_SEPARATOR, false, -1, "", NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kOk, NULL, NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL },
  };

  const kNumDialogItems = sizeof(anInitItems) / sizeof(anInitItems[0]);
  const kOkButtonIndex = kNumDialogItems - 2;

  FarDialogItem aDialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(anInitItems, aDialogItems, kNumDialogItems);

  int anAskCode = g_StartupInfo.ShowDialog(76, kYSize, 
      kHelpTopicConfig, aDialogItems, kNumDialogItems);

  if (anAskCode != kOkButtonIndex)
    return (FALSE);

  g_Options.Enabled = BOOLToBool(aDialogItems[kEnabledCheckBoxIndex].Selected);

  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegisrtryMainKeyName,
      kRegisrtryValueNameEnabled, g_Options.Enabled);
  return(TRUE);
  MY_TRY_END2("Configure", FALSE);
}

void WINAPI GetOpenPluginInfo(HANDLE aPlugin,struct OpenPluginInfo *anInfo)
{
  MY_TRY_BEGIN_COM_INIT;
  ((CPlugin *)aPlugin)->GetOpenPluginInfo(anInfo);
  MY_TRY_END1("GetOpenPluginInfo");      
}

int WINAPI PutFiles(HANDLE aPlugin, struct PluginPanelItem *aPanelItems,
                   int anItemsNumber, int aMove, int anOpMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)aPlugin)->PutFiles(aPanelItems, anItemsNumber, aMove, anOpMode));
  MY_TRY_END2("PutFiles", NFileOperationReturnCode::kError);
}

int WINAPI DeleteFiles(HANDLE aPlugin, PluginPanelItem *aPanelItems,
    int anItemsNumber, int anOpMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)aPlugin)->DeleteFiles(aPanelItems, anItemsNumber, anOpMode));
  MY_TRY_END2("DeleteFiles", FALSE);      
}

int WINAPI ProcessKey(HANDLE aPlugin, int aKey, unsigned int aControlState)
{
  MY_TRY_BEGIN_COM_INIT;
  return (((CPlugin *)aPlugin)->ProcessKey(aKey, aControlState));
  MY_TRY_END2("ProcessKey", FALSE);
}
