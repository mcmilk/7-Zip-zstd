// Windows/Shell.h

#pragma once

#ifndef __WINDOWS_SHELL_H
#define __WINDOWS_SHELL_H

#include "Common/String.h"
#include "Windows/Defs.h"

namespace NWindows{
namespace NShell{

/////////////////////////
// CItemIDList

class CItemIDList
{
  LPITEMIDLIST m_Object;
public:
  CItemIDList(): m_Object(NULL) {}
  /*
  CItemIDList(LPCITEMIDLIST anItemIDList);
  CItemIDList(const CItemIDList& anItemIDList);
  */
  ~CItemIDList() { Free(); }
  void Free();
  void Attach(LPITEMIDLIST anObject)
  {
    Free();
    m_Object = anObject;
  }
  LPITEMIDLIST Detach()
  {
    LPITEMIDLIST anObject = m_Object;
    m_Object = NULL;
    return anObject;
  }
  operator LPITEMIDLIST() { return m_Object;}
  operator LPCITEMIDLIST() const { return m_Object;}
  LPITEMIDLIST* operator&() { return &m_Object; }
  LPITEMIDLIST operator->() { return m_Object; }

  /*
  CItemIDList& operator=(LPCITEMIDLIST anObject);
  CItemIDList& operator=(const CItemIDList &anObject);
  */
};

/////////////////////////////
// CDrop

class CDrop
{
  HDROP m_Object;
  bool m_MustBeFinished;
  bool m_Assigned;
  void Free();
public:
  CDrop(bool aMustBeFinished) : m_MustBeFinished(aMustBeFinished),
      m_Assigned(false) {}
  ~CDrop();
  void Attach(HDROP anObject);
  operator HDROP() { return m_Object;}
  bool QueryPoint(LPPOINT aPoint) 
    { return BOOLToBool(::DragQueryPoint(m_Object, aPoint)); }
  void Finish() {  ::DragFinish(m_Object); }
  UINT QueryFile(UINT aFileIndex, LPTSTR aFileName, UINT aFileNameSize);
  UINT QueryCountOfFiles();
  CSysString QueryFileName(UINT aFileIndex);
  void QueryFileNames(CSysStringVector &aFileNames);
};

/////////////////////////////
// Functions

bool GetPathFromIDList(LPCITEMIDLIST anItemIDList, CSysString &aPath);

bool BrowseForFolder(LPBROWSEINFO lpbi, CSysString &aResultPath);
bool BrowseForFolder(HWND anOwner, LPCTSTR aTitle,
    LPCTSTR anInitialFolder, CSysString &aResultPath);

}}


#endif