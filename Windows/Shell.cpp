// Windows/Shell.cpp

#include "StdAfx.h"

#include "Common/MyCom.h"
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
  CMyComPtr<IMalloc> shellMalloc;
  if(::SHGetMalloc(&shellMalloc) != NOERROR)
    throw 41099;
  shellMalloc->Free(m_Object);
  m_Object = NULL;
}

/*
CItemIDList::(LPCITEMIDLIST itemIDList): m_Object(NULL)
  {  *this = itemIDList; }
CItemIDList::(const CItemIDList& itemIDList): m_Object(NULL)
  {  *this = itemIDList; }

CItemIDList& CItemIDList::operator=(LPCITEMIDLIST object)
{
  Free();
  if (object != 0)
  {
    UINT32 size = GetSize(object);
    m_Object = (LPITEMIDLIST)CoTaskMemAlloc(size);
    if(m_Object != NULL)
      MoveMemory(m_Object, object, size);
  }
  return *this;
}

CItemIDList& CItemIDList::operator=(const CItemIDList &object)
{
  Free();
  if(object.m_Object != NULL)
  {
    UINT32 size = GetSize(object.m_Object);
    m_Object = (LPITEMIDLIST)CoTaskMemAlloc(size);
    if(m_Object != NULL)
      MoveMemory(m_Object, object.m_Object, size);
  }
  return *this;
}
*/
/////////////////////////////
// CDrop

void CDrop::Attach(HDROP object)
{
  Free();
  m_Object = object;
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

UINT CDrop::QueryFile(UINT fileIndex, LPTSTR fileName, UINT fileNameSize)
{
  return ::DragQueryFile(m_Object, fileIndex, fileName, fileNameSize);
}

UINT CDrop::QueryCountOfFiles()
{
  return QueryFile(0xFFFFFFFF, NULL, 0);
}

CSysString CDrop::QueryFileName(UINT fileIndex)
{
  CSysString fileName;
  UINT bufferSize = QueryFile(fileIndex, NULL, 0);
  QueryFile(fileIndex, fileName.GetBuffer(bufferSize), bufferSize + 1);
  fileName.ReleaseBuffer();
  return fileName;
}

void CDrop::QueryFileNames(CSysStringVector &fileNames)
{
  fileNames.Clear();
  UINT numFiles = QueryCountOfFiles();
  fileNames.Reserve(numFiles);
  for(UINT i = 0; i < numFiles; i++)
    fileNames.Add(QueryFileName(i));
}


/////////////////////////////
// Functions

bool GetPathFromIDList(LPCITEMIDLIST itemIDList, CSysString &path)
{
  bool result = BOOLToBool(::SHGetPathFromIDList(itemIDList, 
      path.GetBuffer(MAX_PATH)));
  path.ReleaseBuffer();
  return result;
}

bool BrowseForFolder(LPBROWSEINFO browseInfo, CSysString &resultPath)
{
  NWindows::NCOM::CComInitializer comInitializer;
  LPITEMIDLIST itemIDList = ::SHBrowseForFolder(browseInfo);
  if (itemIDList == NULL)
    return false;
  CItemIDList itemIDListHolder;
  itemIDListHolder.Attach(itemIDList);
  return GetPathFromIDList(itemIDList, resultPath);
}


int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM data) 
{
  switch(uMsg) 
  {
    case BFFM_INITIALIZED:
    {
      SendMessage(hwnd, BFFM_SETSELECTION, TRUE, data);
      break;
    }
    case BFFM_SELCHANGED: 
    {
      TCHAR dir[MAX_PATH];
      if (::SHGetPathFromIDList((LPITEMIDLIST) lp , dir)) 
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)dir);
      else
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)TEXT(""));
      break;
    }
    default:
      break;
  }
  return 0;
}


bool BrowseForFolder(HWND owner, LPCTSTR title, UINT ulFlags, 
    LPCTSTR initialFolder, CSysString &resultPath)
{
  CSysString displayName;
  BROWSEINFO browseInfo;
  browseInfo.hwndOwner = owner;
  browseInfo.pidlRoot = NULL; 
  browseInfo.pszDisplayName = displayName.GetBuffer(MAX_PATH);
  browseInfo.lpszTitle = title;
  browseInfo.ulFlags = ulFlags;
  browseInfo.lpfn = (initialFolder != NULL) ? BrowseCallbackProc : NULL;
  browseInfo.lParam = (LPARAM)initialFolder;
  return BrowseForFolder(&browseInfo, resultPath);
}

bool BrowseForFolder(HWND owner, LPCTSTR title, 
    LPCTSTR initialFolder, CSysString &resultPath)
{
  return BrowseForFolder(owner, title, 
      BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT, initialFolder, resultPath);
  // BIF_STATUSTEXT; BIF_USENEWUI   (Version 5.0)
}

}}
