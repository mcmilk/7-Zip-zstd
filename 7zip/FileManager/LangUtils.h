// LangUtils.h

#ifndef __LANGUTILS_H
#define __LANGUTILS_H

#include "Common/Lang.h"

extern CSysString g_LangID;

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

void LangSetDlgItemsText(HWND dialogWindow, CIDLangPair *idLangPairs, int numItems);
void LangSetWindowText(HWND window, UInt32 langID);

UString LangLoadString(UInt32 langID);
CSysString LangLoadString(UINT resourceID, UInt32 langID);
UString LangLoadStringW(UINT resourceID, UInt32 langID);


#endif
