// BrowseDialog.cpp
 
#include "StdAfx.h"

#ifdef UNDER_CE

#include "Common/IntToString.h"

#include "Windows/PropVariantConversions.h"

#include "BrowseDialog.h"
#include "LangUtils.h"
#include "PropertyNameRes.h"

#ifndef _SFX
#include "RegistryUtils.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NFind;

extern bool g_LVN_ITEMACTIVATE_Support;

static const int kParentIndex = -1;

#ifdef LANG
static CIDLangPair kIDLangPairs[] =
{
  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 }
};
#endif

static bool GetParentPath(const UString &path2, UString &dest, UString &focused)
{
  UString path = path2;
  dest.Empty();
  if (path.IsEmpty())
    return false;
  if (path.Back() == WCHAR_PATH_SEPARATOR)
    path.DeleteBack();
  if (path.IsEmpty())
    return false;
  int pos = path.ReverseFind(WCHAR_PATH_SEPARATOR);
  if (pos < 0 || path.Back() == WCHAR_PATH_SEPARATOR)
    return false;
  focused = path.Mid(pos + 1);
  dest = path.Left(pos + 1);
  return true;
}

bool CBrowseDialog::OnInit()
{
  #ifdef LANG
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  if (!Title.IsEmpty())
    SetText(Title);
  _list.Attach(GetItem(IDC_BROWSE_LIST));

  #ifndef UNDER_CE
  _list.SetUnicodeFormat(true);
  #endif

  #ifndef _SFX
  if (ReadSingleClick())
    _list.SetExtendedListViewStyle(LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT);
  _showDots = ReadShowDots();
  #endif

  _list.SetImageList(GetSysImageList(true), LVSIL_SMALL);
  _list.SetImageList(GetSysImageList(false), LVSIL_NORMAL);

  _list.InsertColumn(0, LangStringSpec(IDS_PROP_NAME, 0x02000204), 100);
  _list.InsertColumn(1, LangStringSpec(IDS_PROP_MTIME, 0x0200020C), 100);
  {
    LV_COLUMNW column;
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    column.fmt = LVCFMT_RIGHT;
    column.iSubItem = 2;
    UString s = LangStringSpec(IDS_PROP_SIZE, 0x02000207);
    column.pszText = (wchar_t *)(const wchar_t *)s;
    _list.InsertColumn(2, &column);

    // _list.InsertColumn(2, LangStringSpec(IDS_PROP_SIZE, 0x02000207), 100);
  }

  _list.InsertItem(0, L"12345678901234567");
  _list.SetSubItem(0, 1, L"2009-09-09");
  _list.SetSubItem(0, 2, L"9999 MB");
  for (int i = 0; i < 3; i++)
    _list.SetColumnWidthAuto(i);
  _list.DeleteAllItems();
  
  UString selectedName;
  if (!FolderMode)
  {
    int pos = Path.ReverseFind(WCHAR_PATH_SEPARATOR);
    if (pos >= 0 && Path.Back() != WCHAR_PATH_SEPARATOR)
    {
      selectedName = Path.Mid(pos + 1);
      Path = Path.Left(pos + 1);
    }
  }
  _ascending = true;
  _sortIndex = 0;

  NormalizeSize();

  while (Reload(Path, selectedName) != S_OK)
  {
    UString parent;
    if (!GetParentPath(Path, parent, selectedName))
      break;
    selectedName.Empty();
    Path = parent;
  }

  return CModalDialog::OnInit();
}

bool CBrowseDialog::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  int mx, my;
  {
    RECT rect;
    GetClientRectOfItem(IDC_BROWSE_PARENT, rect);
    mx = rect.left;
    my = rect.top;
  }
  InvalidateRect(NULL);

  {
    RECT rect;
    GetClientRectOfItem(IDC_BROWSE_PATH, rect);
    MoveItem(IDC_BROWSE_PATH, rect.left, rect.top, xSize - mx - rect.left, RECT_SIZE_Y(rect));
  }

  int bx1, bx2, by;
  GetItemSizes(IDCANCEL, bx1, by);
  GetItemSizes(IDOK, bx2, by);
  int y = ySize - my - by;
  int x = xSize - mx - bx1;
  MoveItem(IDCANCEL, x, y, bx1, by);
  MoveItem(IDOK, x - mx - bx2, y, bx2, by);

  {
    RECT rect;
    GetClientRectOfItem(IDC_BROWSE_LIST, rect);
    _list.Move(rect.left, rect.top, xSize - mx - rect.left, y - my - rect.top);
  }
  return false;
}

