// ExtractDialog.cpp

#include "StdAfx.h"

#include "../../../Common/StringConvert.h"
#include "../../../Common/Wildcard.h"

#include "../../../Windows/FileName.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/ResourceString.h"

#ifndef NO_REGISTRY
#include "../FileManager/HelpUtils.h"
#endif


#include "../FileManager/BrowseDialog.h"
#include "../FileManager/LangUtils.h"
#include "../FileManager/resourceGui.h"

#include "../FileManager/ViewSettings.h"

#include "ExtractDialog.h"
#include "ExtractDialogRes.h"
#include "ExtractRes.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

extern HINSTANCE g_hInstance;

static const UInt32 kPathMode_IDs[] =
{
  IDS_EXTRACT_PATHS_FULL,
  IDS_EXTRACT_PATHS_NO,
  IDS_EXTRACT_PATHS_ABS
};

static const UInt32 kOverwriteMode_IDs[] =
{
  IDS_EXTRACT_OVERWRITE_ASK,
  IDS_EXTRACT_OVERWRITE_WITHOUT_PROMPT,
  IDS_EXTRACT_OVERWRITE_SKIP_EXISTING,
  IDS_EXTRACT_OVERWRITE_RENAME,
  IDS_EXTRACT_OVERWRITE_RENAME_EXISTING
};

#ifndef _SFX

static const
  // NExtract::NPathMode::EEnum
  int
  kPathModeButtonsVals[] =
{
  NExtract::NPathMode::kFullPaths,
  NExtract::NPathMode::kNoPaths,
  NExtract::NPathMode::kAbsPaths
};

static const
  int
  // NExtract::NOverwriteMode::EEnum
  kOverwriteButtonsVals[] =
{
  NExtract::NOverwriteMode::kAsk,
  NExtract::NOverwriteMode::kOverwrite,
  NExtract::NOverwriteMode::kSkip,
  NExtract::NOverwriteMode::kRename,
  NExtract::NOverwriteMode::kRenameExisting
};

#endif

#ifdef LANG

static const UInt32 kLangIDs[] =
{
  IDT_EXTRACT_EXTRACT_TO,
  IDT_EXTRACT_PATH_MODE,
  IDT_EXTRACT_OVERWRITE_MODE,
  // IDX_EXTRACT_ALT_STREAMS,
  IDX_EXTRACT_NT_SECUR,
  IDX_EXTRACT_ELIM_DUP,
  IDG_PASSWORD,
  IDX_PASSWORD_SHOW
};
#endif

// static const int kWildcardsButtonIndex = 2;

#ifndef NO_REGISTRY
static const unsigned kHistorySize = 30;
#endif

#ifndef _SFX

// it's used in CompressDialog also
void AddComboItems(NControl::CComboBox &combo, const UInt32 *langIDs, unsigned numItems, const int *values, int curVal)
{
  int curSel = 0;
  for (unsigned i = 0; i < numItems; i++)
  {
    UString s = LangString(langIDs[i]);
    s.RemoveChar(L'&');
    int index = (int)combo.AddString(s);
    combo.SetItemData(index, i);
    if (values[i] == curVal)
      curSel = i;
  }
  combo.SetCurSel(curSel);
}

// it's used in CompressDialog also
bool GetBoolsVal(const CBoolPair &b1, const CBoolPair &b2)
{
  if (b1.Def) return b1.Val;
  if (b2.Def) return b2.Val;
  return b1.Val;
}

void CExtractDialog::CheckButton_TwoBools(UINT id, const CBoolPair &b1, const CBoolPair &b2)
{
  CheckButton(id, GetBoolsVal(b1, b2));
}

void CExtractDialog::GetButton_Bools(UINT id, CBoolPair &b1, CBoolPair &b2)
{
  bool val = IsButtonCheckedBool(id);
  bool oldVal = GetBoolsVal(b1, b2);
  if (val != oldVal)
    b1.Def = b2.Def = true;
  b1.Val = b2.Val = val;
}

#endif

void StartApplication(const UString &dir, const UString &path)
{
  SHELLEXECUTEINFOW execInfo;
  ZeroMemory(&execInfo, sizeof(execInfo));
  execInfo.cbSize = sizeof(execInfo);
  execInfo.fMask = SEE_MASK_FLAG_DDEWAIT;
  execInfo.hwnd = NULL;
  execInfo.lpVerb = NULL;
  execInfo.lpFile = path;
  execInfo.lpParameters = NULL;
  execInfo.lpDirectory = dir.IsEmpty() ? NULL : (LPCWSTR)dir;
  execInfo.nShow = SW_SHOWNORMAL;
  ShellExecuteExW(&execInfo);
}

