// MyIDList.h

#pragma once

#ifndef __MYIDLIST_H
#define __MYIDLIST_H

#include "../Common/IArchiveHandler2.h"
#include "Common/String.h"
#include "Windows/ItemIDListUtils.h"

const UINT64 kZipViewSignature = 0xF8375A4950273764ui64;
struct CPropertySignature
{
  UINT64 ProgramSignature;
  // UINT64 FileSignature;
};

UINT32 GetIndexFromIDList(LPCITEMIDLIST anIDList);
void CreateIDListFromIndex(CShellItemIDList &aShellItemIDList, UINT32 anIndex);

UString GetNameOfObject(IArchiveFolder * anArchiveFolder, UINT32 anIndex);
UString GetNameOfObject(IArchiveFolder * anArchiveFolder, LPCITEMIDLIST anIDList);

UString GetExtensionOfObject(IArchiveFolder * anArchiveFolder, UINT32 anIndex);


bool IsObjectFolder(IArchiveFolder * anArchiveFolder, UINT32 anIndex);
bool IsObjectFolder(IArchiveFolder * anArchiveFolder, LPCITEMIDLIST anIDList);


#endif