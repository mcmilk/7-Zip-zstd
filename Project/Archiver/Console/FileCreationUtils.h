// FileCreationUtils.h

#pragma once 

#ifndef __FILECREATIONUTILS_H
#define __FILECREATIONUTILS_H

#include "Common/String.h"

class CFileVectorBundle
{
  CSysStringVector m_FileNames;
public:
  ~CFileVectorBundle();
  bool Add(const CSysString &aFilePath, bool aTryToOpen = true);
  void DisableDeleting(int anIndex);
  void Clear();
};

#endif
