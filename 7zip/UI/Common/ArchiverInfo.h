// ArchiverInfo.h

#pragma once

#ifndef __ARCHIVERINFO_H
#define __ARCHIVERINFO_H

#include "Common/String.h"

struct CArchiverInfo
{
  #ifndef EXCLUDE_COM
  CSysString FilePath;
  CLSID ClassID;
  #endif
  UString Name;
  UString Extension;
  UString AddExtension;
  bool UpdateEnabled;
  bool KeepName;
};

void ReadArchiverInfoList(CObjectVector<CArchiverInfo> &archivers);

#endif