// MyLoadMenu

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Interface/PropID.h"

#include "Windows/Menu.h"
#include "Windows/Error.h"

#include "resource.h"
#include "App.h"
#include "Resource/AboutDialog/AboutDialog.h"

#include "HelpUtils.h"
#include "LangUtils.h"

extern CApp g_App;
extern HINSTANCE g_hInstance;	

static LPCTSTR kFMHelpTopic = _T("FM/index.htm");

extern void OptionsDialog(HWND hwndOwner, HINSTANCE hInstance);

using namespace NWindows;

static const int kViewMenuIndex = 2;


struct CStringLangPair
{
  wchar_t *String;
  UINT32 LangID;
};

static CStringLangPair kStringLangPairs[] = 
{
  { L"&File",  0x03000102 },
  { L"&Edit",  0x03000103 },
  { L"&View",  0x03000104 },
  { L"&Tools", 0x03000105 },
  { L"&Help",  0x03000106 },
};

/*
static int FindStringLangItem(const UString &anItem)
{
  for (int i = 0; i < sizeof(kStringLangPairs) / 
      sizeof(kStringLangPairs[0]); i++)
    if (anItem.CompareNoCase(kStringLangPairs[i].String) == 0)
      return i;
  return -1;
}
*/

static CIDLangPair kIDLangPairs[] = 
{
  // File
  { IDM_FILE_OPEN, 0x03000210 },
  { IDM_FILE_OPEN_INSIDE, 0x03000211 },
  { IDM_FILE_OPEN_OUTSIDE, 0x03000212 },
  { IDM_FILE_VIEW, 0x03000220 },
  { IDM_FILE_EDIT, 0x03000221 },
  { IDM_RENAME, 0x03000230 },
  { IDM_COPY_TO, 0x03000231 },
  { IDM_MOVE_TO, 0x03000232 },
  { IDM_DELETE, 0x03000233 },
  { ID_FILE_PROPERTIES, 0x03000240 },
  { IDM_CREATE_FOLDER, 0x03000250 },
  { IDM_CREATE_FILE, 0x03000251 },
  { IDCLOSE, 0x03000260 },

  // Edit
  { IDM_EDIT_CUT, 0x03000320 },
  { IDM_EDIT_COPY, 0x03000321 },
  { IDM_EDIT_PASTE, 0x03000322 },

  { IDM_SELECT_ALL, 0x03000330 },
  { IDM_DESELECT_ALL, 0x03000331 },
  { IDM_INVERT_SELECTION, 0x03000332 },
  { IDM_SELECT, 0x03000333 },
  { IDM_DESELECT, 0x03000334 },
  { IDM_SELECT_BY_TYPE, 0x03000335 },
  { IDM_DESELECT_BY_TYPE, 0x03000336 },

  { IDM_VIEW_LARGE_ICONS, 0x03000410 },
  { IDM_VIEW_SMALL_ICONS, 0x03000411 },
  { IDM_VIEW_LIST, 0x03000412 },
  { IDM_VIEW_DETAILS, 0x03000413 },

  { IDM_VIEW_ARANGE_BY_NAME, 0x02000204 },
  { IDM_VIEW_ARANGE_BY_TYPE, 0x02000214 },
  { IDM_VIEW_ARANGE_BY_DATE, 0x0200020C },
  { IDM_VIEW_ARANGE_BY_SIZE, 0x02000207 },
  { IDM_VIEW_ARANGE_NO_SORT, 0x03000420 },

  { IDM_OPEN_ROOT_FOLDER, 0x03000430 },
  { IDM_OPEN_PARENT_FOLDER, 0x03000431 },
  { IDM_FOLDERS_HISTORY, 0x03000432 },

  { IDM_VIEW_REFRESH, 0x03000440 },

  { IDM_OPTIONS, 0x03000510 },
  
  { IDM_HELP_CONTENTS, 0x03000610 },
  { IDM_ABOUT, 0x03000620 }
};


