// Test Align for updating !!!!!!!!!!!!!!!!!!

#include "StdAfx.h"

#include "Common/MyInitGuid.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/NtCheck.h"

#include "../../Common/FileStreams.h"

#include "../Agent/Agent.h"

#include "Messages.h"
#include "Plugin.h"
#include "ProgressBox.h"

using namespace NWindows;
using namespace NFar;

static const char *kCommandPrefix = "7-zip";
static const TCHAR *kRegisrtryMainKeyName = TEXT("");
static const TCHAR *kRegisrtryValueNameEnabled = TEXT("UsedByDefault3");
static const char *kHelpTopicConfig =  "Config";
static bool kPluginEnabledDefault = true;

HINSTANCE g_hInstance;

#define NT_CHECK_FAIL_ACTION return FALSE;

BOOL WINAPI DllMain(
  #ifdef UNDER_CE
  HANDLE
  #else
  HINSTANCE
  #endif
  hInstance, DWORD dwReason, LPVOID)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    g_hInstance = (HINSTANCE)hInstance;
    NT_CHECK
  }
  return TRUE;
}

static struct COptions
{
  bool Enabled;
} g_Options;

static const TCHAR *kPliginNameForRegestry = TEXT("7-ZIP");

EXTERN_C void WINAPI SetStartupInfo(const PluginStartupInfo *info)
{
  MY_TRY_BEGIN;
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
  public CMyUnknownImp
{
  DWORD m_StartTickValue;
  bool m_MessageBoxIsShown;

  CProgressBox _progressBox;

  UInt64 _numFilesTotal;
  UInt64 _numFilesCur;
  UInt64 _numBytesTotal;
  UInt64 _numBytesCur;

  bool _numFilesTotalDefined;
  bool _numFilesCurDefined;
  bool _numBytesTotalDefined;
  bool _numBytesCurDefined;

  DWORD m_PrevTickCount;

  NWindows::NFile::NFind::CFileInfoW _fileInfo;
public:
  bool PasswordIsDefined;
  UString Password;

  UString _folderPrefix;

public:
  MY_UNKNOWN_IMP3(
     IArchiveOpenVolumeCallback,
     IProgress,
     ICryptoGetTextPassword
    )

  // IProgress
  STDMETHOD(SetTotal)(UInt64 total);
  STDMETHOD(SetCompleted)(const UInt64 *aCompleteValue);

  // IArchiveOpenCallback
  STDMETHOD(SetTotal)(const UInt64 *numFiles, const UInt64 *numBytes);
  STDMETHOD(SetCompleted)(const UInt64 *numFiles, const UInt64 *numBytes);

  // IArchiveOpenVolumeCallback
  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  void Init()
  {
    PasswordIsDefined = false;

    _numFilesTotalDefined = false;
    _numFilesCurDefined = false;
    _numBytesTotalDefined = false;
    _numBytesCurDefined = false;

    m_MessageBoxIsShown = false;
    m_PrevTickCount = GetTickCount();
  }
  void ShowMessage();

  void LoadFileInfo(const UString &folderPrefix, const UString &fileName)
  {
    _folderPrefix = folderPrefix;
    if (!_fileInfo.Find(_folderPrefix + fileName))
      throw 1;
  }
};

void COpenArchiveCallback::ShowMessage()
{
  DWORD currentTime = GetTickCount();
  if (!m_MessageBoxIsShown)
  {
    if (currentTime - m_PrevTickCount < 100)
      return;
    _progressBox.Init(
        // g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kReading), 48);

    m_MessageBoxIsShown = true;
  }
  else
  {
    if (currentTime - m_PrevTickCount < 200)
      return;
  }
  m_PrevTickCount = currentTime;

  UInt64 total = 0, cur = 0;
  bool curIsDefined = false, totalIsDefined = false;

  char message[256] = { 0 };
  if (_numFilesCurDefined)
    ConvertUInt64ToStringAligned(_numFilesCur, message, 5);

  if (_numFilesTotalDefined)
  {
    strcat(message, " / ");
    ConvertUInt64ToStringAligned(_numFilesTotal, message + strlen(message), 5);
    total = _numFilesTotal;
    totalIsDefined = true;
    if (_numFilesCurDefined)
    {
      cur = _numFilesCur;
      curIsDefined = true;
    }
  }
  else if (_numBytesTotalDefined)
  {
    total = _numBytesTotal;
    totalIsDefined = true;
    if (_numBytesCurDefined)
    {
      cur = _numBytesCur;
      curIsDefined = true;
    }
  }
  _progressBox.Progress(
      totalIsDefined ? &total: NULL,
      curIsDefined ? &cur: NULL,
      message);
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 *numFiles, const UInt64 *numBytes)
{
  if (WasEscPressed())
    return E_ABORT;
  
  _numFilesTotalDefined = (numFiles != NULL);
  if (_numFilesTotalDefined)
    _numFilesTotal = *numFiles;

  _numBytesTotalDefined = (numBytes != NULL);
  if (_numBytesTotalDefined)
    _numBytesTotal = *numBytes;

  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *numFiles, const UInt64 *numBytes)
{
  if (WasEscPressed())
    return E_ABORT;

  _numFilesCurDefined = (numFiles != NULL);
  if (_numFilesCurDefined)
    _numFilesCur = *numFiles;

  _numBytesCurDefined = (numBytes != NULL);
  if (_numBytesCurDefined)
    _numBytesCur = *numBytes;

  // if (*numFiles % 100 != 0)
  //   return S_OK;
  ShowMessage();
  return S_OK;
}


STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 /* total */)
{
  if (WasEscPressed())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *completed)
{
  if (WasEscPressed())
    return E_ABORT;
  if (completed == NULL)
    return S_OK;
  ShowMessage();
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetStream(const wchar_t *name, IInStream **inStream)
{
  if (WasEscPressed())
    return E_ABORT;
  *inStream = NULL;
  UString fullPath = _folderPrefix + name;
  if (!_fileInfo.Find(fullPath))
    return S_FALSE;
  if (_fileInfo.IsDir())
    return S_FALSE;
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<IInStream> inStreamTemp = inFile;
  if (!inFile->Open(fullPath))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
}


STDMETHODIMP COpenArchiveCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidName:  prop = GetUnicodeString(_fileInfo.Name, CP_OEMCP); break;
    case kpidIsDir:  prop = _fileInfo.IsDir(); break;
    case kpidSize:  prop = _fileInfo.Size; break;
    case kpidAttrib:  prop = (UInt32)_fileInfo.Attrib; break;
    case kpidCTime:  prop = _fileInfo.CTime; break;
    case kpidATime:  prop = _fileInfo.ATime; break;
    case kpidMTime:  prop = _fileInfo.MTime; break;
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT GetPassword(UString &password)
{
  if (WasEscPressed())
    return E_ABORT;
  password.Empty();
  CInitDialogItem initItems[]=
  {
    { DI_DOUBLEBOX, 3, 1, 72, 4, false, false, 0, false,  NMessageID::kGetPasswordTitle, NULL, NULL },
    { DI_TEXT, 5, 2, 0, 0, false, false, DIF_SHOWAMPERSAND, false, NMessageID::kEnterPasswordForFile, NULL, NULL },
    { DI_PSWEDIT, 5, 3, 70, 3, true, false, 0, true, -1, "", NULL }
  };
  
  const int kNumItems = sizeof(initItems)/sizeof(initItems[0]);
  FarDialogItem dialogItems[kNumItems];
  g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumItems);
  
  // sprintf(DialogItems[1].Data,GetMsg(MGetPasswordForFile),FileName);
  if (g_StartupInfo.ShowDialog(76, 6, NULL, dialogItems, kNumItems) < 0)
    return (E_ABORT);

  AString oemPassword = dialogItems[2].Data;
  password = MultiByteToUnicodeString(oemPassword, CP_OEMCP);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    RINOK(GetPassword(Password));
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}

/*
HRESULT OpenArchive(const CSysString &fileName,
    IInFolderArchive **archiveHandlerResult,
    CArchiverInfo &archiverInfoResult,
    UString &defaultName,
    IArchiveOpenCallback *openArchiveCallback)
{
  HRESULT OpenArchive(const CSysString &fileName,
    IInArchive **archive,
    CArchiverInfo &archiverInfoResult,
    IArchiveOpenCallback *openArchiveCallback);
}
*/

static HANDLE MyOpenFilePluginW(const wchar_t *name)
{
  UString normalizedName = name;
  normalizedName.Trim();
  UString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(normalizedName, fullName, fileNamePartStartIndex);
  NFile::NFind::CFileInfoW fileInfo;
  if (!fileInfo.Find(fullName))
    return INVALID_HANDLE_VALUE;
  if (fileInfo.IsDir())
     return INVALID_HANDLE_VALUE;


  CMyComPtr<IInFolderArchive> archiveHandler;

  // CArchiverInfo archiverInfoResult;
  // ::OutputDebugStringA("before OpenArchive\n");
  
  CScreenRestorer screenRestorer;
  {
    screenRestorer.Save();
  }

  COpenArchiveCallback *openArchiveCallbackSpec = new COpenArchiveCallback;
  CMyComPtr<IArchiveOpenCallback> openArchiveCallback = openArchiveCallbackSpec;

  // if ((opMode & OPM_SILENT) == 0 && (opMode & OPM_FIND ) == 0)
  openArchiveCallbackSpec->Init();
  openArchiveCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex),
      fullName.Mid(fileNamePartStartIndex));
  
  // ::OutputDebugStringA("before OpenArchive\n");
  
  CAgent *agent = new CAgent;
  archiveHandler = agent;
  CMyComBSTR archiveType;
  HRESULT result = archiveHandler->Open(NULL,
      GetUnicodeString(fullName, CP_OEMCP), UString(), &archiveType, openArchiveCallback);
  /*
  HRESULT result = ::OpenArchive(fullName, &archiveHandler,
      archiverInfoResult, defaultName, openArchiveCallback);
  */
  if (result != S_OK)
  {
    if (result == E_ABORT)
      return (HANDLE)-2;
    return INVALID_HANDLE_VALUE;
  }

  UString errorMessage = agent->GetErrorMessage();
  if (!errorMessage.IsEmpty())
    PrintErrorMessage("7-Zip", UnicodeStringToMultiByte(errorMessage, CP_OEMCP));

  // ::OutputDebugStringA("after OpenArchive\n");

  CPlugin *plugin = new CPlugin(
      fullName,
      // defaultName,
      archiveHandler,
      (const wchar_t *)archiveType
      );
  if (plugin == NULL)
    return(INVALID_HANDLE_VALUE);
  plugin->PasswordIsDefined = openArchiveCallbackSpec->PasswordIsDefined;
  plugin->Password = openArchiveCallbackSpec->Password;

  return (HANDLE)(plugin);
}

