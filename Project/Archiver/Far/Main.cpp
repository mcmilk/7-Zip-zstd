// Test Align for updating !!!!!!!!!!!!!!!!!!

#include "StdAfx.h"

#include <locale.h>
#include <initguid.h>

#include "Plugin.h"

#include "Common/WildCard.h"
#include "Common/DynamicBuffer.h"
#include "Common/StringConvert.h"
#include "Common/Defs.h"

#include "Windows/FileFind.h"
#include "Windows/FileIO.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"

#include "Far/FarUtils.h"
#include "Far/ProgressBox.h"

#include "Messages.h"

#include "Interface/FileStreams.h"

#include "../Common/DefaultName.h"
#include "../Common/OpenEngine2.h"
#include "../../Compress/Interface/CompressInterface.h"
#include "Interface/CryptoInterface.h"


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
  void WINAPI SetStartupInfo(struct PluginStartupInfo *info);
  HANDLE WINAPI OpenFilePlugin(char *name, const unsigned char *Data, 
      unsigned int DataSize);
  HANDLE WINAPI OpenPlugin(int openFrom, int anItem);
  void WINAPI ClosePlugin(HANDLE plugin);
  int WINAPI GetFindData(HANDLE plugin, struct PluginPanelItem **panelItems, 
      int *itemsNumber, int OpMode);
  void WINAPI FreeFindData(HANDLE plugin, struct PluginPanelItem *panelItems,
    int itemsNumber);
  int WINAPI GetFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
    int itemsNumber, int move, char *destPath, int opMode);
  int WINAPI SetDirectory(HANDLE plugin, char *dir, int opMode);
  void WINAPI GetPluginInfo(struct PluginInfo *info);
  int WINAPI Configure(int itemNumber);
  void WINAPI GetOpenPluginInfo(HANDLE plugin, struct OpenPluginInfo *info);
  int WINAPI PutFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
      int itemsNumber, int move, int opMode);
  int WINAPI DeleteFiles(HANDLE plugin, PluginPanelItem *panelItems,
      int itemsNumber, int opMode);
  int WINAPI ProcessKey(HANDLE plugin, int key, unsigned int controlState);
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

void WINAPI SetStartupInfo(struct PluginStartupInfo *info)
{
  MY_TRY_BEGIN_COM_INIT;
  g_StartupInfo.Init(*info, kPliginNameForRegestry);
  g_Options.Enabled = g_StartupInfo.QueryRegKeyValue(
      HKEY_CURRENT_USER, kRegisrtryMainKeyName, 
      kRegisrtryValueNameEnabled, kPluginEnabledDefault);
  MY_TRY_END1("SetStartupInfo");
}

class COpenArchiveCallback: 
  public IArchiveOpenCallback,
  public IArchiveOpenVolumeCallback,
  public IProgress,
  public ICryptoGetTextPassword,
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

  NWindows::NFile::NFind::CFileInfo _fileInfo;
public:
  bool PasswordIsDefined;
  UString Password;

  CSysString _folderPrefix;

public:
BEGIN_COM_MAP(COpenArchiveCallback)
  COM_INTERFACE_ENTRY(IArchiveOpenCallback)
  COM_INTERFACE_ENTRY(IArchiveOpenVolumeCallback)
  COM_INTERFACE_ENTRY(IProgress)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COpenArchiveCallback)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 total);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IArchiveOpenCallback
  STDMETHOD(SetTotal)(const UINT64 *numFiles, const UINT64 *numBytes);
  STDMETHOD(SetCompleted)(const UINT64 *numFiles, const UINT64 *numBytes);

  // IArchiveOpenVolumeCallback
  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  void Init(CMessageBox *messageBox)
  {
    PasswordIsDefined = false;

    m_NumFilesMax = 0;
    m_MessageBoxIsShown = false;
    m_PrevTickCount = GetTickCount();
    m_MessageBox = messageBox;
  }
  void ShowMessage(const UINT64 *completed);

  void LoadFileInfo(const CSysString &folderPrefix,  
      const CSysString &fileName)
  {
    _folderPrefix = folderPrefix;
    if (!NWindows::NFile::NFind::FindFile(_folderPrefix + fileName, _fileInfo))
      throw 1;
  }
};