static int FindLangItem(int ControlID)
{
  for (int i = 0; i < sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]); i++)
    if (kIDLangPairs[i].ControlID == ControlID)
      return i;
  return -1;
}

/*
void MyChangeMenu(HMENU menuLoc, int baseIndex = -1)
{
  CMenu menu;
  menu.Attach(menuLoc);
  for (int i = 0; i < menu.GetItemCount(); i++)
  {
    HMENU subMenu = menu.GetSubMenu(i);
    CSysString menuString;
    menu.GetMenuString(i, MF_BYPOSITION, menuString);

    // if (menu.GetItemInfo(i, true, &menuInfo))
    {
      CSysString newString;
      if (subMenu)
      {
        MyChangeMenu(subMenu);
        if (baseIndex >= 0 && baseIndex < sizeof(kStringLangPairs) / 
              sizeof(kStringLangPairs[0]))
          newString = LangLoadString(kStringLangPairs[baseIndex++].LangID);
        else
          continue;
        if (newString.IsEmpty())
          continue;

        // int langPos = FindStringLangItem(GetUnicodeString(menuInfo.dwTypeData));
        // if (langPos >= 0)
        //   newString = LangLoadString(kStringLangPairs[langPos].LangID);
        // else
        //   newString = menuInfo.dwTypeData;
      }
      else
      {
        UINT id = menu.GetItemID(i);
        int langPos = FindLangItem(id);
        if (langPos < 0)
          continue;
        newString = LangLoadString(kIDLangPairs[langPos].LangID);
        if (newString.IsEmpty())
          continue;
        int tabPos = menuString.ReverseFind(wchar_t('\t'));
        if (tabPos >= 0)
          newString += menuString.Mid(tabPos);
      }
      MENUITEMINFO menuInfo;
      menuInfo.cbSize = sizeof(menuInfo);
      menuInfo.fType = MFT_STRING;
      menuInfo.fMask = MIIM_TYPE;
      menuInfo.dwTypeData = (LPTSTR)(LPCTSTR)newString;
      menu.SetItemInfo(i, true, &menuInfo);
      // HMENU subMenu = menu.GetSubMenu(i);
    }
  }
}
*/

void MyChangeMenu(HMENU menuLoc, int baseIndex = -1)
{
  CMenu menu;
  menu.Attach(menuLoc);
  for (int i = 0; i < menu.GetItemCount(); i++)
  {
    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_ID;
    menuInfo.fType = MFT_STRING;
    const int kBufferSize = 1024;
    TCHAR buffer[kBufferSize + 1];
    menuInfo.dwTypeData = buffer;
    menuInfo.cch = kBufferSize;
    if (menu.GetItemInfo(i, true, &menuInfo))
    {
      CSysString newString;
      if (menuInfo.hSubMenu)
      {
        MyChangeMenu(menuInfo.hSubMenu);
        if (baseIndex >= 0 && baseIndex < sizeof(kStringLangPairs) / 
              sizeof(kStringLangPairs[0]))
          newString = LangLoadString(kStringLangPairs[baseIndex++].LangID);
        else
          continue;
        if (newString.IsEmpty())
          continue;

        // int langPos = FindStringLangItem(GetUnicodeString(menuInfo.dwTypeData));
        // if (langPos >= 0)
        //   newString = LangLoadString(kStringLangPairs[langPos].LangID);
        // else
        //   newString = menuInfo.dwTypeData;
      }
      else
      {
        int langPos = FindLangItem(menuInfo.wID);
        if (langPos < 0)
          continue;
        newString = LangLoadString(kIDLangPairs[langPos].LangID);
        if (newString.IsEmpty())
          continue;
        CSysString shorcutString = menuInfo.dwTypeData;
        int tabPos = shorcutString.ReverseFind(wchar_t('\t'));
        if (tabPos >= 0)
          newString += shorcutString.Mid(tabPos);
      }
      menuInfo.dwTypeData = (LPTSTR)(LPCTSTR)newString;
      menuInfo.fMask = MIIM_STRING;
      menu.SetItemInfo(i, true, &menuInfo);
      // HMENU subMenu = menu.GetSubMenu(i);
    }
  }
}

