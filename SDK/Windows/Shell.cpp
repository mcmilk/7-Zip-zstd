// Windows/Shell.cpp

#include "StdAfx.h"

#include "Windows/Shell.h"
#include "Windows/COM.h"

namespace NWindows {
namespace NShell {

/////////////////////////
// CItemIDList

void CItemIDList::Free()
{ 
  if(m_Object == NULL)
    return;
  CComPtr<IMalloc> aShellMalloc;
  if(::SHGetMalloc(&aShellMalloc) != NOERROR)
    throw 41099;
  aShellMalloc->Free(m_Object);
  m_Object = NULL;
}

/*
CItemIDList::(LPCITEMIDLIST anItemIDList): m_Object(NULL)
  {  *this = anItemIDList; }
CItemIDList::(const CItemIDList& anItemIDList): m_Object(NULL)
  {  *this = anItemIDList; }

CItemIDList& CItemIDList::operator=(LPCITEMIDLIST anObject)
{
  Free();
  if (anObject != 0)
  {
    UINT32 aSize = GetSize(anObject);
    m_Object = (LPITEMIDLIST)CoTaskMemAlloc(aSize);
    if(m_Object != NULL)
      MoveMemory(m_Object, anObject, aSize);
  }
  return *this;
}

CItemIDList& CItemIDList::operator=(const CItemIDList &anObject)
{
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
*/

/////////////////////////////
// CDrop

void CDrop::Attach(HDROP anObject)
{
  Free();
  m_Object = anObject;
  m_Assigned = true;
}

void CDrop::Free()
{
  if(m_MustBeFinished && m_Assigned)
    Finish();
  m_Assigned = false;
}

CDrop::~CDrop()
{
  Free();
}

UINT CDrop::QueryFile(UINT aFileIndex, LPTSTR aFileName, UINT aFileNameSize)
{
  return ::DragQueryFile(m_Object, aFileIndex, aFileName, aFileNameSize);
}

UINT CDrop::QueryCountOfFiles()
{
  return QueryFile(0xFFFFFFFF, NULL, 0);
}

CSysString CDrop::QueryFileName(UINT aFileIndex)
{
  CSysString aFileName;
  UINT aBufferSize = QueryFile(aFileIndex, NULL, 0);
  QueryFile(aFileIndex, aFileName.GetBuffer(aBufferSize), aBufferSize + 1);
  aFileName.ReleaseBuffer();
  return aFileName;
}

void CDrop::QueryFileNames(CSysStringVector &aFileNames)
{
  aFileNames.Clear();
  UINT aNumFiles = QueryCountOfFiles();
  aFileNames.Reserve(aNumFiles);
  for(UINT i = 0; i < aNumFiles; i++)
    aFileNames.Add(QueryFileName(i));
}


/////////////////////////////
// Functions

bool GetPathFromIDList(LPCITEMIDLIST anItemIDList, CSysString &aPath)
{
  bool aResult = BOOLToBool(::SHGetPathFromIDList(anItemIDList, 
      aPath.GetBuffer(MAX_PATH)));
  aPath.ReleaseBuffer();
  return aResult;
}

bool BrowseForFolder(LPBROWSEINFO aBrowseInfo, CSysString &aResultPath)
{
  NWindows::NCOM::CComInitializer aComInitializer;
  LPITEMIDLIST anItemIDList = ::SHBrowseForFolder(aBrowseInfo);
  if (anItemIDList == NULL)
    return false;
  CItemIDList anItemIDListHolder;
  anItemIDListHolder.Attach(anItemIDList);
  return GetPathFromIDList(anItemIDList, aResultPath);
}


int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM aData) 
{
  switch(uMsg) 
  {
    case BFFM_INITIALIZED:
    {
      SendMessage(hwnd, BFFM_SETSELECTION, TRUE, aData);
      break;
    }
    case BFFM_SELCHANGED: 
    {
      TCHAR aDir[MAX_PATH];
      if (::SHGetPathFromIDList((LPITEMIDLIST) lp , aDir)) 
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)aDir);
      else
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)_T(""));
      break;
    }
    default:
      break;
  }
  return 0;
}


bool BrowseForFolder(HWND anOwner, LPCTSTR aTitle, UINT ulFlags, 
    LPCTSTR anInitialFolder, CSysString &aResultPath)
{
  CSysString aDisplayName;
  BROWSEINFO aBrowseInfo;
  aBrowseInfo.hwndOwner = anOwner;
  aBrowseInfo.pidlRoot = NULL; 
  aBrowseInfo.pszDisplayName = aDisplayName.GetBuffer(MAX_PATH);
  aBrowseInfo.lpszTitle = aTitle;
  aBrowseInfo.ulFlags = ulFlags;
  aBrowseInfo.lpfn = (anInitialFolder != NULL) ? BrowseCallbackProc : NULL;
  aBrowseInfo.lParam = (LPARAM)anInitialFolder;
  return BrowseForFolder(&aBrowseInfo, aResultPath);
}

bool BrowseForFolder(HWND anOwner, LPCTSTR aTitle, 
    LPCTSTR anInitialFolder, CSysString &aResultPath)
{
  return BrowseForFolder(anOwner, aTitle, 
      BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT, anInitialFolder, aResultPath);
  // BIF_STATUSTEXT; BIF_USENEWUI   (Version 5.0)
}

}}