void COpenArchiveCallback::ShowMessage(const UINT64 *completed)
{
  UINT32 currentTime = GetTickCount();
  if (!m_MessageBoxIsShown)
  {
    if (currentTime - m_PrevTickCount < 400)
      return;
    m_MessageBox->Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle), 
      g_StartupInfo.GetMsgString(NMessageID::kReading), 2, 30);
    m_MessageBoxIsShown = true;
  }
  else
  {
    if (currentTime - m_PrevTickCount < 200)
      return;
  }
  m_PrevTickCount = currentTime;
  char aMessage[256];
  sprintf(aMessage, "%5I64u", m_NumFilesMax);
  char aMessage2[256];
  aMessage2[0] = '\0';
  if (completed != NULL)
    sprintf(aMessage2, "%5I64u", *completed);
  const char *aMessages[2] = 
     {aMessage, aMessage2 };
  m_MessageBox->ShowProcessMessages(aMessages);
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 *numFiles, const UINT64 *numBytes)
{
  if (WasEscPressed())
    return E_ABORT;
  m_NumFilesDefined = (numFiles != NULL);
  if (m_NumFilesDefined)
    m_NumFiles = *numFiles;
  m_NumBytesDefined = (numBytes != NULL);
  if (m_NumBytesDefined)
    m_NumBytes = *numBytes;
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 *numFiles, const UINT64 *numBytes)
{
  if (WasEscPressed())
    return E_ABORT;
  if (numFiles == NULL)
    return S_OK;
  m_NumFilesMax = *numFiles;
  // if (*numFiles % 100 != 0)
  //   return S_OK;
  ShowMessage(NULL);
  return S_OK;
}


STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 total)
{
  if (WasEscPressed())
    return E_ABORT;
  /*
  aNumFilesDefined = (numFiles != NULL);
  if (aNumFilesDefined)
    aNumFiles = *numFiles;
  aNumBytesDefined = (numBytes != NULL);
  if (aNumBytesDefined)
    aNumBytes = *numBytes;
  */
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 *completed)
{
  if (WasEscPressed())
    return E_ABORT;
  if (completed == NULL)
    return S_OK;
  // if (*completed % 100 != 0)
  //   return S_OK;
  ShowMessage(completed);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetStream(const wchar_t *name, 
    IInStream **inStream)
{
  *inStream = NULL;
  CSysString fullPath = _folderPrefix + GetSystemString(name, CP_OEMCP);
  if (!NWindows::NFile::NFind::FindFile(fullPath, _fileInfo))
    return S_FALSE;
  if (_fileInfo.IsDirectory())
    return S_FALSE;
  CComObjectNoLock<CInFileStream> *inFile = new CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStreamTemp = inFile;
  if (!inFile->Open(fullPath))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
}


STDMETHODIMP COpenArchiveCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
  switch(propID)
  {
  case kpidName:
    propVariant = GetUnicodeString(_fileInfo.Name, CP_OEMCP);
    break;
  case kpidIsFolder:
    propVariant = _fileInfo.IsDirectory();
    break;
  case kpidSize:
    propVariant = _fileInfo.Size;
    break;
  case kpidAttributes:
    propVariant = (UINT32)_fileInfo.Attributes;
    break;
  case kpidLastAccessTime:
    propVariant = _fileInfo.LastAccessTime;
    break;
  case kpidCreationTime:
    propVariant = _fileInfo.CreationTime;
    break;
  case kpidLastWriteTime:
    propVariant = _fileInfo.LastWriteTime;
    break;
  }
  propVariant.Detach(value);
  return S_OK;
}