static UString ConvertSizeToStringShort(UInt64 value)
{
  wchar_t s[32];
  wchar_t c = L'\0', c2 = L'\0';
  if (value < (UInt64)10000)
  {
    c = L'\0';
    c2 = L'\0';
  }
  else if (value < ((UInt64)10000 << 10))
  {
    value >>= 10;
    c = L'K';
  }
  else if (value < ((UInt64)10000 << 20))
  {
    value >>= 20;
    c = L'M';
  }
  else
  {
    value >>= 30;
    c = L'G';
  }
  ConvertUInt64ToString(value, s);
  int p = MyStringLen(s);
  if (c != 0)
    s[p++] = L' ';
  s[p++] = c;
  s[p++] = c2;
  s[p++] = L'\0';
  return s;
}

int CBrowseDialog::CompareItems(LPARAM lParam1, LPARAM lParam2)
{
  if (lParam1 == kParentIndex) return -1;
  if (lParam2 == kParentIndex) return 1;
  const CFileInfoW &f1 = _files[(int)lParam1];
  const CFileInfoW &f2 = _files[(int)lParam2];

  bool isDir1 = f1.IsDir();
  bool isDir2 = f2.IsDir();
  
  if (isDir1 && !isDir2) return -1;
  if (isDir2 && !isDir1) return 1;
  
  int result = 0;
  switch(_sortIndex)
  {
    case 0: result = f1.Name.CompareNoCase(f2.Name); break;
    case 1: result = CompareFileTime(&f1.MTime, &f2.MTime); break;
    case 2: result = MyCompare(f1.Size, f2.Size); break;
  }
  return _ascending ? result: (-result);
}

static int CALLBACK CompareItems2(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
  if (lpData == NULL)
    return 0;
  return ((CBrowseDialog*)lpData)->CompareItems(lParam1, lParam2);
}

static HRESULT GetNormalizedError()
{
  HRESULT errorCode = GetLastError();
  return (errorCode == 0) ? 1 : errorCode;
}

HRESULT CBrowseDialog::Reload(const UString &pathPrefix, const UString &selectedName)
{
  CEnumeratorW enumerator(pathPrefix + L'*');
  CObjectVector<CFileInfoW> files;
  for (;;)
  {
    bool found;
    CFileInfoW fi;
    if (!enumerator.Next(fi, found))
      return GetNormalizedError();
    if (!found)
      break;
    files.Add(fi);
  }

  Path = pathPrefix;

  _files = files;

  SetItemText(IDC_BROWSE_PATH, Path);
  _list.SetRedraw(false);
  _list.DeleteAllItems();

  if (!Path.IsEmpty() && Path.Back() != WCHAR_PATH_SEPARATOR)
    Path += WCHAR_PATH_SEPARATOR;

  LVITEMW item;

  int index = 0;
  int cursorIndex = -1;

  #ifndef _SFX
  if (_showDots)
  {
    UString itemName = L"..";
    item.iItem = index;
    if (selectedName.IsEmpty())
      cursorIndex = item.iItem;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    int subItem = 0;
    item.iSubItem = subItem++;
    item.lParam = kParentIndex;
    item.pszText = (wchar_t *)(const wchar_t *)itemName;
    item.iImage = _extToIconMap.GetIconIndex(FILE_ATTRIBUTE_DIRECTORY, Path);
    if (item.iImage < 0)
      item.iImage = 0;
    _list.InsertItem(&item);
    _list.SetSubItem(index, subItem++, L"");
    _list.SetSubItem(index, subItem++, L"");
    index++;
  }
  #endif

  for (int i = 0; i < _files.Size(); i++)
  {
    const CFileInfoW &fi = _files[i];
    item.iItem = index;
    if (fi.Name.CompareNoCase(selectedName) == 0)
      cursorIndex = item.iItem;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    int subItem = 0;
    item.iSubItem = subItem++;
    item.lParam = i;
    item.pszText = (wchar_t *)(const wchar_t *)fi.Name;
    item.iImage = _extToIconMap.GetIconIndex(fi.Attrib, Path + fi.Name);
    if (item.iImage < 0)
      item.iImage = 0;
    _list.InsertItem(&item);
    {
      FILETIME ft;
      UString s;
      if (FileTimeToLocalFileTime(&fi.MTime, &ft))
        s = ConvertFileTimeToString(ft, false, false);
      _list.SetSubItem(index, subItem++, s);
    }
    {
      UString s;
      if (!fi.IsDir())
        s = ConvertSizeToStringShort(fi.Size);
      _list.SetSubItem(index, subItem++, s);
    }
    index++;
  }

  if (_list.GetItemCount() > 0 && cursorIndex >= 0)
    _list.SetItemState_FocusedSelected(cursorIndex);
  _list.SortItems(CompareItems2, (LPARAM)this);
  if (_list.GetItemCount() > 0 && cursorIndex < 0)
    _list.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);
  _list.EnsureVisible(_list.GetFocusedItem(), false);
  _list.SetRedraw(true);
  return S_OK;
}

