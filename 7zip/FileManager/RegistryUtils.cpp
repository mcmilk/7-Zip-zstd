// RegistryUtils.cpp

#include "StdAfx.h"

#include "RegistryUtils.h"
#include "Windows/Registry.h"

using namespace NWindows;
using namespace NRegistry;

static const TCHAR *kCUBasePath = TEXT("Software\\7-ZIP");

static const TCHAR *kCU_FMPath = TEXT("Software\\7-ZIP\\FM");
static const TCHAR *kLangValueName = TEXT("Lang");
static const TCHAR *kEditorValueName = TEXT("Editor");
static const TCHAR *kShowDotsValueName = TEXT("ShowDots");
static const TCHAR *kShowRealFileIconsValueName = TEXT("ShowRealFileIcons");
static const TCHAR *kShowSystemMenuValueName = TEXT("ShowSystemMenu");

void SaveRegLang(const CSysString &langFile)
{
  CKey cuKey;
  cuKey.Create(HKEY_CURRENT_USER, kCUBasePath);
  cuKey.SetValue(kLangValueName, langFile);
}

void ReadRegLang(CSysString &langFile)
{
  langFile.Empty();
  CKey cuKey;
  cuKey.Create(HKEY_CURRENT_USER, kCUBasePath);
  cuKey.QueryValue(kLangValueName, langFile);
}

void SaveRegEditor(const CSysString &editorPath)
{
  CKey cuKey;
  cuKey.Create(HKEY_CURRENT_USER, kCU_FMPath);
  cuKey.SetValue(kEditorValueName, editorPath);
}

void ReadRegEditor(CSysString &editorPath)
{
  editorPath.Empty();
  CKey cuKey;
  cuKey.Create(HKEY_CURRENT_USER, kCU_FMPath);
  cuKey.QueryValue(kEditorValueName, editorPath);
  /*
  if (editorPath.IsEmpty())
    editorPath = TEXT("notepad.exe");
  */
}

static void SaveOption(const TCHAR *value, bool enabled)
{
  CKey cuKey;
  cuKey.Create(HKEY_CURRENT_USER, kCU_FMPath);
  cuKey.SetValue(value, enabled);
}

static bool ReadOption(const TCHAR *value, bool defaultValue)
{
  CKey cuKey;
  cuKey.Create(HKEY_CURRENT_USER, kCU_FMPath);
  bool enabled;
  if (cuKey.QueryValue(value, enabled) != ERROR_SUCCESS)
    return defaultValue;
  return enabled;
}

void SaveShowDots(bool showDots)
  { SaveOption(kShowDotsValueName, showDots); }
bool ReadShowDots()
  { return ReadOption(kShowDotsValueName, false); }

void SaveShowRealFileIcons(bool show)
  { SaveOption(kShowRealFileIconsValueName, show); }
bool ReadShowRealFileIcons()
  { return ReadOption(kShowRealFileIconsValueName, false); }

void SaveShowSystemMenu(bool show)
  { SaveOption(kShowSystemMenuValueName, show); }
bool ReadShowSystemMenu()
  { return ReadOption(kShowSystemMenuValueName, false); }