bool CExtractDialog::OnInit()
{
  #ifdef LANG
  {
    UString s;
    LangString_OnlyFromLangFile(IDD_EXTRACT, s);
    if (s.IsEmpty())
      GetText(s);
    if (!ArcPath.IsEmpty())
    {
      s += " : ";
      s += ArcPath;
    }
    SetText(s);
    // LangSetWindowText(*this, IDD_EXTRACT);
    LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  }
  #endif
  
  #ifndef _SFX
  _passwordControl.Attach(GetItem(IDE_EXTRACT_PASSWORD));
  _passwordControl.SetText(Password);
  _passwordControl.SetPasswordChar(TEXT('*'));
  _pathName.Attach(GetItem(IDE_EXTRACT_NAME));
  #endif

  _freeSpace.Attach(GetItem(IDC_STATIC_EXTRACT_FREE_SPACE));
  _freeSpace.SetText(L"");

  #ifdef NO_REGISTRY
  
  PathMode = NExtract::NPathMode::kFullPaths;
  OverwriteMode = NExtract::NOverwriteMode::kAsk;
  
  #else
  
  _info.Load();

  if (_info.PathMode == NExtract::NPathMode::kCurPaths)
    _info.PathMode = NExtract::NPathMode::kFullPaths;

  if (!PathMode_Force && _info.PathMode_Force)
    PathMode = _info.PathMode;
  if (!OverwriteMode_Force && _info.OverwriteMode_Force)
    OverwriteMode = _info.OverwriteMode;

  // CheckButton_TwoBools(IDX_EXTRACT_ALT_STREAMS, AltStreams, _info.AltStreams);
  CheckButton_TwoBools(IDX_EXTRACT_NT_SECUR,    NtSecurity, _info.NtSecurity);
  CheckButton_TwoBools(IDX_EXTRACT_ELIM_DUP,    ElimDup,    _info.ElimDup);
  
  CheckButton(IDX_PASSWORD_SHOW, _info.ShowPassword.Val);
  UpdatePasswordControl();

  #endif

  _path.Attach(GetItem(IDC_EXTRACT_PATH));

  UString pathPrefix = DirPath;

  #ifndef _SFX
  
  if (_info.SplitDest.Val)
  {
    CheckButton(IDX_EXTRACT_NAME_ENABLE, true);
    UString pathName;
    SplitPathToParts_Smart(DirPath, pathPrefix, pathName);
    if (pathPrefix.IsEmpty())
      pathPrefix = pathName;
    else
      _pathName.SetText(pathName);
  }
  else
    ShowItem_Bool(IDE_EXTRACT_NAME, false);

  #endif

  _path.SetText(pathPrefix);
  ShowPathFreeSpace(pathPrefix);

  #ifndef NO_REGISTRY
  for (unsigned i = 0; i < _info.Paths.Size() && i < kHistorySize; i++)
    _path.AddString(_info.Paths[i]);
  #endif

  /*
  if (_info.Paths.Size() > 0)
    _path.SetCurSel(0);
  else
    _path.SetCurSel(-1);
  */

  #ifndef _SFX
  m_bOpenOutputFolder = ReadOptOpenOutputFolder();
  CheckButton(IDC_EXTRACT_CHECK_OPEN_OUTPUT_FOLDER, m_bOpenOutputFolder);
  #endif

  #ifndef _SFX
  _pathMode.Attach(GetItem(IDC_EXTRACT_PATH_MODE));
  _overwriteMode.Attach(GetItem(IDC_EXTRACT_OVERWRITE_MODE));

  AddComboItems(_pathMode, kPathMode_IDs, ARRAY_SIZE(kPathMode_IDs), kPathModeButtonsVals, PathMode);
  AddComboItems(_overwriteMode, kOverwriteMode_IDs, ARRAY_SIZE(kOverwriteMode_IDs), kOverwriteButtonsVals, OverwriteMode);
  #endif

  HICON icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON));
  SetIcon(ICON_BIG, icon);
 
  // CWindow filesWindow = GetItem(IDC_EXTRACT_RADIO_FILES);
  // filesWindow.Enable(_enableFilesButton);

  NormalizePosition();

  return CModalDialog::OnInit();
}

#ifndef _SFX
void CExtractDialog::UpdatePasswordControl()
{
  _passwordControl.SetPasswordChar(IsShowPasswordChecked() ? 0 : TEXT('*'));
  UString password;
  _passwordControl.GetText(password);
  _passwordControl.SetText(password);
}
#endif