void MyLoadMenu(HWND hWnd)
{
  HMENU oldMenu = ::GetMenu(hWnd);
  HMENU baseMenu = ::LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_MENU));
  ::SetMenu(hWnd, baseMenu);
  ::DestroyMenu(oldMenu);
  if (!g_LangPath.IsEmpty())
  {
    HMENU menuOld = ::GetMenu(hWnd);
    MyChangeMenu(menuOld, 0);
  }
  ::DrawMenuBar(hWnd);
}

extern HWND g_HWND;
void MyLoadMenu()
{
  MyLoadMenu(g_HWND);
}

void OnMenuActivating(HWND hWnd, HMENU hMenu, int position)
{
  if (position == kViewMenuIndex)
  {
    // View;
    CMenu menu;
    menu.Attach(hMenu);
    menu.CheckRadioItem(IDM_VIEW_LARGE_ICONS, IDM_VIEW_DETAILS, 
      IDM_VIEW_LARGE_ICONS + g_App.GetListViewMode(), MF_BYCOMMAND);
  }
}

void LoadFileMenu(HMENU hMenu, int startPos, bool forFileMode)
{
  CMenu destMenu;
  destMenu.Attach(hMenu);
  CMenu srcMenu;
  srcMenu.Attach(::GetSubMenu(::GetMenu(g_HWND), 0));
  
  for (int i = 0; i < srcMenu.GetItemCount(); i++)
  {
    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(menuInfo);

    /*
    menuInfo.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
    menuInfo.fType = MFT_STRING;

    if (!srcMenu.GetItemInfo(i, true, &menuInfo))
    {
      // MessageBox(0, NError::MyFormatMessage(GetLastError()), "Error", 0);
      continue;
    }
    // menuInfo.wID = srcMenu.GetItemID(i);
    // menuInfo.fState = srcMenu.GetItemState(i, MF_BYPOSITION);

    // menuInfo.hSubMenu = srcMenu.GetSubMenu(i);
    CSysString menuString;
    if (menuInfo.fType == MFT_STRING)
    {
      srcMenu.GetMenuString(i, MF_BYPOSITION, menuString);
      menuInfo.dwTypeData = (LPTSTR)(LPCTSTR)menuString;
    }
    menuInfo.dwTypeData = (LPTSTR)(LPCTSTR)menuString;
    */
    
    menuInfo.fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE | MIIM_STRING;
    menuInfo.fType = MFT_STRING;
    const int kBufferSize = 1024;
    TCHAR buffer[kBufferSize + 1];
    menuInfo.dwTypeData = buffer;
    menuInfo.cch = kBufferSize;

    if (srcMenu.GetItemInfo(i, true, &menuInfo))
    {
      if (menuInfo.wID == IDCLOSE)
        continue;
      bool createItem = (menuInfo.wID == IDM_CREATE_FOLDER || 
          menuInfo.wID == IDM_CREATE_FILE);
      if (forFileMode)
      {
        if (createItem)
         continue;
      }
      else
      {
        if (!createItem)
         continue;
      }
      if (destMenu.InsertItem(startPos, true, &menuInfo))
        startPos++;
    }
  }
  while (destMenu.GetItemCount() > 0)
  {
    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIIM_TYPE;
    int lastIndex = destMenu.GetItemCount() - 1;
    if (!destMenu.GetItemInfo(lastIndex, true, &menuInfo))
      break;
    if(menuInfo.fType != MFT_SEPARATOR)
      break;
    if (!destMenu.RemoveItem(lastIndex, MF_BYPOSITION))
      break;
  }
}