HRESULT GetPassword(UString &password)
{
  password.Empty();
  CInitDialogItem anInitItems[]=
  {
    { DI_DOUBLEBOX, 3, 1, 72, 4, false, false, 0, false,  NMessageID::kGetPasswordTitle, NULL, NULL }, 
    { DI_TEXT, 5, 2, 0, 0, false, false, DIF_SHOWAMPERSAND, false, NMessageID::kEnterPasswordForFile, NULL, NULL },
    { DI_PSWEDIT, 5, 3, 70, 3, true, false, 0, true, -1, "", NULL }
  };
  
  const kNumItems = sizeof(anInitItems)/sizeof(anInitItems[0]);
  FarDialogItem aDialogItems[kNumItems];
  g_StartupInfo.InitDialogItems(anInitItems, aDialogItems, kNumItems);
  
  // sprintf(DialogItems[1].Data,GetMsg(MGetPasswordForFile),FileName);
  if (g_StartupInfo.ShowDialog(76, 6, NULL, aDialogItems, kNumItems) < 0)
    return (E_ABORT);

  AString oemPassword = aDialogItems[2].Data;
  password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    RETURN_IF_NOT_S_OK(GetPassword(Password));
    PasswordIsDefined = true;
  }
  CComBSTR temp = Password;
  *password = temp.Detach();
  return S_OK;
}

static HANDLE MyOpenFilePlugin(const char *name)
{
  CSysString aNormalizedName = name;
  aNormalizedName.Trim();
  CSysString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(aNormalizedName, fullName, fileNamePartStartIndex);
  NFile::NFind::CFileInfo aFileInfo;
  if (!NFile::NFind::FindFile(fullName, aFileInfo))
    return INVALID_HANDLE_VALUE;
  if (aFileInfo.IsDirectory())
     return INVALID_HANDLE_VALUE;


  CComPtr<IInFolderArchive> archiveHandler;

  NZipRootRegistry::CArchiverInfo archiverInfoResult;
  // ::OutputDebugString("before OpenArchive\n");
  
  CComObjectNoLock<COpenArchiveCallback> *openArchiveCallbackSpec =
    new CComObjectNoLock<COpenArchiveCallback>;
  
  CComPtr<IArchiveOpenCallback> openArchiveCallback = openArchiveCallbackSpec;

  // if ((opMode & OPM_SILENT) == 0 && (opMode & OPM_FIND ) == 0)
  CScreenRestorer screenRestorer;
  CMessageBox m_MessageBox;
  {
    screenRestorer.Save();
  }
  openArchiveCallbackSpec->Init(&m_MessageBox);
  openArchiveCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex), 
      fullName.Mid(fileNamePartStartIndex));
  
  // ::OutputDebugString("before OpenArchive\n");
  
  UString defaultName;
  HRESULT result = ::OpenArchive(fullName, &archiveHandler, 
      archiverInfoResult, defaultName, openArchiveCallback);
  if (result != S_OK)
     return INVALID_HANDLE_VALUE;

  // ::OutputDebugString("after OpenArchive\n");

  /*
  std::auto_ptr<CProxyHandler> aProxyHandler(new CProxyHandler());

  if(aProxyHandler->Init(archiveHandler, 
      aFileInfo,
      GetDefaultName(fullName, archiverInfoResult.Extension), 
      openArchiveCallbackSpec) != S_OK)
    return INVALID_HANDLE_VALUE;

  // ::OutputDebugString("after Init\n");
  */

  CPlugin *plugin = new CPlugin(fullName, defaultName, 
      archiveHandler, archiverInfoResult);
  if (plugin == NULL)
    return(INVALID_HANDLE_VALUE);
  plugin->PasswordIsDefined = openArchiveCallbackSpec->PasswordIsDefined;
  plugin->Password = openArchiveCallbackSpec->Password;

  return (HANDLE)(plugin);
}

HANDLE WINAPI OpenFilePlugin(char *name, 
    const unsigned char *data, unsigned int dataSize)
{
  MY_TRY_BEGIN_COM_INIT;
  if (name == NULL || (!g_Options.Enabled))
  {
    // if (!Opt.ProcessShiftF1)
      return(INVALID_HANDLE_VALUE);
  }
  return MyOpenFilePlugin(name);
  MY_TRY_END2("OpenFilePlugin", INVALID_HANDLE_VALUE);
}

