// ArchiverInfo.h

#pragma once

#ifndef __ARCHIVERINFO_H
#define __ARCHIVERINFO_H

#include "Common/String.h"

struct CArchiverExtInfo
{
  UString Extension;
  UString AddExtension;
  CArchiverExtInfo() {}
  CArchiverExtInfo(const UString &extension):
    Extension(extension) {}
  CArchiverExtInfo(const UString &extension, const UString &addExtension):
    Extension(extension), AddExtension(addExtension) {}
};

struct CArchiverInfo
{
  #ifndef EXCLUDE_COM
  CSysString FilePath;
  CLSID ClassID;
  #endif
  UString Name;
  CObjectVector<CArchiverExtInfo> Extensions;
  int FindExtension(const UString &ext) const
  {
    for (int i = 0; i < Extensions.Size(); i++)
      if (ext.CollateNoCase(Extensions[i].Extension) == 0)
        return i;
    return -1;
  }
  UString GetAllExtensions() const
  {
    UString s;
    for (int i = 0; i < Extensions.Size(); i++)
    {
      if (i > 0)
        s += ' ';
      s += Extensions[i].Extension;
    }
    return s;
  }
  const UString &GetMainExtension() const 
  { 
    return Extensions[0].Extension;
  }
  bool UpdateEnabled;
  bool KeepName;
};

void ReadArchiverInfoList(CObjectVector<CArchiverInfo> &archivers);

#endif