bool ExecuteFileCommand(int id)
{
  switch (id)
  {	
    // File
    case IDM_FILE_OPEN:
      g_App.OpenItem();
      break;
    case IDM_FILE_OPEN_INSIDE:
      g_App.OpenItemInside();
      break;
    case IDM_FILE_OPEN_OUTSIDE:
      g_App.OpenItemOutside();
      break;
    case IDM_FILE_VIEW:
      break;
    case IDM_FILE_EDIT:
      g_App.EditItem();
      break;
    case IDM_RENAME:
      g_App.Rename();
      break;
    case IDM_COPY_TO:
      g_App.CopyTo();
      break;
    case IDM_MOVE_TO:
      g_App.MoveTo();
      break;
    case IDM_DELETE:
      g_App.Delete();
      break;

    case IDM_CREATE_FOLDER:
      g_App.CreateFolder();
      break;
    case IDM_CREATE_FILE:
      g_App.CreateFile();
      break;
    default:
      return false;
  } 
  return true;
}

bool OnMenuCommand(HWND hWnd, int id)
{
  if (ExecuteFileCommand(id))
    return true;

  switch (id)
  {	
    // File
    case IDCLOSE:
      SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
      SendMessage (hWnd, WM_CLOSE, 0, 0);
      break;
    
    // Edit
    case IDM_SELECT_ALL:
      g_App.SelectAll(true);
      break;
    case IDM_DESELECT_ALL:
      g_App.SelectAll(false);
      break;
    case IDM_INVERT_SELECTION:
      g_App.InvertSelection();
      break;
    case IDM_SELECT:
      g_App.SelectSpec(true);
      break;
    case IDM_DESELECT:
      g_App.SelectSpec(false);
      break;
    case IDM_SELECT_BY_TYPE:
      g_App.SelectByType(true);
      break;
    case IDM_DESELECT_BY_TYPE:
      g_App.SelectByType(false);
      break;

    //View
    case IDM_VIEW_LARGE_ICONS:
    case IDM_VIEW_SMALL_ICONS:
    case IDM_VIEW_LIST:
    case IDM_VIEW_DETAILS:
    {
      UINT index = id - IDM_VIEW_LARGE_ICONS;
      if (index < 4)
      {
        g_App.SetListViewMode(index);
        /*
        CMenu menu;
        menu.Attach(::GetSubMenu(::GetMenu(hWnd), kViewMenuIndex));
        menu.CheckRadioItem(IDM_VIEW_LARGE_ICONS, IDM_VIEW_DETAILS, 
            id, MF_BYCOMMAND);
        */
      }
      break;
    }
    case IDM_VIEW_ARANGE_BY_NAME:
    {
      g_App.SortItemsWithPropID(kpidName);
      break;
    }
    case IDM_VIEW_ARANGE_BY_TYPE:
    {
      g_App.SortItemsWithPropID(kpidExtension);
      break;
    }
    case IDM_VIEW_ARANGE_BY_DATE:
    {
      g_App.SortItemsWithPropID(kpidLastWriteTime);
      break;
    }
    case IDM_VIEW_ARANGE_BY_SIZE:
    {
      g_App.SortItemsWithPropID(kpidSize);
      break;
    }
    case IDM_VIEW_ARANGE_NO_SORT:
    {
      g_App.SortItemsWithPropID(kpidNoProperty);
      break;
    }

    case IDM_OPEN_ROOT_FOLDER:
      g_App.OpenRootFolder();
      break;
    case IDM_OPEN_PARENT_FOLDER:
      g_App.OpenParentFolder();
      break;
    case IDM_FOLDERS_HISTORY:
      g_App.FoldersHistory();
      break;
    case IDM_VIEW_REFRESH:
      g_App.RefreshView();
      break;

    // Tools
    case IDM_OPTIONS:
      OptionsDialog(hWnd, g_hInstance);
      break;
          
    // Help
    case IDM_HELP_CONTENTS:
      ShowHelpWindow(NULL, kFMHelpTopic);
      break;
    case IDM_ABOUT:
    {
      CAboutDialog dialog;
      dialog.Create(hWnd);
      break;
    }
    default:
      return false;
  }
  return true;
}

