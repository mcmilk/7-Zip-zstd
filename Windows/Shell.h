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
  // CItemIDList(LPCITEMIDLIST itemIDList);
  // CItemIDList(const CItemIDList& itemIDList);
  ~CItemIDList() { Free(); }
  void Free();
  void Attach(LPITEMIDLIST object)
  {
    Free();
    m_Object = object;
  }
  LPITEMIDLIST Detach()
  {
    LPITEMIDLIST object = m_Object;
    m_Object = NULL;
    return object;
  }
  operator LPITEMIDLIST() { return m_Object;}
  operator LPCITEMIDLIST() const { return m_Object;}
  LPITEMIDLIST* operator&() { return &m_Object; }
  LPITEMIDLIST operator->() { return m_Object; }

  // CItemIDList& operator=(LPCITEMIDLIST object);
  // CItemIDList& operator=(const CItemIDList &object);
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
  CDrop(bool mustBeFinished) : m_MustBeFinished(mustBeFinished),
      m_Assigned(false) {}
  ~CDrop();
  void Attach(HDROP object);
  operator HDROP() { return m_Object;}
  bool QueryPoint(LPPOINT point) 
    { return BOOLToBool(::DragQueryPoint(m_Object, point)); }
  void Finish() {  ::DragFinish(m_Object); }
  UINT QueryFile(UINT fileIndex, LPTSTR fileName, UINT fileNameSize);
  UINT QueryCountOfFiles();
  CSysString QueryFileName(UINT fileIndex);
  void QueryFileNames(CSysStringVector &fileNames);
};

/////////////////////////////
// Functions

bool GetPathFromIDList(LPCITEMIDLIST itemIDList, CSysString &path);

bool BrowseForFolder(LPBROWSEINFO lpbi, CSysString &resultPath);
bool BrowseForFolder(HWND owner, LPCTSTR title,
    LPCTSTR initialFolder, CSysString &resultPath);

}}


#endif