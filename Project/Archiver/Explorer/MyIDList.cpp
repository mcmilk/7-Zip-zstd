// MyIDList.cpp

#include "StdAfx.h"

#include "MyIDList.h"
#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

using namespace NWindows;
using namespace NCOM;

bool CheckIDList(LPCITEMIDLIST anIDList)
{
  return *(UINT64 *)(anIDList->mkid.abID) == kZipViewSignature;
}

UINT32 GetIndexFromIDList(LPCITEMIDLIST anIDList)
{
  return *(UINT32 *)(anIDList->mkid.abID + sizeof(CPropertySignature));
}

void CreateIDListFromIndex(CShellItemIDList &aShellItemIDList, UINT32 anIndex)
{
  aShellItemIDList.Create(sizeof(CPropertySignature) + sizeof(anIndex));
  memmove(LPITEMIDLIST(aShellItemIDList)->mkid.abID, &kZipViewSignature, 
      sizeof(kZipViewSignature));
  memmove(LPITEMIDLIST(aShellItemIDList)->mkid.abID + 
      sizeof(CPropertySignature), &anIndex, sizeof(anIndex));
}


UString GetNameOfObject(IArchiveFolder *anArchiveFolder, UINT32 anIndex)
{
  CPropVariant aPropVariantName;
  if (anArchiveFolder->GetProperty(anIndex, kaipidName, &aPropVariantName) != S_OK)
    throw 3;
  if(aPropVariantName.vt != VT_BSTR)
    throw 2;
  return aPropVariantName.bstrVal;
}

UString GetNameOfObject(IArchiveFolder * anArchiveFolder, LPCITEMIDLIST anIDList)
{
  return GetNameOfObject(anArchiveFolder, GetIndexFromIDList(anIDList));
}

UString GetExtensionOfObject(IArchiveFolder *anArchiveFolder, UINT32 anIndex)
{
  UString aName = GetNameOfObject(anArchiveFolder, anIndex);
  int aDotIndex = aName.ReverseFind(L'.');
  if(aDotIndex < 0)
    return UString();
  return aName.Mid(aDotIndex);
}

bool IsObjectFolder(IArchiveFolder * anArchiveFolder, UINT32 anIndex)
{
  CPropVariant aPropertyIsFolder;
  if (anArchiveFolder->GetProperty(anIndex, kaipidIsFolder, &aPropertyIsFolder) != S_OK)
    throw 3;
  if(aPropertyIsFolder.vt != VT_BOOL)
    throw 2;
  return VARIANT_BOOLToBool(aPropertyIsFolder.boolVal);
}

bool IsObjectFolder(IArchiveFolder * anArchiveFolder, LPCITEMIDLIST anIDList)
{
  return IsObjectFolder(anArchiveFolder, GetIndexFromIDList(anIDList));
}


