// ItemIDListUtils.h

#include "StdAfx.h"

#include "ItemIDListUtils.h"

namespace NItemIDList {

CHolder::CHolder(const CHolder& anItemIDList):
  m_Object(NULL)
{
  *this = anItemIDList;
}

bool CHolder::Create(UINT16 aPureSize)
{   
  Free();
  m_Object = LPITEMIDLIST(CoTaskMemAlloc(2 + aPureSize + 2));
  if(m_Object == NULL)
    return false;
  m_Object->mkid.cb = 2 + aPureSize;
  LPITEMIDLIST aNewIDListEnd = LPITEMIDLIST(((BYTE *)m_Object) + 2 + aPureSize);
  aNewIDListEnd->mkid.cb = 0;
  return (m_Object != NULL); 
}

void CHolder::Free() 
{ 
  if(m_Object == NULL)
    return;
  CoTaskMemFree(m_Object);
  m_Object = NULL;
}

CHolder& CHolder::operator=(LPCITEMIDLIST anObject)
{
  if(m_Object != NULL)
    Free();
  UINT32 aSize = GetSize(anObject);
  m_Object = (LPITEMIDLIST)CoTaskMemAlloc(aSize);
  if(m_Object != NULL)
    MoveMemory(m_Object, anObject, aSize);
  return *this;
}

CHolder& CHolder::operator=(const CHolder &anObject)
{
  if(m_Object != NULL)
    Free();
  if(anObject.m_Object != NULL)
  {
    UINT32 aSize = GetSize(anObject.m_Object);
    m_Object = (LPITEMIDLIST)CoTaskMemAlloc(aSize);
    if(m_Object != NULL)
      MoveMemory(m_Object, anObject.m_Object, aSize);
  }
  return *this;
}

////////////////////////
// static

LPITEMIDLIST GetNextItem(LPCITEMIDLIST anIDList)
{
  if (anIDList)
    return (LPITEMIDLIST)(LPBYTE) ( ((LPBYTE)anIDList) + anIDList->mkid.cb);
  return (NULL);
}

UINT32 GetSize(LPCITEMIDLIST anIDList)
{
  UINT32 aSizeTotal = 0;
  if(anIDList != NULL)
  {
    while(anIDList->mkid.cb != 0)
    {
      aSizeTotal += anIDList->mkid.cb;
      anIDList = GetNextItem(anIDList);
    }  
    aSizeTotal += sizeof(ITEMIDLIST) - 1;
  }
  return (aSizeTotal);
}

LPITEMIDLIST GetLastItem(LPCITEMIDLIST anIDList)
{
  LPITEMIDLIST anIDListLast = NULL;
  if(anIDList)
    while(anIDList->mkid.cb != 0)
    {
      anIDListLast = (LPITEMIDLIST)anIDList;
      anIDList = GetNextItem(anIDList);
    }  
  return anIDListLast;
}

}

////////////////////////////////////////////////////////////////
// CItemIDListManager : Class to manage pidls

CItemIDListManager::CItemIDListManager():
  m_pMalloc(NULL)
{
  SHGetMalloc(&m_pMalloc);
}

CItemIDListManager::~CItemIDListManager()
{
  if (m_pMalloc)
    m_pMalloc->Release();
}

LPITEMIDLIST CItemIDListManager::Create(UINT16 aPureSize)
{
  LPITEMIDLIST aPointer = LPITEMIDLIST(m_pMalloc->Alloc(2 + aPureSize + 2));
  if(aPointer == NULL)
    return NULL;
  aPointer->mkid.cb = 2 + aPureSize;
  LPITEMIDLIST aNewIDListEnd = LPITEMIDLIST(((BYTE *)aPointer) + 2 + aPureSize);
  aNewIDListEnd->mkid.cb = 0;
  return aPointer;
}

void CItemIDListManager::Delete(LPITEMIDLIST anIDList)
{
  m_pMalloc->Free(anIDList);
}

LPITEMIDLIST CItemIDListManager::Copy(LPCITEMIDLIST anIDListSrc)
{
  if (NULL == anIDListSrc)
    return (NULL);
  LPITEMIDLIST anIDListTarget = NULL;
  UINT32 aSize = NItemIDList::GetSize(anIDListSrc);
  anIDListTarget = (LPITEMIDLIST)m_pMalloc->Alloc(aSize);
  if (!anIDListTarget)
    return (NULL);
  MoveMemory(anIDListTarget, anIDListSrc, aSize);
  return anIDListTarget;
}

LPITEMIDLIST CItemIDListManager::Concatenate(LPCITEMIDLIST anIDList1, 
  LPCITEMIDLIST anIDList2)
{
  if(!anIDList1 && !anIDList2)
    return NULL;

  if(!anIDList1)
    return Copy(anIDList2);

  if(!anIDList2)
    return Copy(anIDList1);

  UINT32 cb1 = NItemIDList::GetSize(anIDList1) - 2;
  UINT32 cb2 = NItemIDList::GetSize(anIDList2);

  LPITEMIDLIST  anIDListNew = (LPITEMIDLIST)m_pMalloc->Alloc(cb1 + cb2);

  if(anIDListNew)
  {
    MoveMemory(anIDListNew, anIDList1, cb1);
    MoveMemory(((LPBYTE)anIDListNew) + cb1, anIDList2, cb2);
  }
  return anIDListNew;
}
