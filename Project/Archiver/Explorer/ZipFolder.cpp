// CZipFilders.cpp

#include "StdAfx.h"

#include "ZipFolder.h"

#include "HandlersManager.h"
#include "ZipEnum.h"
#include "ZipViewObject.h"
#include "ExtractIcon.h"
#include "resource.h"
#include "Common/StringConvert.h"

#include "Windows/Shell.h"
#include "Windows/ResourceString.h"

#include "MyIDList.h"

#include "../Resource/Extract/resource.h"

using namespace NWindows;

// extern HINSTANCE  g_hInstance;

/*
void PrintNumber(DWORD aNumber, const char *aMessage)
{
  OutputDebugString(aMessage);
  char sz[32];
  ultoa(aNumber, sz, 16);
  OutputDebugString(" ");
  OutputDebugString(sz);
  OutputDebugString("\n");
}
*/

/*
void PrintMessage(const char *aMessage)
{
  MessageBoxA(NULL, aMessage, "Error", MB_OK);
}

void PrintMessage(const wchar_t *aMessage)
{
  MessageBoxW(NULL, aMessage, L"Error", MB_OK);
}
*/

CZipFolder::CZipFolder():
  m_IsRootFolder(true),
  m_CantOpen(false)
{
  m_IDListManager = new CItemIDListManager;
  // m_IDList  = m_IDListManager->Copy(NULL);
  SHGetMalloc(&m_Malloc);
  // PrintNumber(UINT32(this), "CZipFolder");
  // TRACE1("%x CZipFolder\n", DWORD(this));
}

void CZipFolder::Init(CProxyHandler *aProxyHandlerSpec,
    IArchiveHandler100 *anArchiveHandler,
    IArchiveFolder *anArchiveFolderItem, const UString &aFileName)
{
  m_ProxyHandlerSpec = aProxyHandlerSpec;
  m_ProxyHandler = aProxyHandlerSpec;
  // m_IDList = m_IDListManager->Copy(anIDList);
  m_IsRootFolder = false;
  m_ArchiveFolder = anArchiveFolderItem;
  m_ArchiveHandler = anArchiveHandler;
  m_FileName = aFileName;
}

CZipFolder::~CZipFolder()
{
  // TRACE1("%x ~CZipFolder\n", DWORD(this));
  // MessageBox(NULL, "CZipFolder destructor", "", MB_OK);
  // PrintNumber(UINT32(this), "~CZipFolder");
  m_ProxyHandler.Release();
  g_HandlersManager.RemoveNotUsedHandlers();
  delete m_IDListManager;
}

///////////////////////////////////////////////////////////
// IPersist Implementation
// IMPLEMENTED
// IPersistFolder methods
//
//   - GetClassID
//   - Initialize

//-------------------------------------------------------------------
// Procedure....: GetClassID()
// Description..: Returns the CLSID of this server
// -------------------------------------------------------------------

STDMETHODIMP CZipFolder::GetClassID(LPCLSID aClassID)
{
  *aClassID = CLSID_CZipFolder;
  return S_OK;
}

///////////////////////////////////////////////////////////
// IPersistFolder Implementation

STDMETHODIMP CZipFolder::Initialize(LPCITEMIDLIST anIDList)
{
  // PrintNumber(UINT32(this), "Initialize");
  // TRACE1("%x Initialize  ", DWORD(this));
  m_AbsoluteIDList = anIDList;

  if(m_IsRootFolder)
  {
    if(m_ArchiveHandler)
      m_ArchiveHandler.Release();
    CSysString aFileName;
    if(!NShell::GetPathFromIDList(anIDList, aFileName))
      return E_FAIL;
    m_FileName = GetUnicodeString(aFileName);
    // TRACE1("%s", m_FileName);
    bool aHandlerIsNew;
    g_HandlersManager.GetProxyHandler(aFileName, false, 
        &m_ProxyHandlerSpec, &m_ProxyHandler, &m_ArchiveHandler, aHandlerIsNew);
    if (m_ArchiveHandler)
    {
      RETURN_IF_NOT_S_OK(m_ArchiveHandler->BindToRootFolder(&m_ArchiveFolder));
    }

  }
  // TRACE0("\n");
  return S_OK;
}