HRESULT CBrowseDialog::Reload()
{
  UString selectedCur;
  int index = _list.GetNextSelectedItem(-1);
  if (index >= 0)
  {
    int fileIndex = GetRealItemIndex(index);
    if (fileIndex != kParentIndex)
      selectedCur = _files[fileIndex].Name;
  }
  return Reload(Path, selectedCur);
}

void CBrowseDialog::OpenParentFolder()
{
  UString parent, selected;
  if (GetParentPath(Path, parent, selected))
    Reload(parent, selected);
}

extern UString HResultToMessage(HRESULT errorCode);

bool CBrowseDialog::OnNotify(UINT /* controlID */, LPNMHDR header)
{
  if (header->hwndFrom != _list)
    return false;
  switch(header->code)
  {
    case LVN_ITEMACTIVATE:
      if (g_LVN_ITEMACTIVATE_Support)
      {
        OnItemEnter();
        return true;
      }
      break;
    case NM_DBLCLK:
    case NM_RETURN: // probabably it's unused
      if (!g_LVN_ITEMACTIVATE_Support)
      {
        OnItemEnter();
        return true;
      }
      break;
    case LVN_COLUMNCLICK:
    {
      int index = LPNMLISTVIEW(header)->iSubItem;
      if (index == _sortIndex)
        _ascending = !_ascending;
      else
      {
        _ascending = (index == 0);
        _sortIndex = index;
      }
      Reload();
      return false;
    }
    case LVN_KEYDOWN:
    {
      LRESULT result;
      bool boolResult = OnKeyDown(LPNMLVKEYDOWN(header), result);
      return boolResult;
    }
  }
  return false;
}

bool CBrowseDialog::OnKeyDown(LPNMLVKEYDOWN keyDownInfo, LRESULT &result)
{
  bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  result = 0;

  switch(keyDownInfo->wVKey)
  {
    case VK_BACK:
      OpenParentFolder();
      return true;
    case 'R':
      if (ctrl)
      {
        Reload();
        return true;
      }
      return false;
  }
  return false;
}

bool CBrowseDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_BROWSE_PARENT:
      OpenParentFolder();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CBrowseDialog::ShowError(LPCWSTR s) { MessageBoxW(*this, s, L"7-Zip", MB_ICONERROR); }

void CBrowseDialog::ShowSelectError()
{
  ShowError(FolderMode ?
      L"You must select some folder":
      L"You must select some file");
}

void CBrowseDialog::FinishOnOK()
{
  int index = _list.GetNextSelectedItem(-1);
  if (index < 0)
  {
    if (!FolderMode)
    {
      ShowSelectError();
      return;
    }
  }
  else
  {
    int fileIndex = GetRealItemIndex(index);
    if (fileIndex == kParentIndex)
    {
      OpenParentFolder();
      return;
    }
    const CFileInfoW &file = _files[fileIndex];
    if (file.IsDir() != FolderMode)
    {
      ShowSelectError();
      return;
    }
    Path += file.Name;
  }
  End(IDOK);
}

void CBrowseDialog::OnItemEnter()
{
  int index = _list.GetNextSelectedItem(-1);
  if (index < 0)
    return;
  int fileIndex = GetRealItemIndex(index);
  if (fileIndex == kParentIndex)
    OpenParentFolder();
  else
  {
    const CFileInfoW &file = _files[fileIndex];
    if (!file.IsDir())
    {
      if (!FolderMode)
        FinishOnOK();
      else
        ShowSelectError();
      return;
    }
    HRESULT res = Reload(Path + file.Name + WCHAR_PATH_SEPARATOR, L"");
    if (res != S_OK)
      ShowError(HResultToMessage(res));
  }
}

void CBrowseDialog::OnOK()
{
  if (GetFocus() == _list)
  {
    OnItemEnter();
    return;
  }
  FinishOnOK();
}

static bool MyBrowse(HWND owner, LPCWSTR title, LPCWSTR initialFolder, UString &resultPath, bool folderMode)
{
  CBrowseDialog dialog;
  dialog.Title = title;
  dialog.Path = initialFolder;
  dialog.FolderMode = folderMode;
  if (dialog.Create(owner) != IDOK)
    return false;
  resultPath = dialog.Path;
  return true;
}

bool MyBrowseForFolder(HWND owner, LPCWSTR title, LPCWSTR initialFolder, UString &resultPath)
{
  return MyBrowse(owner, title, initialFolder, resultPath, true);
}

bool MyBrowseForFile(HWND owner, LPCWSTR title, LPCWSTR initialFolder, LPCWSTR, UString &resultPath)
{
  return MyBrowse(owner, title, initialFolder, resultPath, false);
}

#endif
