// ViewSettings.h

#pragma once

#ifndef __VIEWSETTINGS_H
#define __VIEWSETTINGS_H

#include "Common/Vector.h"
#include "Common/String.h"

struct CColumnInfo
{
  PROPID PropID;
  bool IsVisible;
  UINT32 Width;
};

inline bool operator==(const CColumnInfo &a1, const CColumnInfo &a2)
{ 
  return (a1.PropID == a2.PropID) && 
    (a1.IsVisible == a2.IsVisible) && (a1.Width == a2.Width); 
}

inline bool operator!=(const CColumnInfo &a1, const CColumnInfo &a2)
{ 
  return !(a1 == a2);
}


struct CListViewInfo
{
  CObjectVector<CColumnInfo> Columns;
  // int SortIndex;
  PROPID SortID;
  bool Ascending;

  void Clear()
  {
    // SortIndex = -1;
    SortID = 0;
    Ascending = true;
    Columns.Clear();
  }

  int FindColumnWithID(PROPID propID) const
  {
    for (int i = 0; i < Columns.Size(); i++)
      if (Columns[i].PropID == propID)
        return i;
    return -1;
  }

  bool IsEqual(const CListViewInfo &aNewInfo) const 
  {
    if (Columns.Size() != aNewInfo.Columns.Size() ||
      // SortIndex != aNewInfo.SortIndex ||  
      SortID != aNewInfo.SortID ||  
      Ascending != aNewInfo.Ascending)
      return false;
    for (int i = 0; i < Columns.Size(); i++)
      if (Columns[i] != aNewInfo.Columns[i])
        return false;
    return true;
  }
  // void OrderItems();
};

void SaveListViewInfo(const CSysString &anID, const CListViewInfo &viewInfo);
void ReadListViewInfo(const CSysString &anID, CListViewInfo &viewInfo);

void SaveWindowSize(const RECT &rect, bool maximized);
bool ReadWindowSize(RECT &rect, bool &maximized);

void SavePanelsInfo(UINT32 numPanels, UINT32 currentPanel, UINT32 splitterPos);
bool ReadPanelsInfo(UINT32 &numPanels, UINT32 &currentPanel, UINT32 &splitterPos);

void SavePanelPath(UINT32 panel, const CSysString &path);
bool ReadPanelPath(UINT32 panel, CSysString &path);

void SaveFolderHistory(const UStringVector &folders);
void ReadFolderHistory(UStringVector &folders);

void SaveFastFolders(const UStringVector &folders);
void ReadFastFolders(UStringVector &folders);

void SaveCopyHistory(const UStringVector &folders);
void ReadCopyHistory(UStringVector &folders);

void AddUniqueStringToHeadOfList(UStringVector &list, 
    const UString &string);

#endif