HRESULT CZipFolder::MyInitialize()
{
  // TRACE1("%x MyInitialize\n", DWORD(this));
  if(m_IsRootFolder)
  {
    if(m_ArchiveHandler)
      m_ArchiveHandler.Release();

    bool aHandlerIsNew;
    m_CantOpen = true;
    if(!g_HandlersManager.GetProxyHandler(GetSystemString(m_FileName), 
        true, &m_ProxyHandlerSpec, &m_ProxyHandler, &m_ArchiveHandler, aHandlerIsNew/*, m_HandlerCookie*/))
    {
      MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      m_ArchiveHandler.Release();
      return S_FALSE;
      // return S_OK
    }
    RETURN_IF_NOT_S_OK(m_ArchiveHandler->BindToRootFolder(&m_ArchiveFolder));
    // m_ProxyHandler->m_AbsoluteIDList = m_AbsoluteIDList;
  }
  return S_OK;
}



///////////////////////////////////////////////////////////
// IShellFolder Implementation
//

STDMETHODIMP CZipFolder::BindToObject(LPCITEMIDLIST anIDList,
    LPBC pbcReserved, REFIID anInterfaceID, LPVOID *ppvOut)
{
  // TRACE1("%x BindToObject\n", DWORD(this));
  *ppvOut = NULL;

  if(!m_ArchiveHandler)
  {
    if (m_IsRootFolder && !m_CantOpen)
    {
      HRESULT aResult = MyInitialize();
      if (aResult != S_OK)
        return E_FAIL;
    }
    else 
      return E_FAIL;
  }

  if (!CheckIDList(anIDList))
  {
    return E_INVALIDARG;
  }


  CComObjectNoLock<CZipFolder> *aShellFolderMain = new 
      CComObjectNoLock<CZipFolder>;
  if(!aShellFolderMain)
    return E_OUTOFMEMORY;
  LPCITEMIDLIST anIDListCur = anIDList;
  CComPtr<IArchiveFolder> aSubItem = m_ArchiveFolder;
  while(anIDListCur->mkid.cb != 0)
  {
    CComPtr<IArchiveFolder> aNewFolder;
    RETURN_IF_NOT_S_OK(aSubItem->BindToFolder(GetIndexFromIDList(anIDListCur), &aNewFolder));
    anIDListCur = NItemIDList::GetNextItem(anIDListCur);
    aSubItem = aNewFolder;
  }

  CComPtr<IShellFolder> aShellFolder(aShellFolderMain);
  /*
  UString aName = m_ProxyHandler->GetNameOfObject(anIDList);
  CArchiveFolderItem *aSubItem = m_ArchiveFolder->FindSubItem(aName, true);
  if(aSubItem == NULL)
  {
    PrintMessage("BindToObject:Can not find subfolder");
    return E_FAIL;
  }
  */
  aShellFolderMain->Init(m_ProxyHandlerSpec, m_ArchiveHandler, aSubItem, m_FileName);

  CShellItemIDList aNewAbsoluteIDPathSpec(m_IDListManager);
  LPITEMIDLIST aNewAbsoluteIDPath = m_IDListManager->Concatenate(m_AbsoluteIDList, anIDList);
  aNewAbsoluteIDPathSpec.Attach(aNewAbsoluteIDPath);
  aShellFolderMain->Initialize(aNewAbsoluteIDPath);
  
  return aShellFolder->QueryInterface(anInterfaceID, ppvOut);
}

STDMETHODIMP CZipFolder::BindToStorage(LPCITEMIDLIST anIDList, 
  LPBC pbcReserved, REFIID anInterfaceID, LPVOID *ppvOut)
{
  *ppvOut = NULL;
  return E_NOTIMPL;
}

static bool CheckIDList(LPCITEMIDLIST anIDList)
{
  if (anIDList->mkid.cb < sizeof(CPropertySignature))
    return false;
  const CPropertySignature &aPropertySignature = *(const CPropertySignature *)anIDList->mkid.abID;
  return (aPropertySignature.ProgramSignature == kZipViewSignature);
}

