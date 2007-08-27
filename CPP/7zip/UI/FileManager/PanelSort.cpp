// PanelSort.cpp

#include "StdAfx.h"

#include "Windows/PropVariant.h"

#include "../../PropID.h"

#include "Panel.h"

using namespace NWindows;

static UString GetExtension(const UString &name)
{
  int dotPos = name.ReverseFind(L'.');
  if (dotPos < 0)
    return UString();
  return name.Mid(dotPos);
}

int CALLBACK CompareItems2(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
  if(lpData == NULL)
    return 0;
  CPanel *panel = (CPanel*)lpData;
  
  switch(panel->_sortID)
  {
    // if (panel->_sortIndex == 0)
    case kpidName:
    {
      const UString name1 = panel->GetItemName((int)lParam1);
      const UString name2 = panel->GetItemName((int)lParam2);
      int res = name1.CompareNoCase(name2);
      /*
      if (res != 0 || !panel->_flatMode)
        return res;
      const UString prefix1 = panel->GetItemPrefix(lParam1);
      const UString prefix2 = panel->GetItemPrefix(lParam2);
      return res = prefix1.CompareNoCase(prefix2);
      */
      return res;
    }
    case kpidNoProperty:
    {
      return MyCompare(lParam1, lParam2);
    }
    case kpidExtension:
    {
      const UString ext1 = GetExtension(panel->GetItemName((int)lParam1));
      const UString ext2 = GetExtension(panel->GetItemName((int)lParam2));
      return ext1.CompareNoCase(ext2);
    }
  }
  /*
  if (panel->_sortIndex == 1)
    return MyCompare(file1.Size, file2.Size);
  return ::CompareFileTime(&file1.LastWriteTime, &file2.LastWriteTime);
  */

  // PROPID propID = panel->_properties[panel->_sortIndex].ID;
  PROPID propID = panel->_sortID;

  NCOM::CPropVariant propVariant1, propVariant2;
  // Name must be first property
  panel->_folder->GetProperty((UINT32)lParam1, propID, &propVariant1);
  panel->_folder->GetProperty((UINT32)lParam2, propID, &propVariant2);
  if(propVariant1.vt != propVariant2.vt)
    return 0; // It means some BUG
  if (propVariant1.vt == VT_BSTR)
  {
    return _wcsicmp(propVariant1.bstrVal, propVariant2.bstrVal);
  }
  return propVariant1.Compare(propVariant2);
  // return 0;
}

int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
  if(lpData == NULL)
    return 0;
  if (lParam1 == kParentIndex)
    return -1;
  if (lParam2 == kParentIndex)
    return 1;

  CPanel *panel = (CPanel*)lpData;

  bool isDirectory1 = panel->IsItemFolder((int)lParam1);
  bool isDirectory2 = panel->IsItemFolder((int)lParam2);
  
  if(isDirectory1 && (!isDirectory2))
    return -1;
  if((!isDirectory1) && isDirectory2)
    return 1;

  int result = CompareItems2(lParam1, lParam2, lpData);
  return panel->_ascending ? result: (-result);
}


/*
void CPanel::SortItems(int index)
{
  if(index == _sortIndex)
    _ascending = !_ascending;
  else
  {
    _sortIndex = index;
    _ascending = true;
    switch (_properties[_sortIndex].ID)
    {
      case kpidSize:
      case kpidPackedSize:
      case kpidCreationTime:
      case kpidLastAccessTime:
      case kpidLastWriteTime:
      _ascending = false;
      break;
    }
  }
  _listView.SortItems(CompareItems, (LPARAM)this);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
}
void CPanel::SortItemsWithPropID(PROPID propID)
{
  int index = _properties.FindItemWithID(propID);
  if (index >= 0)
    SortItems(index);
}
*/
void CPanel::SortItemsWithPropID(PROPID propID)
{
  if(propID == _sortID)
    _ascending = !_ascending;
  else
  {
    _sortID = propID;
    _ascending = true;
    switch (propID)
    {
      case kpidSize:
      case kpidPackedSize:
      case kpidCreationTime:
      case kpidLastAccessTime:
      case kpidLastWriteTime:
      _ascending = false;
      break;
    }
  }
  _listView.SortItems(CompareItems, (LPARAM)this);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
}


void CPanel::OnColumnClick(LPNMLISTVIEW info)
{
  /*
  int index = _properties.FindItemWithID(_visibleProperties[info->iSubItem].ID);
  SortItems(index);
  */
  SortItemsWithPropID(_visibleProperties[info->iSubItem].ID);
}
