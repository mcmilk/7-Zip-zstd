// ItemIDListUtils.h

#pragma once

#ifndef __ITEMIDLISTUTILS_H
#define __ITEMIDLISTUTILS_H

#include "Common/Types.h"

/////////////////////////////
// It is not for shell using
// It's for internal using only
// since it uses CoTaskMemFree instead SHGetMalloc

namespace NItemIDList {

LPITEMIDLIST GetNextItem(LPCITEMIDLIST anIDList);
UINT32 GetSize(LPCITEMIDLIST anIDList);
LPITEMIDLIST  GetLastItem(LPCITEMIDLIST anIDList);

class CHolder
{
  LPITEMIDLIST m_Object;
public:
  CHolder(): m_Object(NULL) {}
  CHolder(const CHolder& anItemIDList);
  ~CHolder() { Free(); }
  void Attach(LPITEMIDLIST anObject)
  {
    if (m_Object != NULL)
      Free();
    m_Object = anObject;
  }
  LPITEMIDLIST Detach()
  {
    LPITEMIDLIST anObject = m_Object;
    m_Object = NULL;
    return anObject;
  }
  void Free(); 
  bool Create(UINT16 aPureSize);
  operator LPITEMIDLIST() { return m_Object;}
  operator LPCITEMIDLIST() const { return m_Object;}
  LPITEMIDLIST* operator&() { return &m_Object; }
  LPITEMIDLIST operator->() { return m_Object; }

  CHolder& operator=(LPCITEMIDLIST anObject);
  CHolder& operator=(const CHolder &anObject);
};

}

class CItemIDListManager
{
public:
  CItemIDListManager();
  ~CItemIDListManager();
public:
  LPITEMIDLIST Create(UINT16 aPureSize);
  void Delete(LPITEMIDLIST anIDList);
  LPITEMIDLIST  Copy(LPCITEMIDLIST anIDListSrc);
  // CZipIDData* GetDataPointer(LPCITEMIDLIST anIDList);
  LPITEMIDLIST  Concatenate(LPCITEMIDLIST anIDList1, LPCITEMIDLIST anIDList2);
private:
  LPMALLOC m_pMalloc;
};

class CShellItemIDList
{
  CItemIDListManager *m_Manager;
  LPITEMIDLIST m_Object;
public:
  CShellItemIDList(CItemIDListManager *aManager): 
     m_Manager(aManager), 
     m_Object(NULL) {}
  bool Create(UINT16 aPureSize)
  {   
    Free();
    m_Object = m_Manager->Create(aPureSize);
    return (m_Object != NULL); 
  }
  ~CShellItemIDList() { Free(); }
  void Free() 
  { 
    if(m_Object != NULL)
      m_Manager->Delete(m_Object);
    m_Object = NULL;
  }
  void Attach(LPITEMIDLIST anObject)
  {
    Free();
    m_Object = anObject;
  }
  operator LPITEMIDLIST() { return m_Object;}
  LPITEMIDLIST* operator&() { return &m_Object; }
};

#endif