STDMETHODIMP CZipFolder::CompareIDs( LPARAM lParam, 
    LPCITEMIDLIST anIDList1, LPCITEMIDLIST anIDList2)
{
  // TRACE1("%x CompareIDs\n", DWORD(this));
  // TODO : Implement your own compare routine for pidl1 and pidl2
  //  Note that pidl1 and pidl2 may be fully qualified pidls, in which
  //  you shouldn't compare just the first items in the respective pidls

  // Hint : Use lParam to determine whether to compare items or sub-items

  // Return one of these:
  // < 0 ; if pidl1 should precede pidl2
  // > 0 ; if pidl1 should follow pidl2
  // = 0 ; if pidl1 == pidl2

  if (!CheckIDList(anIDList1))
    return -1;
  if (!CheckIDList(anIDList2))
    return 1;

  if(!m_ArchiveHandler)
  {
    if (m_IsRootFolder && !m_CantOpen)
    {
      HRESULT aResult = MyInitialize();
      if (aResult != S_OK)
        return E_FAIL;
    }
    else 
      return E_FAIL;
  }

  LPCITEMIDLIST anIDListCur1 = anIDList1;
  LPCITEMIDLIST anIDListCur2 = anIDList2;

  CComPtr<IArchiveFolder> aSubItem = m_ArchiveFolder;
  CComPtr<IArchiveFolder> anArchiveFolder1 = m_ArchiveFolder;
  CComPtr<IArchiveFolder> anArchiveFolder2 = m_ArchiveFolder;

  while(anIDListCur1->mkid.cb != 0 && anIDListCur2->mkid.cb != 0)
  {
    UINT32 anIndex1 = GetIndexFromIDList(anIDListCur1);
    UINT32 anIndex2 = GetIndexFromIDList(anIDListCur2);
    bool anIsFolder1 = IsObjectFolder(anArchiveFolder1, anIndex1);
    bool anIsFolder2 = IsObjectFolder(anArchiveFolder2, anIndex2);
    if (anIndex1 != anIndex2)
    {
      if(anIsFolder1 && !anIsFolder2)
        return -1;
      else if(!anIsFolder1 && anIsFolder2)
        return 1;
      UString aName1 = GetNameOfObject(anArchiveFolder1, anIndex1);
      UString aName2 = GetNameOfObject(anArchiveFolder2, anIndex2);
      int i = aName1.CollateNoCase(aName2);
      if(i != 0)
        return i;
      if (!anIsFolder1 && !anIsFolder2)
        return 0;
    }
    if (anIsFolder1)
    {
      CComPtr<IArchiveFolder> aSubFolder;
      RETURN_IF_NOT_S_OK(anArchiveFolder1->BindToFolder(anIndex1, &aSubFolder));
      anArchiveFolder1 = aSubFolder;
      anIDListCur1 = NItemIDList::GetNextItem(anIDListCur1);
    }
    if (anIsFolder2)
    {
      CComPtr<IArchiveFolder> aSubFolder;
      RETURN_IF_NOT_S_OK(anArchiveFolder2->BindToFolder(anIndex2, &aSubFolder));
      anArchiveFolder2 = aSubFolder;
      anIDListCur2 = NItemIDList::GetNextItem(anIDListCur2);
    }
  }
  if(anIDListCur2->mkid.cb > 0)
    return -1;
  if(anIDListCur1->mkid.cb > 0)
    return 1;
  return 0;
}

STDMETHODIMP CZipFolder::CreateViewObject( HWND hwndOwner, 
    REFIID anInterfaceID, LPVOID *ppvOut)
{
  // TRACE1("%x CreateViewObject\n", DWORD(this));
  *ppvOut = NULL;
  
  if (!IsEqualIID (anInterfaceID, IID_IShellView))
    return E_NOTIMPL;

  if(!m_ArchiveHandler)
  {
    if (m_IsRootFolder && !m_CantOpen)
    {
      HRESULT aResult = MyInitialize();
      if (aResult != S_OK)
        return E_FAIL;
    }
    else 
      return E_FAIL;
  }

  CComObjectNoLock<CZipViewObject> *aShellViewMain = 
      new CComObjectNoLock<CZipViewObject>;
  if(!aShellViewMain)
    return E_OUTOFMEMORY;
  CComPtr<IShellView> aShellView(aShellViewMain);
  aShellViewMain->Init(this, m_ProxyHandlerSpec, m_ArchiveFolder);

  return aShellView->QueryInterface(anInterfaceID, ppvOut);
}