static HANDLE MyOpenFilePlugin(const char *name)
{
  UINT codePage =
  #ifdef UNDER_CE
    CP_OEMCP;
  #else
    ::AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  #endif
  return MyOpenFilePluginW(GetUnicodeString(name, codePage));
}

EXTERN_C HANDLE WINAPI OpenFilePlugin(char *name, const unsigned char * /* data */, int /* dataSize */)
{
  MY_TRY_BEGIN;
  if (name == NULL || (!g_Options.Enabled))
  {
    // if (!Opt.ProcessShiftF1)
      return(INVALID_HANDLE_VALUE);
  }
  return MyOpenFilePlugin(name);
  MY_TRY_END2("OpenFilePlugin", INVALID_HANDLE_VALUE);
}

/*
EXTERN_C HANDLE WINAPI OpenFilePluginW(const wchar_t *name,const unsigned char *Data,int DataSize,int OpMode)
{
  MY_TRY_BEGIN;
  if (name == NULL || (!g_Options.Enabled))
  {
    // if (!Opt.ProcessShiftF1)
      return(INVALID_HANDLE_VALUE);
  }
  return MyOpenFilePluginW(name);
  ::OutputDebugStringA("OpenFilePluginW\n");
  MY_TRY_END2("OpenFilePluginW", INVALID_HANDLE_VALUE);
}
*/

