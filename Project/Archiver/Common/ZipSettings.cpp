// ZipSettings.h

#include "StdAfx.h"

#include "ZipSettings.h"

namespace NZipSettings {

////////////////////////////////////////////////////////////
// CListViewInfo

int CListViewInfo::FindColumnWithID(PROPID anID) const
{
  for(int i = 0; i < ColumnInfoVector.size(); i++)
    if(ColumnInfoVector[i].PropID == anID)
      return i;
  return -1;
}

void CListViewInfo::OrderItems()
{
  CColumnInfoVector aVisibleVector;
  CColumnInfoVector anInvisibleVector;
  for(int i = 0; i < ColumnInfoVector.size(); i++)
  {
    const CColumnInfo &aColumnInfo = ColumnInfoVector[i];
    if(aColumnInfo.IsVisible)
      aVisibleVector.push_back(aColumnInfo);
    else
      anInvisibleVector.push_back(aColumnInfo);
  }
  ColumnInfoVector.clear();
  for(i = 0; i < aVisibleVector.size(); i++)
    ColumnInfoVector.push_back(aVisibleVector[i]);
  for(i = 0; i < anInvisibleVector.size(); i++)
    ColumnInfoVector.push_back(anInvisibleVector[i]);
}


}