STDMETHODIMP CZipFolder::EnumObjects(HWND hwndOwner, 
    DWORD aFlags, LPENUMIDLIST *anEnumIDList)
{
  // TRACE1("%x EnumObjects\n", DWORD(this));

  if(!m_ArchiveHandler)
  {
    if (m_IsRootFolder && !m_CantOpen)
    {
      HRESULT aResult = MyInitialize();
      if (aResult != S_OK)
        return E_FAIL;
    }
    else 
      return E_FAIL;
  }

  CComObjectNoLock<CZipEnumIDList> *anEnumObject = 
      new CComObjectNoLock<CZipEnumIDList>;
  if (anEnumObject == NULL)
    return E_OUTOFMEMORY;
 
  CComPtr<IEnumIDList> anEnum(anEnumObject);
  anEnumObject->Init(aFlags, m_ArchiveFolder);
  return anEnum->QueryInterface(IID_IEnumIDList, (LPVOID*)anEnumIDList);
}

STDMETHODIMP CZipFolder::GetAttributesOf(UINT uCount, 
    LPCITEMIDLIST anIDLists[], LPDWORD pdwAttribs)
{
  // TODO : Return appropriate ShellFolder attributes in pdwAttribs
  // By default the following ORed value suffices.
  //
  // TRACE1("%x GetAttributesOf\n", DWORD(this));

  if(!m_ArchiveHandler)
  {
    if (m_IsRootFolder && !m_CantOpen)
    {
      HRESULT aResult = MyInitialize();
      if (aResult != S_OK)
        return E_FAIL;
    }
    else 
      return E_FAIL;
  }
  
  if(uCount == 0)
  {
    *pdwAttribs = SFGAO_FOLDER;
    return S_OK;
  }

  DWORD aMask = *pdwAttribs;
  bool anAllHasSubFolder = true;
  bool anAllAreFolders = true;
  for(UINT anIndex = 0; anIndex < uCount; anIndex++)
  {
    LPCITEMIDLIST anIDList = anIDLists[anIndex];
    bool anIsFolder = IsObjectFolder(m_ArchiveFolder, anIDList);
    anAllAreFolders = anAllAreFolders && anIsFolder;
    if(anAllHasSubFolder && (aMask & SFGAO_HASSUBFOLDER) != 0)
    {
      bool aHasSubFolderLoc = false;
      if(anIsFolder)
      {
        CComPtr<IArchiveFolder> aSubFolder;
        RETURN_IF_NOT_S_OK(m_ArchiveFolder->BindToFolder(
            GetIndexFromIDList(anIDList), &aSubFolder));
        UINT32 aNumSubFolders;
        RETURN_IF_NOT_S_OK(aSubFolder->GetNumberOfSubFolders(&aNumSubFolders));
        aHasSubFolderLoc = (aNumSubFolders > 0);
      }
      anAllHasSubFolder = aHasSubFolderLoc;
    }
  }

  DWORD anAttributes = 0;
  if((aMask & SFGAO_FOLDER) != 0)
    if (anAllAreFolders)
      anAttributes |= SFGAO_FOLDER;
  if((aMask & SFGAO_HASSUBFOLDER) != 0)
    if (anAllHasSubFolder)
      anAttributes |= SFGAO_HASSUBFOLDER;
  *pdwAttribs = anAttributes;
  return S_OK;
}

/*-------------------------------------------------------------------*/
// Procedure....: GetUIObjectOf()
// Description..: Returns an interface to perform UI actions
/*-------------------------------------------------------------------*/

// Enumeration object for the folder