EXTERN_C HANDLE WINAPI OpenPlugin(int openFrom, INT_PTR item)
{
  MY_TRY_BEGIN;
  if(openFrom == OPEN_COMMANDLINE)
  {
    AString fileName = (const char *)item;
    if(fileName.IsEmpty())
      return INVALID_HANDLE_VALUE;
    if (fileName.Length() >= 2 &&
        fileName[0] == '\"' && fileName[fileName.Length() - 1] == '\"')
      fileName = fileName.Mid(1, fileName.Length() - 2);

    return MyOpenFilePlugin(fileName);
  }
  if(openFrom == OPEN_PLUGINSMENU)
  {
    switch(item)
    {
      case 0:
      {
        PluginPanelItem pluginPanelItem;
        if(!g_StartupInfo.ControlGetActivePanelCurrentItemInfo(pluginPanelItem))
          throw 142134;
        return MyOpenFilePlugin(pluginPanelItem.FindData.cFileName);
      }
      case 1:
      {
        CObjectVector<PluginPanelItem> pluginPanelItem;
        if(!g_StartupInfo.ControlGetActivePanelSelectedOrCurrentItems(pluginPanelItem))
          throw 142134;
        if (CompressFiles(pluginPanelItem) == S_OK)
        {
          /* int t = */ g_StartupInfo.ControlClearPanelSelection();
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

EXTERN_C void WINAPI ClosePlugin(HANDLE plugin)
{
  MY_TRY_BEGIN;
  delete (CPlugin *)plugin;
  MY_TRY_END1("ClosePlugin");
}

EXTERN_C int WINAPI GetFindData(HANDLE plugin, struct PluginPanelItem **panelItems, int *itemsNumber, int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->GetFindData(panelItems, itemsNumber, opMode));
  MY_TRY_END2("GetFindData", FALSE);
}

EXTERN_C void WINAPI FreeFindData(HANDLE plugin, struct PluginPanelItem *panelItems, int itemsNumber)
{
  MY_TRY_BEGIN;
  ((CPlugin *)plugin)->FreeFindData(panelItems, itemsNumber);
  MY_TRY_END1("FreeFindData");
}

EXTERN_C int WINAPI GetFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
    int itemsNumber, int move, char *destPath, int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->GetFiles(panelItems, itemsNumber, move, destPath, opMode));
  MY_TRY_END2("GetFiles", NFileOperationReturnCode::kError);
}

EXTERN_C int WINAPI SetDirectory(HANDLE plugin, const char *dir, int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->SetDirectory(dir, opMode));
  MY_TRY_END2("SetDirectory", FALSE);
}

EXTERN_C void WINAPI GetPluginInfo(struct PluginInfo *info)
{
  MY_TRY_BEGIN;

  info->StructSize = sizeof(*info);
  info->Flags = 0;
  info->DiskMenuStrings = NULL;
  info->DiskMenuNumbers = NULL;
  info->DiskMenuStringsNumber = 0;
  static const char *pluginMenuStrings[2];
  pluginMenuStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  pluginMenuStrings[1] = g_StartupInfo.GetMsgString(NMessageID::kCreateArchiveMenuString);
  info->PluginMenuStrings = (char **)pluginMenuStrings;
  info->PluginMenuStringsNumber = 2;
  static const char *pluginCfgStrings[1];
  pluginCfgStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  info->PluginConfigStrings = (char **)pluginCfgStrings;
  info->PluginConfigStringsNumber = sizeof(pluginCfgStrings) / sizeof(pluginCfgStrings[0]);
  info->CommandPrefix = (char *)kCommandPrefix;
  MY_TRY_END1("GetPluginInfo");
}

EXTERN_C int WINAPI Configure(int /* itemNumber */)
{
  MY_TRY_BEGIN;

  const int kEnabledCheckBoxIndex = 1;

  const int kYSize = 7;

  struct CInitDialogItem initItems[]=
  {
    { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kConfigTitle, NULL, NULL },
    { DI_CHECKBOX, 5, 2, 0, 0, true, g_Options.Enabled, 0, false, NMessageID::kConfigPluginEnabled, NULL, NULL },
    { DI_TEXT, 5, 3, 0, 0, false, false, DIF_BOXCOLOR | DIF_SEPARATOR, false, -1, "", NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kOk, NULL, NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL },
  };

  const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
  const int kOkButtonIndex = kNumDialogItems - 2;

  FarDialogItem dialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);

  int askCode = g_StartupInfo.ShowDialog(76, kYSize,
      kHelpTopicConfig, dialogItems, kNumDialogItems);

  if (askCode != kOkButtonIndex)
    return (FALSE);

  g_Options.Enabled = BOOLToBool(dialogItems[kEnabledCheckBoxIndex].Selected);

  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegisrtryMainKeyName,
      kRegisrtryValueNameEnabled, g_Options.Enabled);
  return(TRUE);
  MY_TRY_END2("Configure", FALSE);
}

EXTERN_C void WINAPI GetOpenPluginInfo(HANDLE plugin,struct OpenPluginInfo *info)
{
  MY_TRY_BEGIN;
  ((CPlugin *)plugin)->GetOpenPluginInfo(info);
  MY_TRY_END1("GetOpenPluginInfo");
}

EXTERN_C int WINAPI PutFiles(HANDLE plugin, struct PluginPanelItem *panelItems, int itemsNumber, int move, int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->PutFiles(panelItems, itemsNumber, move, opMode));
  MY_TRY_END2("PutFiles", NFileOperationReturnCode::kError);
}

EXTERN_C int WINAPI DeleteFiles(HANDLE plugin, PluginPanelItem *panelItems, int itemsNumber, int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->DeleteFiles(panelItems, itemsNumber, opMode));
  MY_TRY_END2("DeleteFiles", FALSE);
}

EXTERN_C int WINAPI ProcessKey(HANDLE plugin, int key, unsigned int controlState)
{
  MY_TRY_BEGIN;
  return (((CPlugin *)plugin)->ProcessKey(key, controlState));
  MY_TRY_END2("ProcessKey", FALSE);
}
