// LangUtils.cpp

#include "StdAfx.h"

#include "LangUtils.h"
#include "Common/StringConvert.h"
#include "Windows/ResourceString.h"
#include "Windows/Window.h"
#include "RegistryUtils.h"

CLang g_Lang;
CSysString g_LangPath;

void ReloadLang()
{
  ReadRegLang(g_LangPath);
  g_Lang.Clear();
  if (!g_LangPath.IsEmpty())
    g_Lang.Open(g_LangPath);
}

class CLangLoader
{
public:
  CLangLoader()
  {
    ReloadLang();
  }
} g_LangLoader;

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
