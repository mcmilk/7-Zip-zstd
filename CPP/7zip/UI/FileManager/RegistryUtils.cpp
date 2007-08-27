// RegistryUtils.cpp

#include "StdAfx.h"

#include "RegistryUtils.h"
#include "Windows/Registry.h"

using namespace NWindows;
using namespace NRegistry;

static const TCHAR *kCUBasePath = TEXT("Software\\7-ZIP");
static const TCHAR *kCU_FMPath = TEXT("Software\\7-ZIP\\FM");
// static const TCHAR *kLM_Path = TEXT("Software\\7-ZIP\\FM");

static const WCHAR *kLangValueName = L"Lang";
static const WCHAR *kEditor = L"Editor";
static const TCHAR *kShowDots = TEXT("ShowDots");
static const TCHAR *kShowRealFileIcons = TEXT("ShowRealFileIcons");
static const TCHAR *kShowSystemMenu = TEXT("ShowSystemMenu");

static const TCHAR *kFullRow = TEXT("FullRow");
static const TCHAR *kShowGrid = TEXT("ShowGrid");
static const TCHAR *kAlternativeSelection = TEXT("AlternativeSelection");
// static const TCHAR *kLockMemoryAdd = TEXT("LockMemoryAdd");
static const TCHAR *kLargePagesEnable = TEXT("LargePages");
// static const TCHAR *kSingleClick = TEXT("SingleClick");
// static const TCHAR *kUnderline = TEXT("Underline");

void SaveRegLang(const UString &langFile)
{
  CKey key;
  key.Create(HKEY_CURRENT_USER, kCUBasePath);
  key.SetValue(kLangValueName, langFile);
}

void ReadRegLang(UString &langFile)
{
  langFile.Empty();
  CKey key;
  if (key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) == ERROR_SUCCESS)
    key.QueryValue(kLangValueName, langFile);
}

void SaveRegEditor(const UString &editorPath)
{
  CKey key;
  key.Create(HKEY_CURRENT_USER, kCU_FMPath);
  key.SetValue(kEditor, editorPath);
}

void ReadRegEditor(UString &editorPath)
{
  editorPath.Empty();
  CKey key;
  if (key.Open(HKEY_CURRENT_USER, kCU_FMPath, KEY_READ) == ERROR_SUCCESS)
    key.QueryValue(kEditor, editorPath);
}

static void Save7ZipOption(const TCHAR *value, bool enabled)
{
  CKey key;
  key.Create(HKEY_CURRENT_USER, kCUBasePath);
  key.SetValue(value, enabled);
}

static void SaveOption(const TCHAR *value, bool enabled)
{
  CKey key;
  key.Create(HKEY_CURRENT_USER, kCU_FMPath);
  key.SetValue(value, enabled);
}

static bool Read7ZipOption(const TCHAR *value, bool defaultValue)
{
  CKey key;
  if (key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) == ERROR_SUCCESS)
  {
    bool enabled;
    if (key.QueryValue(value, enabled) == ERROR_SUCCESS)
      return enabled;
  }
  return defaultValue;
}

static bool ReadOption(const TCHAR *value, bool defaultValue)
{
  CKey key;
  if (key.Open(HKEY_CURRENT_USER, kCU_FMPath, KEY_READ) == ERROR_SUCCESS)
  {
    bool enabled;
    if (key.QueryValue(value, enabled) == ERROR_SUCCESS)
      return enabled;
  }
  return defaultValue;
}

/*
static void SaveLmOption(const TCHAR *value, bool enabled)
{
  CKey key;
  key.Create(HKEY_LOCAL_MACHINE, kLM_Path);
  key.SetValue(value, enabled);
}

static bool ReadLmOption(const TCHAR *value, bool defaultValue)
{
  CKey key;
  if (key.Open(HKEY_LOCAL_MACHINE, kLM_Path, KEY_READ) == ERROR_SUCCESS)
  {
    bool enabled;
    if (key.QueryValue(value, enabled) == ERROR_SUCCESS)
      return enabled;
  }
  return defaultValue;
}
*/

void SaveShowDots(bool showDots) { SaveOption(kShowDots, showDots); }
bool ReadShowDots() { return ReadOption(kShowDots, false); }

void SaveShowRealFileIcons(bool show)  { SaveOption(kShowRealFileIcons, show); }
bool ReadShowRealFileIcons() { return ReadOption(kShowRealFileIcons, false); }

void SaveShowSystemMenu(bool show) { SaveOption(kShowSystemMenu, show); }
bool ReadShowSystemMenu(){ return ReadOption(kShowSystemMenu, false); }

void SaveFullRow(bool enable) { SaveOption(kFullRow, enable); }
bool ReadFullRow() { return ReadOption(kFullRow, false); }

void SaveShowGrid(bool enable) { SaveOption(kShowGrid, enable); }
bool ReadShowGrid(){ return ReadOption(kShowGrid, false); }

void SaveAlternativeSelection(bool enable) { SaveOption(kAlternativeSelection, enable); }
bool ReadAlternativeSelection(){ return ReadOption(kAlternativeSelection, false); }

/*
void SaveSingleClick(bool enable) { SaveOption(kSingleClick, enable); }
bool ReadSingleClick(){ return ReadOption(kSingleClick, false); }

void SaveUnderline(bool enable) { SaveOption(kUnderline, enable); }
bool ReadUnderline(){ return ReadOption(kUnderline, false); }
*/

// void SaveLockMemoryAdd(bool enable) { SaveLmOption(kLockMemoryAdd, enable); }
// bool ReadLockMemoryAdd() { return ReadLmOption(kLockMemoryAdd, true); }

void SaveLockMemoryEnable(bool enable) { Save7ZipOption(kLargePagesEnable, enable); }
bool ReadLockMemoryEnable() { return Read7ZipOption(kLargePagesEnable, false); }


