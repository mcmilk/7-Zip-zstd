// FileCreationUtils.h

#pragma once 

#ifndef __FILECREATIONUTILS_H
#define __FILECREATIONUTILS_H

#include "Common/String.h"

class CFileVectorBundle
{
  UStringVector m_FileNames;
public:
  ~CFileVectorBundle() { Clear(); }
  bool Add(const UString &filePath, bool tryToOpen = true);
  void DisableDeleting(int index);
  void Clear();
};

#endif
