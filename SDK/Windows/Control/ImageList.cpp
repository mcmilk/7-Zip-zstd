// Windows/Control/ImageList.cpp

#include "StdAfx.h"

#include "Windows/Control/ImageList.h"

namespace NWindows {
namespace NControl {

bool CImageList::Attach(HIMAGELIST anImageList)
{
  if(anImageList == NULL)
    return false;
  m_Object = anImageList;
  return true;
}

HIMAGELIST CImageList::Detach()
{
  HIMAGELIST anImageList = m_Object;
  m_Object = NULL;
  return anImageList;
}

bool CImageList::Create(int aWidth, int aHeight, UINT aFlags, 
    int anInitialNumber, int aGrow)
{
  HIMAGELIST anObject = ImageList_Create(aWidth, aHeight, aFlags, 
      anInitialNumber, aGrow);
  if(anObject == NULL)
    return false;
  return Attach(anObject);
}

bool CImageList::Destroy() // DeleteImageList() in MFC
{
  if (m_Object == NULL)
    return false;
  return BOOLToBool(ImageList_Destroy(Detach()));
}

}}

