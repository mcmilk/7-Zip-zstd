// LangUtils.cpp

#include "StdAfx.h"

#include "LangUtils.h"
#include "Common/StringConvert.h"
#include "Windows/ResourceString.h"
#include "Windows/Synchronization.h"
#include "Windows/Window.h"
#include "RegistryUtils.h"
#include "ProgramLocation.h"

using namespace NWindows;

static CLang g_Lang;
CSysString g_LangID;

void ReloadLang()
{
  ReadRegLang(g_LangID);
  g_Lang.Clear();
  if (!g_LangID.IsEmpty())
  {
    CSysString langPath = g_LangID;
    if (langPath.Find('\\') < 0)
    {
      if (langPath.Find('.') < 0)
        langPath += TEXT(".txt");
      UString folderPath;
      if (GetProgramFolderPath(folderPath))
        langPath = GetSystemString(folderPath) + CSysString(TEXT("Lang\\")) + langPath;
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

/*
class CLangLoader
{
public:
  CLangLoader() { ReloadLang(); }
} g_LangLoader;
*/

void LangSetDlgItemsText(HWND dialogWindow, CIDLangPair *idLangPairs, int numItems)
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
    SetWindowText(window, GetSystemString(message));
}

UString LangLoadString(UInt32 langID)
{
  UString message;
  if (g_Lang.GetMessage(langID, message))
    return message;
  return UString();
}

CSysString LangLoadString(UINT resourceID, UInt32 langID)
{
  UString message;
  if (g_Lang.GetMessage(langID, message))
    return GetSystemString(message);
  return NWindows::MyLoadString(resourceID);
}

UString LangLoadStringW(UINT resourceID, UInt32 langID)
{
  UString message;
  if (g_Lang.GetMessage(langID, message))
    return message;
  return NWindows::MyLoadStringW(resourceID);
}