bool CExtractDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDB_EXTRACT_SET_PATH:
      OnButtonSetPath();
      return true;
    case IDC_EXTRACT_BUTTON_OPEN_PATH:
      OnButtonOpenPath();
      return true;
    case IDC_EXTRACT_CHECK_OPEN_OUTPUT_FOLDER:
      m_bOpenOutputFolder = IsButtonCheckedBool(IDC_EXTRACT_CHECK_OPEN_OUTPUT_FOLDER);
      return true;
    #ifndef _SFX
    case IDC_CHECK_DELETE_SOURCE_FILE:
      m_bDeleteSourceFile = IsButtonCheckedBool(IDC_CHECK_DELETE_SOURCE_FILE);
      return true;
    #endif
    #ifndef _SFX
    case IDX_EXTRACT_NAME_ENABLE:
      ShowItem_Bool(IDE_EXTRACT_NAME, IsButtonCheckedBool(IDX_EXTRACT_NAME_ENABLE));
      return true;
    case IDX_PASSWORD_SHOW:
    {
      UpdatePasswordControl();
      return true;
    }
    #endif
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CExtractDialog::OnButtonSetPath()
{
  UString currentPath;
  _path.GetText(currentPath);
  UString title = LangString(IDS_EXTRACT_SET_FOLDER);
  UString resultPath;
  if (!MyBrowseForFolder(*this, title, currentPath, resultPath))
    return;
  #ifndef NO_REGISTRY
  _path.SetCurSel(-1);
  #endif
  _path.SetText(resultPath);
  ShowPathFreeSpace(resultPath);
}

void AddUniqueString(UStringVector &list, const UString &s)
{
  FOR_VECTOR (i, list)
    if (s.IsEqualTo_NoCase(list[i]))
      return;
  list.Add(s);
}

void CExtractDialog::OnOK()
{
  #ifndef _SFX
  SaveOptOpenOutputFolder(m_bOpenOutputFolder);
  #endif
  #ifndef _SFX
  int pathMode2 = kPathModeButtonsVals[_pathMode.GetCurSel()];
  if (PathMode != NExtract::NPathMode::kCurPaths ||
      pathMode2 != NExtract::NPathMode::kFullPaths)
    PathMode = (NExtract::NPathMode::EEnum)pathMode2;

  OverwriteMode = (NExtract::NOverwriteMode::EEnum)kOverwriteButtonsVals[_overwriteMode.GetCurSel()];

  // _filesMode = (NExtractionDialog::NFilesMode::EEnum)GetFilesMode();

  _passwordControl.GetText(Password);

  #endif

  #ifndef NO_REGISTRY

  // GetButton_Bools(IDX_EXTRACT_ALT_STREAMS, AltStreams, _info.AltStreams);
  GetButton_Bools(IDX_EXTRACT_NT_SECUR,    NtSecurity, _info.NtSecurity);
  GetButton_Bools(IDX_EXTRACT_ELIM_DUP,    ElimDup,    _info.ElimDup);

  bool showPassword = IsShowPasswordChecked();
  if (showPassword != _info.ShowPassword.Val)
  {
    _info.ShowPassword.Def = true;
    _info.ShowPassword.Val = showPassword;
  }

  if (_info.PathMode != pathMode2)
  {
    _info.PathMode_Force = true;
    _info.PathMode = (NExtract::NPathMode::EEnum)pathMode2;
    /*
    // we allow kAbsPaths in registry.
    if (_info.PathMode == NExtract::NPathMode::kAbsPaths)
      _info.PathMode = NExtract::NPathMode::kFullPaths;
    */
  }

  if (!OverwriteMode_Force && _info.OverwriteMode != OverwriteMode)
    _info.OverwriteMode_Force = true;
  _info.OverwriteMode = OverwriteMode;


  #else
  
  ElimDup.Val = IsButtonCheckedBool(IDX_EXTRACT_ELIM_DUP);

  #endif
  
  UString s;
  
  #ifdef NO_REGISTRY
  
  _path.GetText(s);
  
  #else

  int currentItem = _path.GetCurSel();
  if (currentItem == CB_ERR)
  {
    _path.GetText(s);
    if (_path.GetCount() >= kHistorySize)
      currentItem = _path.GetCount() - 1;
  }
  else
    _path.GetLBText(currentItem, s);
  
  #endif

  s.Trim();
  NName::NormalizeDirPathPrefix(s);
  
  #ifndef _SFX
  
  bool splitDest = IsButtonCheckedBool(IDX_EXTRACT_NAME_ENABLE);
  if (splitDest)
  {
    UString pathName;
    _pathName.GetText(pathName);
    pathName.Trim();
    s += pathName;
    NName::NormalizeDirPathPrefix(s);
  }
  if (splitDest != _info.SplitDest.Val)
  {
    _info.SplitDest.Def = true;
    _info.SplitDest.Val = splitDest;
  }

  #endif

  DirPath = s;
  
  #ifndef NO_REGISTRY
  _info.Paths.Clear();
  #ifndef _SFX
  AddUniqueString(_info.Paths, s);
  #endif
  for (int i = 0; i < _path.GetCount(); i++)
    if (i != currentItem)
    {
      UString sTemp;
      _path.GetLBText(i, sTemp);
      sTemp.Trim();
      AddUniqueString(_info.Paths, sTemp);
    }
  _info.Save();
  #endif
  
  CModalDialog::OnOK();
}

