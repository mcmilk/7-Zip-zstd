// LangUtils.h

#ifndef __LANG_UTILS_H
#define __LANG_UTILS_H

#include "Common/Lang.h"
#include "Windows/ResourceString.h"

extern UString g_LangID;

struct CIDLangPair
{
  int ControlID;
  UInt32 LangID;
};

void ReloadLang();
void LoadLangOneTime();
void ReloadLangSmart();

struct CLangEx
{
  CLang Lang;
  UString ShortName;
};

void LoadLangs(CObjectVector<CLangEx> &langs);

void LangSetDlgItemsText(HWND dialogWindow, const CIDLangPair *idLangPairs, int numItems);
void LangSetWindowText(HWND window, UInt32 langID);

UString LangString(UInt32 langID);
UString LangString(UINT resourceID, UInt32 langID);

#ifdef LANG
#define LangStringSpec(resourceID, langID) LangString(resourceID, langID)
#else
#define LangStringSpec(resourceID, langID) NWindows::MyLoadStringW(resourceID)
#endif

#endif
