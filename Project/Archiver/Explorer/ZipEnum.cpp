// ZipEnum.cpp

#include "StdAfx.h"

#include "ZipEnum.h"
#include "ZipFolder.h"

#include "MyIDList.h"

void CZipEnumIDList::Init(DWORD uFlags, IArchiveFolder *anArchiveFolderItem)
{
  m_ArchiveFolder = anArchiveFolderItem;
  m_ArchiveFolder->GetNumberOfItems(&m_NumSubItems);
  m_Flags = uFlags;
}

STDMETHODIMP CZipEnumIDList::Reset()
{
  m_Index = 0;
  return S_OK;
}

bool CZipEnumIDList::TestFileItem(/*const CArchiveFolderFileItem &anItem*/)
{
  return (m_Flags & SHCONTF_NONFOLDERS) != 0;
}

bool CZipEnumIDList::TestDirItem(/*const CArchiveFolderItem &anItem*/)
{
  return (m_Flags & SHCONTF_FOLDERS) != 0;
}

STDMETHODIMP CZipEnumIDList::Next(ULONG aNumItems, LPITEMIDLIST *anItems,
	  ULONG *aNumFetched)
{
  HRESULT aResult = S_OK;
  if(aNumItems > 1 && !aNumFetched)
    return E_INVALIDARG;

  for(DWORD anIndex = 0; anIndex < aNumItems; m_Index++)
  {
    if(m_Index >= m_NumSubItems)
    {
      aResult =  S_FALSE;
      break;
    }
    if (IsObjectFolder(m_ArchiveFolder, m_Index))
    {
      if(!TestDirItem())
        continue;
    }
    else
    {
      if(!TestFileItem())
        continue;
    }
    CShellItemIDList aShellItemIDList(&m_IDListManager);
    CreateIDListFromIndex(aShellItemIDList, m_Index);
    anItems[anIndex] = m_IDListManager.Copy(aShellItemIDList);
    anIndex++;
  }
  if (aNumFetched)
    *aNumFetched = anIndex;
  return aResult;
}

STDMETHODIMP CZipEnumIDList::Skip(ULONG aNumSkip)
{
  for(DWORD i = 0; i < aNumSkip; m_Index++)
  {
    if(m_Index >= m_NumSubItems)
      return S_FALSE;
    if (IsObjectFolder(m_ArchiveFolder, m_Index))
    {
      if(!TestDirItem())
        continue;
    }
    else
    {
      if(!TestFileItem())
        continue;
    }
    i++;
  }
  return S_OK;
}

STDMETHODIMP CZipEnumIDList::Clone( IEnumIDList **ppenum )
{
	return E_NOTIMPL;
}
