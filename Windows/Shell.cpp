// Windows/Shell.cpp

#include "StdAfx.h"

#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif
#include "Common/MyCom.h"
#include "Windows/Shell.h"
#include "Windows/COM.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

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

UINT CDrop::QueryCountOfFiles()
{
  return QueryFile(0xFFFFFFFF, (LPTSTR)NULL, 0);
}

UString CDrop::QueryFileName(UINT fileIndex)
{
  UString fileName;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    AString fileNameA;
    UINT bufferSize = QueryFile(fileIndex, (LPTSTR)NULL, 0);
    QueryFile(fileIndex, fileNameA.GetBuffer(bufferSize + 2), bufferSize + 1);
    fileNameA.ReleaseBuffer();
    fileName = GetUnicodeString(fileNameA);
  }
  else
  #endif
  {
    UINT bufferSize = QueryFile(fileIndex, (LPWSTR)NULL, 0);
    QueryFile(fileIndex, fileName.GetBuffer(bufferSize + 2), bufferSize + 1);
    fileName.ReleaseBuffer();
  }
  return fileName;
}

void CDrop::QueryFileNames(UStringVector &fileNames)
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
  bool result = BOOLToBool(::SHGetPathFromIDList(itemIDList, path.GetBuffer(MAX_PATH * 2)));
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


int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /* lp */, LPARAM data) 
{
  switch(uMsg) 
  {
    case BFFM_INITIALIZED:
    {
      SendMessage(hwnd, BFFM_SETSELECTION, TRUE, data);
      break;
    }
    /*
    case BFFM_SELCHANGED: 
    {
      TCHAR dir[MAX_PATH];
      if (::SHGetPathFromIDList((LPITEMIDLIST) lp , dir)) 
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)dir);
      else
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)TEXT(""));
      break;
    }
    */
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

#ifndef _UNICODE

typedef BOOL (WINAPI * SHGetPathFromIDListWP)(LPCITEMIDLIST pidl, LPWSTR pszPath);

bool GetPathFromIDList(LPCITEMIDLIST itemIDList, UString &path)
{
  path.Empty();
  SHGetPathFromIDListWP shGetPathFromIDListW = (SHGetPathFromIDListWP)
    ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "SHGetPathFromIDListW");
  if (shGetPathFromIDListW == 0)
    return false;
  bool result = BOOLToBool(shGetPathFromIDListW(itemIDList, path.GetBuffer(MAX_PATH * 2)));
  path.ReleaseBuffer();
  return result;
}

typedef LPITEMIDLIST (WINAPI * SHBrowseForFolderWP)(LPBROWSEINFOW lpbi);

bool BrowseForFolder(LPBROWSEINFOW browseInfo, UString &resultPath)
{
  NWindows::NCOM::CComInitializer comInitializer;
  SHBrowseForFolderWP shBrowseForFolderW = (SHBrowseForFolderWP)
    ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "SHBrowseForFolderW");
  if (shBrowseForFolderW == 0)
    return false;
  LPITEMIDLIST itemIDList = shBrowseForFolderW(browseInfo);
  if (itemIDList == NULL)
    return false;
  CItemIDList itemIDListHolder;
  itemIDListHolder.Attach(itemIDList);
  return GetPathFromIDList(itemIDList, resultPath);
}


int CALLBACK BrowseCallbackProc2(HWND hwnd, UINT uMsg, LPARAM /* lp */, LPARAM data) 
{
  switch(uMsg) 
  {
    case BFFM_INITIALIZED:
    {
      SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, data);
      break;
    }
    /*
    case BFFM_SELCHANGED: 
    {
      wchar_t dir[MAX_PATH * 2];

      if (shGetPathFromIDListW((LPITEMIDLIST)lp , dir)) 
        SendMessageW(hwnd, BFFM_SETSTATUSTEXTW, 0, (LPARAM)dir);
      else
        SendMessageW(hwnd, BFFM_SETSTATUSTEXTW, 0, (LPARAM)L"");
      break;
    }
    */
    default:
      break;
  }
  return 0;
}


static bool BrowseForFolder(HWND owner, LPCWSTR title, UINT ulFlags, 
    LPCWSTR initialFolder, UString &resultPath)
{
  UString displayName;
  BROWSEINFOW browseInfo;
  browseInfo.hwndOwner = owner;
  browseInfo.pidlRoot = NULL; 
  browseInfo.pszDisplayName = displayName.GetBuffer(MAX_PATH);
  browseInfo.lpszTitle = title;
  browseInfo.ulFlags = ulFlags;
  browseInfo.lpfn = (initialFolder != NULL) ? BrowseCallbackProc2 : NULL;
  browseInfo.lParam = (LPARAM)initialFolder;
  return BrowseForFolder(&browseInfo, resultPath);
}

bool BrowseForFolder(HWND owner, LPCWSTR title, LPCWSTR initialFolder, UString &resultPath)
{
  if (g_IsNT)
    return BrowseForFolder(owner, title, 
      BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS 
      //  | BIF_STATUSTEXT // This flag is not supported when BIF_NEWDIALOGSTYLE is specified.
      , initialFolder, resultPath);
  // BIF_STATUSTEXT; BIF_USENEWUI   (Version 5.0)
  CSysString s;
  bool res = BrowseForFolder(owner, GetSystemString(title), 
      BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS 
      // | BIF_STATUSTEXT  // This flag is not supported when BIF_NEWDIALOGSTYLE is specified.
      , GetSystemString(initialFolder), s); 
  resultPath = GetUnicodeString(s);
  return res;
}

#endif

}}
