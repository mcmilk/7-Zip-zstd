// RegistryUtils.cpp

#include "StdAfx.h"

#include "RegistryUtils.h"
#include "Windows/Registry.h"

using namespace NWindows;
using namespace NRegistry;

static const TCHAR *kCUBasePath = TEXT("Software\\7-ZIP");
static const TCHAR *kCU_FMPath = TEXT("Software\\7-ZIP\\FM");

static const TCHAR *kLangValueName = TEXT("Lang");
static const TCHAR *kEditor = TEXT("Editor");
static const TCHAR *kShowDots = TEXT("ShowDots");
static const TCHAR *kShowRealFileIcons = TEXT("ShowRealFileIcons");
static const TCHAR *kShowSystemMenu = TEXT("ShowSystemMenu");

static const TCHAR *kFullRow = TEXT("FullRow");
static const TCHAR *kShowGrid = TEXT("ShowGrid");
static const TCHAR *kAlternativeSelection = TEXT("AlternativeSelection");
// static const TCHAR *kSingleClick = TEXT("SingleClick");
// static const TCHAR *kUnderline = TEXT("Underline");

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
  cuKey.SetValue(kEditor, editorPath);
}

void ReadRegEditor(CSysString &editorPath)
{
  editorPath.Empty();
  CKey cuKey;
  cuKey.Create(HKEY_CURRENT_USER, kCU_FMPath);
  cuKey.QueryValue(kEditor, editorPath);
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