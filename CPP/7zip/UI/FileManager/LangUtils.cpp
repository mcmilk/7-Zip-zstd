// LangUtils.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/DLL.h"
#include "Windows/FileFind.h"
#include "Windows/Synchronization.h"
#include "Windows/Window.h"

#include "LangUtils.h"
#include "RegistryUtils.h"

using namespace NWindows;

static CLang g_Lang;
UString g_LangID;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

static FString GetLangDirPrefix()
{
  return NDLL::GetModuleDirPrefix() + FString(FTEXT("Lang") FSTRING_PATH_SEPARATOR);
}

void ReloadLang()
{
  ReadRegLang(g_LangID);
  g_Lang.Clear();
  if (!g_LangID.IsEmpty() && g_LangID != L"-")
  {
    FString langPath = us2fs(g_LangID);
    if (langPath.Find(WCHAR_PATH_SEPARATOR) < 0)
    {
      if (langPath.Find(FTEXT('.')) < 0)
        langPath += FTEXT(".txt");
      langPath = GetLangDirPrefix() + langPath;
    }
    g_Lang.Open(langPath);
  }
}

static bool g_Loaded = false;
static NSynchronization::CCriticalSection g_CriticalSection;

void LoadLangOneTime()
{
  NSynchronization::CCriticalSectionLock lock(g_CriticalSection);
  if (g_Loaded)
    return;
  g_Loaded = true;
  ReloadLang();
}

void LangSetDlgItemsText(HWND dialogWindow, const CIDLangPair *idLangPairs, int numItems)
{
  for (int i = 0; i < numItems; i++)
  {
    const CIDLangPair &idLangPair = idLangPairs[i];
    UString message;
    if (g_Lang.GetMessage(idLangPair.LangID, message))
    {
      NWindows::CWindow window(GetDlgItem(dialogWindow, idLangPair.ControlID));
      window.SetText(message);
    }
  }
}

void LangSetWindowText(HWND window, UInt32 langID)
{
  UString message;
  if (g_Lang.GetMessage(langID, message))
    MySetWindowText(window, message);
}

UString LangString(UInt32 langID)
{
  UString message;
  if (g_Lang.GetMessage(langID, message))
    return message;
  return UString();
}

UString LangString(UINT resourceID, UInt32 langID)
{
  UString message;
  if (g_Lang.GetMessage(langID, message))
    return message;
  return MyLoadStringW(resourceID);
}

void LoadLangs(CObjectVector<CLangEx> &langs)
{
  langs.Clear();
  const FString dirPrefix = GetLangDirPrefix();
  NFile::NFind::CEnumerator enumerator(dirPrefix + FTEXT("*.txt"));
  NFile::NFind::CFileInfo fi;
  while (enumerator.Next(fi))
  {
    if (fi.IsDir())
      continue;
    const int kExtSize = 4;
    const FString ext = fi.Name.Right(kExtSize);
    if (ext.CompareNoCase(FTEXT(".txt")) != 0)
      continue;
    CLangEx lang;
    lang.ShortName = fs2us(fi.Name.Left(fi.Name.Length() - kExtSize));
    if (lang.Lang.Open(dirPrefix + fi.Name))
      langs.Add(lang);
  }
}

bool SplidID(const UString &id, WORD &primID, WORD &subID)
{
  primID = 0;
  subID = 0;
  const wchar_t *start = id;
  const wchar_t *end;
  UInt64 value = ConvertStringToUInt64(start, &end);
  if (start == end)
    return false;
  primID = (WORD)value;
  if (*end == 0)
    return true;
  if (*end != L'-')
    return false;
  start = end + 1;
  value = ConvertStringToUInt64(start, &end);
  if (start == end)
    return false;
  subID = (WORD)value;
  return (*end == 0);
}

typedef LANGID (WINAPI *GetUserDefaultUILanguageP)();

void FindMatchLang(UString &shortName)
{
  shortName.Empty();

  LANGID SystemDefaultLangID = GetSystemDefaultLangID(); // Lang for non-Unicode in XP64
  LANGID UserDefaultLangID = GetUserDefaultLangID(); // Standarts and formats in XP64

  if (SystemDefaultLangID != UserDefaultLangID)
    return;
  LANGID langID = UserDefaultLangID;
  /*
  LANGID SystemDefaultUILanguage; // english  in XP64
  LANGID UserDefaultUILanguage; // english  in XP64

  GetUserDefaultUILanguageP fn = (GetUserDefaultUILanguageP)GetProcAddress(
      GetModuleHandle("kernel32"), "GetUserDefaultUILanguage");
  if (fn != NULL)
    UserDefaultUILanguage = fn();
  fn = (GetUserDefaultUILanguageP)GetProcAddress(
      GetModuleHandle("kernel32"), "GetSystemDefaultUILanguage");
  if (fn != NULL)
    SystemDefaultUILanguage = fn();
  */

  WORD primLang = (WORD)(PRIMARYLANGID(langID));
  WORD subLang = (WORD)(SUBLANGID(langID));
  CObjectVector<CLangEx> langs;
  LoadLangs(langs);
  for (int i = 0; i < langs.Size(); i++)
  {
    const CLangEx &lang = langs[i];
    UString id;
    if (lang.Lang.GetMessage(0x00000002, id))
    {
      WORD primID;
      WORD subID;
      if (SplidID(id, primID, subID))
        if (primID == primLang)
        {
          if (subID == 0)
            shortName = lang.ShortName;
          if (subLang == subID)
          {
            shortName = lang.ShortName;
            return;
          }
        }
    }
  }
}

void ReloadLangSmart()
{
  #ifndef _UNICODE
  if (g_IsNT)
  #endif
  {
    ReadRegLang(g_LangID);
    if (g_LangID.IsEmpty())
    {
      UString shortName;
      FindMatchLang(shortName);
      if (shortName.IsEmpty())
        shortName = L"-";
      SaveRegLang(shortName);
    }
  }
  ReloadLang();
}