#ifndef NO_REGISTRY
#define kHelpTopic "fm/plugins/7-zip/extract.htm"
void CExtractDialog::OnHelp()
{
  ShowHelpWindow(kHelpTopic);
  CModalDialog::OnHelp();
}
#endif

BOOL IsDirectory(const wchar_t * lpszPathFile)
{
  DWORD	dwAttr;

  dwAttr = GetFileAttributesW(lpszPathFile);
  return (dwAttr != (DWORD)-1) && (dwAttr & FILE_ATTRIBUTE_DIRECTORY);
}

void CExtractDialog::OnButtonOpenPath()
{
  UString currentPath;
  _path.GetText(currentPath);

  if (IsDirectory(currentPath))
  {
    StartApplication(currentPath, currentPath);
  }
  else
  {
    wchar_t szMsg[1024];
    wsprintfW(szMsg, L"Folder \"%s\" is not available yet.\n\n"
      L"Note: the program will create the folder automatically when extracting.", (LPCWSTR)currentPath);
    MessageBoxW((HWND)_window, szMsg, L"7-Zip", MB_ICONEXCLAMATION);
  }
}

static int MakeByteSizeString64(wchar_t * lpszBuf, size_t ccBuf, unsigned __int64 n64Byte)
{
  int nRet = 0;

  if (n64Byte < 1000ui64)
  {
    // < 1K
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%I64d B", n64Byte);
  }
  else if (n64Byte < 1024000ui64) // 1024 * 1000
  {
    // 1K <= n64Byte < 1M
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.1f KB", (double)n64Byte / 1024.0);
  }
  else if (n64Byte < 1048576000ui64) //  1024 * 1024 * 1000
  {
    // 1M <= n64Byte < 1G
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.2f MB", (double)n64Byte / 1048576.0); // 1024 * 1024
  }
  else if (n64Byte < 1073741824000ui64) //  1024 * 1024 * 1024 * 1000
  {
    // 1 G <= n64Byte < 1T
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.2f GB", (double)n64Byte / 1073741824.0); // 1024.0F * 1024.0F * 1024.0F
  }
  else
  {
    // n64Byte >= 1T
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.2f TB", (double)n64Byte / 1099511627776.0);
                  // 1024.0F * 1024.0F * 1024.0F * 1024.0F
  }
  return nRet;
}

void CExtractDialog::ShowPathFreeSpace(UString & strPath)
{
  bool bBadPath;
  UString strText;

  strText.Empty();

  bBadPath = false;
  strPath.Trim();
  for (; !IsDirectory(strPath); )
  {
    int n = strPath.ReverseFind(L'\\');
    if (n == -1)
    {
      bBadPath = true;
      break;
    }
    else
    {
      strPath.ReleaseBuf_SetEnd(n);
    }
  }
  if (!bBadPath)
  {
    unsigned __int64 n64FreeBytesAvailable;
    unsigned __int64 n64TotalNumberOfBytes;
    unsigned __int64 n64TotalNumberOfFreeBytes;

    if (GetDiskFreeSpaceExW(strPath, (PULARGE_INTEGER)&n64FreeBytesAvailable, 
      (PULARGE_INTEGER)&n64TotalNumberOfBytes, (PULARGE_INTEGER)&n64TotalNumberOfFreeBytes))
    {
      wchar_t szFreeBytes[1024];
      wchar_t szTotalBytes[1024];
      MakeByteSizeString64(szFreeBytes, 1024-4, n64TotalNumberOfFreeBytes);
      MakeByteSizeString64(szTotalBytes, 1024-4, n64TotalNumberOfBytes);
      int nLen = swprintf(strText.GetBuf(1024), 1024-4,  L"%s Free (Total: %s)", szFreeBytes, szTotalBytes);
      strText.ReleaseBuf_SetEnd(nLen);
    }
  }
  _freeSpace.SetText(strText);
}


bool CExtractDialog::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (itemID == IDC_EXTRACT_PATH)
  {
#ifdef NO_REGISTRY
    if (code == EN_CHANGE)
#else
    if (code == CBN_EDITCHANGE)
#endif
    {
      UString strPath;
      _path.GetText(strPath);

      ShowPathFreeSpace(strPath);
      return true;
    }
#ifndef NO_REGISTRY
    else if (code == CBN_SELCHANGE)
    {
      int nSel = _path.GetCurSel();
      if (nSel != CB_ERR)
      {
        UString strPath;
        _path.GetLBText(nSel, strPath);
        ShowPathFreeSpace(strPath);
      }
      return true;
    }
#endif
  }
  return CModalDialog::OnCommand(code, itemID, lParam);
}
