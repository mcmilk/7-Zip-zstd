// LangUtils.h

#pragma once

#ifndef __LANGUTILS_H
#define __LANGUTILS_H

#include "Common/Lang.h"

// extern CLang g_Lang;
extern CSysString g_LangPath;

struct CIDLangPair
{
  int ControlID;
  UINT32 LangID;
};

void ReloadLang();

void LangSetDlgItemsText(HWND dialogWindow, CIDLangPair *idLangPairs, int numItems);
void LangSetWindowText(HWND window, UINT32 langID);

CSysString LangLoadString(UINT32 langID);
CSysString LangLoadString(UINT resourceID, UINT32 langID);
UString LangLoadStringW(UINT resourceID, UINT32 langID);


#endif



