// LangUtils.cpp

#include "StdAfx.h"

#include "LangUtils.h"
#include "Common/StringConvert.h"
#include "Windows/ResourceString.h"
#include "ZipRegistry.h"

CLang g_Lang;
CSysString aLangPath;

class CLangLoader
{
public:
  CLangLoader()
  {
    ReadRegLang(aLangPath);
    if (!aLangPath.IsEmpty())
      g_Lang.Open(aLangPath);
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