STDMETHODIMP CZipFolder::GetUIObjectOf( HWND hwndOwner, 
   UINT aNumItems, LPCITEMIDLIST FAR* anItemIDLists, REFIID anInterfaceID, 
   UINT FAR* prgfInOut, LPVOID FAR* anInterfaces )
{
  // TRACE1("%x GetUIObjectOf\n", DWORD(this));
  *anInterfaces = NULL;

  if(!m_ArchiveHandler)
  {
    if (m_IsRootFolder && !m_CantOpen)
    {
      HRESULT aResult = MyInitialize();
      if (aResult != S_OK)
        return E_FAIL;
    }
    else 
      return E_FAIL;
  }

  if(anInterfaceID == IID_IExtractIcon )
  {
    if(aNumItems != 1)  
      return E_INVALIDARG;
    CComObjectNoLock<CExtractIconImp> *anIExtractIconMain = new 
        CComObjectNoLock<CExtractIconImp>;
    anIExtractIconMain->Init(m_ArchiveFolder, GetIndexFromIDList(anItemIDLists[0]));
    CComPtr<IExtractIcon> anIExtractIcon(anIExtractIconMain);
    return anIExtractIcon->QueryInterface(anInterfaceID, anInterfaces);
  }
  else 
    return E_NOINTERFACE;
}

  
STDMETHODIMP CZipFolder::GetDisplayNameOf(LPCITEMIDLIST anIDList, 
    DWORD aFlags, LPSTRRET aNameInfo)
{
  // TRACE1("%x GetDisplayNameOf\n", DWORD(this));
  
  if(!m_ArchiveHandler)
  {
    if (m_IsRootFolder && !m_CantOpen)
    {
      HRESULT aResult = MyInitialize();
      if (aResult != S_OK)
        return E_FAIL;
    }
    else 
      return E_FAIL;
  }
  
  if (m_Malloc == NULL)
    return E_OUTOFMEMORY;
  if (!aNameInfo)
    return E_INVALIDARG;
  UString aName;
  if((aFlags & SHGDN_INFOLDER ) == 0)
  {
    CComPtr<IArchiveFolder> anItem = m_ArchiveFolder;
    UString aPathInArchive;

    while (true)
    {
      CComPtr<IArchiveFolder> aNewFolder;
      RETURN_IF_NOT_S_OK(anItem->BindToParentFolder(&aNewFolder));  
      if (!aNewFolder)
        break;
      CComBSTR aCurrentName;
      RETURN_IF_NOT_S_OK(anItem->GetName(&aCurrentName));
      aPathInArchive = UString(aCurrentName) + UString(L'\\') + aPathInArchive;
      anItem = aNewFolder;
    }
    aName = m_FileName + UString(L'\\') + aPathInArchive;
  }
  LPCITEMIDLIST anIDListCur = anIDList;
  IArchiveFolder *aSubItem = m_ArchiveFolder;
  while(anIDListCur->mkid.cb != 0)
  {
    CComPtr<IArchiveFolder> aNewFolder;
    RETURN_IF_NOT_S_OK(aSubItem->BindToFolder(GetIndexFromIDList(anIDListCur), &aNewFolder));
    anIDListCur = NItemIDList::GetNextItem(anIDListCur);
    CComBSTR aCurrentName;
    RETURN_IF_NOT_S_OK(aNewFolder->GetName(&aCurrentName));
    aSubItem = aNewFolder;
    aName += aCurrentName;
    if(anIDListCur->mkid.cb != 0)
      aName += '\\';
  }
  aNameInfo->pOleStr = (wchar_t *)m_Malloc->Alloc((aName.Length() + 1) * sizeof(wchar_t));
  if(aNameInfo->pOleStr == NULL)
    return E_OUTOFMEMORY;
  aNameInfo->uType = STRRET_WSTR;
  wcscpy(aNameInfo->pOleStr, aName);
  return S_OK;
}

STDMETHODIMP CZipFolder::ParseDisplayName( HWND hwndOwner, 
    LPBC pbcReserved, LPOLESTR lpDisplayName, LPDWORD pdwEaten, 
    LPITEMIDLIST *pPidlNew, LPDWORD pdwAttributes)
{
  // TODO : Return the anIDList associated with lpDisplayName
  return E_NOTIMPL;
}

STDMETHODIMP CZipFolder::SetNameOf(HWND hwndOwner, 
  LPCITEMIDLIST anIDList, LPCOLESTR lpName,
  DWORD dw, LPITEMIDLIST *pPidlOut)
{
  // TODO : Set the display name
  return E_NOTIMPL;
}

/*
STDMETHODIMP CZipFolder::OnChange(LONG lEvent, 
    LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
  return S_OK;
}
*/