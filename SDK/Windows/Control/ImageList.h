// Windows/Control/ImageList.h

#pragma once

#ifndef __WINDOWS_CONTROL_IMAGELIST_H
#define __WINDOWS_CONTROL_IMAGELIST_H

#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CImageList
{
  HIMAGELIST m_Object;
public:
  operator HIMAGELIST() const {return m_Object; }
  CImageList(): m_Object(NULL) {}
  bool Attach(HIMAGELIST anImageList);
  HIMAGELIST Detach();

  bool Create(int aWidth, int aHeight, UINT aFlags, int anInitialNumber, int aGrow);
  bool Destroy(); // DeleteImageList() in MFC

  ~CImageList()
    { Destroy(); }

  int GetImageCount() const
    { return ImageList_GetImageCount(m_Object); }

  bool GetImageInfo(int anIndex, IMAGEINFO* anImageInfo) const
    { return BOOLToBool(ImageList_GetImageInfo(m_Object, anIndex, anImageInfo)); }

  int Add(HICON anIcon)
    { return ImageList_AddIcon(m_Object, anIcon); }
  int Replace(int anIndex, HICON anIcon)
    { return ImageList_ReplaceIcon(m_Object, anIndex, anIcon); }

  // If anIndex is -1, the function removes all images.
  bool Remove(int anIndex)
    { return BOOLToBool(ImageList_Remove(m_Object, anIndex)); }
  bool RemoveAll()
    { return BOOLToBool(ImageList_RemoveAll(m_Object)); }

  HICON ExtractIcon(int anIndex)
    { return ImageList_ExtractIcon(NULL, m_Object, anIndex); }
  HICON GetIcon(int anIndex, UINT aFlags)
    { return ImageList_GetIcon(m_Object, anIndex, aFlags); }

  bool GetIconSize(int &aWidth, int &aHeight) const
    { return BOOLToBool(ImageList_GetIconSize(m_Object, &aWidth, &aHeight)); }
  bool SetIconSize(int aWidth, int aHeight)
    { return BOOLToBool(ImageList_SetIconSize(m_Object, aWidth, aHeight)); }
};

}}

#endif
 