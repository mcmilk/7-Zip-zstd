// RegistryUtils.cpp

#include "StdAfx.h"

#include "RegistryUtils.h"
#include "Windows/Registry.h"

using namespace NWindows;
using namespace NRegistry;

static const TCHAR *kCUBasePath = _T("Software\\7-ZIP");

static const TCHAR *kCU_FMPath = _T("Software\\7-ZIP\\FM");
static const TCHAR *kLangValueName = _T("Lang");
static const TCHAR *kEditorValueName = _T("Editor");

void SaveRegLang(const CSysString &langFile)
{
  CKey aCUKey;
  aCUKey.Create(HKEY_CURRENT_USER, kCUBasePath);
  aCUKey.SetValue(kLangValueName, langFile);
}

void ReadRegLang(CSysString &langFile)
{
  langFile.Empty();
  CKey aCUKey;
  aCUKey.Create(HKEY_CURRENT_USER, kCUBasePath);
  aCUKey.QueryValue(kLangValueName, langFile);
}

void SaveRegEditor(const CSysString &editorPath)
{
  CKey aCUKey;
  aCUKey.Create(HKEY_CURRENT_USER, kCU_FMPath);
  aCUKey.SetValue(kEditorValueName, editorPath);
}

void ReadRegEditor(CSysString &editorPath)
{
  editorPath.Empty();
  CKey aCUKey;
  aCUKey.Create(HKEY_CURRENT_USER, kCU_FMPath);
  aCUKey.QueryValue(kEditorValueName, editorPath);
  /*
  if (editorPath.IsEmpty())
    editorPath = TEXT("notepad.exe");
  */
}