HANDLE WINAPI OpenPlugin(int openFrom, int anItem)
{
  MY_TRY_BEGIN_COM_INIT;
  if(openFrom == OPEN_COMMANDLINE)
  {
    CSysString aFileName = (const char *)anItem;
    if(aFileName.IsEmpty())
      return INVALID_HANDLE_VALUE;
    if (aFileName.Length() >= 2 && 
        aFileName[0] == '\"' && aFileName[aFileName.Length() - 1] == '\"')
      aFileName = aFileName.Mid(1, aFileName.Length() - 2);

    return MyOpenFilePlugin(aFileName);
  }
  if(openFrom == OPEN_PLUGINSMENU)
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

void WINAPI ClosePlugin(HANDLE plugin)
{
  MY_TRY_BEGIN_COM_INIT;
  delete (CPlugin *)plugin;
  MY_TRY_END1("ClosePlugin");
}

int WINAPI GetFindData(HANDLE plugin, struct PluginPanelItem **panelItems,
    int *itemsNumber,int opMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)plugin)->GetFindData(panelItems, itemsNumber, opMode));
  MY_TRY_END2("GetFindData", FALSE);
}

void WINAPI FreeFindData(HANDLE plugin, struct PluginPanelItem *panelItems,
    int itemsNumber)
{
  MY_TRY_BEGIN_COM_INIT;
  ((CPlugin *)plugin)->FreeFindData(panelItems, itemsNumber);
  MY_TRY_END1("FreeFindData");      
}

int WINAPI GetFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
    int itemsNumber, int move, char *destPath, int opMode)
{
  MY_TRY_BEGIN_COM_INIT;  
  return(((CPlugin *)plugin)->GetFiles(panelItems, itemsNumber, move, destPath, opMode));
  MY_TRY_END2("GetFiles", NFileOperationReturnCode::kError);
}

int WINAPI SetDirectory(HANDLE plugin, char *dir, int opMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)plugin)->SetDirectory(dir, opMode));
  MY_TRY_END2("SetDirectory", FALSE);
}

void WINAPI GetPluginInfo(struct PluginInfo *info)
{
  MY_TRY_BEGIN_COM_INIT;

  info->StructSize = sizeof(*info);
  info->Flags = 0;
  info->DiskMenuStrings = NULL;
  info->DiskMenuNumbers = NULL;
  info->DiskMenuStringsNumber = 0;
  static const char *aPluginMenuStrings[2];
  aPluginMenuStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  aPluginMenuStrings[1] = g_StartupInfo.GetMsgString(NMessageID::kCreateArchiveMenuString);
  info->PluginMenuStrings = (char **)aPluginMenuStrings;
  info->PluginMenuStringsNumber = 2;
  static const char *PluginCfgStrings[1];
  PluginCfgStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  info->PluginConfigStrings = (char **)PluginCfgStrings;
  info->PluginConfigStringsNumber = sizeof(PluginCfgStrings) / sizeof(PluginCfgStrings[0]);
  info->CommandPrefix = (char *)kCommandPrefix;
  MY_TRY_END1("GetPluginInfo");      
}

int WINAPI Configure(int itemNumber)
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

void WINAPI GetOpenPluginInfo(HANDLE plugin,struct OpenPluginInfo *info)
{
  MY_TRY_BEGIN_COM_INIT;
  ((CPlugin *)plugin)->GetOpenPluginInfo(info);
  MY_TRY_END1("GetOpenPluginInfo");      
}

int WINAPI PutFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
                   int itemsNumber, int move, int opMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)plugin)->PutFiles(panelItems, itemsNumber, move, opMode));
  MY_TRY_END2("PutFiles", NFileOperationReturnCode::kError);
}

int WINAPI DeleteFiles(HANDLE plugin, PluginPanelItem *panelItems,
    int itemsNumber, int opMode)
{
  MY_TRY_BEGIN_COM_INIT;
  return(((CPlugin *)plugin)->DeleteFiles(panelItems, itemsNumber, opMode));
  MY_TRY_END2("DeleteFiles", FALSE);      
}

int WINAPI ProcessKey(HANDLE plugin, int key, unsigned int controlState)
{
  MY_TRY_BEGIN_COM_INIT;
  return (((CPlugin *)plugin)->ProcessKey(key, controlState));
  MY_TRY_END2("ProcessKey", FALSE);
}
