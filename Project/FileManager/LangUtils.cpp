// LangUtils.cpp

#include "StdAfx.h"

#include "LangUtils.h"
#include "Common/StringConvert.h"
#include "Windows/ResourceString.h"
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

void LangSetDlgItemsText(HWND aDialogWindow, CIDLangPair *anIDLangPairs, int aNumItems)
{
  for (int i = 0; i < aNumItems; i++)
  {
    const CIDLangPair &anIDLangPair = anIDLangPairs[i];
    UString aMessage;
    if (g_Lang.GetMessage(anIDLangPair.LangID, aMessage))
      SetDlgItemText(aDialogWindow, anIDLangPair.ControlID, GetSystemString(aMessage));
  }
}

void LangSetWindowText(HWND aWindow, UINT32 aLangID)
{
  UString aMessage;
  if (g_Lang.GetMessage(aLangID, aMessage))
    SetWindowText(aWindow, GetSystemString(aMessage));
}

CSysString LangLoadString(UINT32 aLangID)
{
  UString aMessage;
  if (g_Lang.GetMessage(aLangID, aMessage))
    return GetSystemString(aMessage);
  return CSysString();
}

CSysString LangLoadString(UINT aResourceID, UINT32 aLangID)
{
  UString aMessage;
  if (g_Lang.GetMessage(aLangID, aMessage))
    return GetSystemString(aMessage);
  return NWindows::MyLoadString(aResourceID);
}

UString LangLoadStringW(UINT aResourceID, UINT32 aLangID)
{
  UString aMessage;
  if (g_Lang.GetMessage(aLangID, aMessage))
    return aMessage;
  return GetUnicodeString(NWindows::MyLoadString(aResourceID));
}
