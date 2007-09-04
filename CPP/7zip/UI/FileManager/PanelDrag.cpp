// PanelDrag.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/Memory.h"
#include "Windows/FileDir.h"
#include "Windows/Shell.h"

#include "../Common/ArchiveName.h"
#include "../Common/CompressCall.h"

#include "MessagesDialog.h"

#include "App.h"
#include "EnumFormatEtc.h"

using namespace NWindows;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

static wchar_t *kTempDirPrefix = L"7zE"; 
static LPCTSTR kSvenZipSetFolderFormat = TEXT("7-Zip::SetTargetFolder"); 

////////////////////////////////////////////////////////

class CDataObject: 
  public IDataObject,
  public CMyUnknownImp
{
private:
  FORMATETC m_Etc;
  UINT m_SetFolderFormat;

public:
  MY_UNKNOWN_IMP1_MT(IDataObject)

  STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM medium);
  STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM medium);
  STDMETHODIMP QueryGetData(LPFORMATETC pformatetc );

  STDMETHODIMP GetCanonicalFormatEtc ( LPFORMATETC /* pformatetc */, LPFORMATETC pformatetcOut)
    { pformatetcOut->ptd = NULL; return ResultFromScode(E_NOTIMPL); }

  STDMETHODIMP SetData(LPFORMATETC etc, STGMEDIUM *medium, BOOL release);
  STDMETHODIMP EnumFormatEtc(DWORD drection, LPENUMFORMATETC *enumFormatEtc);
  
  STDMETHODIMP DAdvise(FORMATETC * /* etc */, DWORD /* advf */, LPADVISESINK /* pAdvSink */, DWORD * /* pdwConnection */)
    { return OLE_E_ADVISENOTSUPPORTED; }
  STDMETHODIMP DUnadvise(DWORD /* dwConnection */) { return OLE_E_ADVISENOTSUPPORTED; }
  STDMETHODIMP EnumDAdvise( LPENUMSTATDATA * /* ppenumAdvise */) { return OLE_E_ADVISENOTSUPPORTED; }

  CDataObject();

  NMemory::CGlobal hGlobal;
  UString Path;
};

CDataObject::CDataObject()
{
  m_SetFolderFormat = RegisterClipboardFormat(kSvenZipSetFolderFormat); 
  m_Etc.cfFormat = CF_HDROP;
  m_Etc.ptd = NULL;
  m_Etc.dwAspect = DVASPECT_CONTENT;
  m_Etc.lindex = -1;
  m_Etc.tymed = TYMED_HGLOBAL;
}

STDMETHODIMP CDataObject::SetData(LPFORMATETC etc, STGMEDIUM *medium, BOOL /* release */)
{ 
  if (etc->cfFormat == m_SetFolderFormat && etc->tymed == TYMED_HGLOBAL && 
      etc->dwAspect == DVASPECT_CONTENT && medium->tymed == TYMED_HGLOBAL)
  {
    Path.Empty();
    if (medium->hGlobal == 0)
      return S_OK; 
    size_t size = GlobalSize(medium->hGlobal) / sizeof(wchar_t);
    const wchar_t *src = (const wchar_t *)GlobalLock(medium->hGlobal);
    if (src != 0)
    {
      for (size_t i = 0; i < size; i++)
      {
        wchar_t c = src[i];
        if (c == 0)
          break;
        Path += c;
      }
      GlobalUnlock(medium->hGlobal);
      return S_OK; 
    }
  }
  return E_NOTIMPL; 
}

static HGLOBAL DuplicateGlobalMem(HGLOBAL srcGlobal)
{
  SIZE_T size = GlobalSize(srcGlobal);
  const void *src = GlobalLock(srcGlobal);
  if (src == 0)
    return 0;
  HGLOBAL destGlobal = GlobalAlloc(GHND | GMEM_SHARE, size);
  if (destGlobal != 0)
  {
    void *dest = GlobalLock(destGlobal);
    if (dest == 0)
    {
      GlobalFree(destGlobal);
      destGlobal = 0;
    }
    else
    {
      memcpy(dest, src, size);
      GlobalUnlock(destGlobal);
    }
  }
  GlobalUnlock(srcGlobal);
  return destGlobal;
}

STDMETHODIMP CDataObject::GetData(LPFORMATETC etc, LPSTGMEDIUM medium)
{
  RINOK(QueryGetData(etc));
  medium->tymed = m_Etc.tymed;
  medium->pUnkForRelease = 0;
  medium->hGlobal = DuplicateGlobalMem(hGlobal);
  if (medium->hGlobal == 0)
    return E_OUTOFMEMORY;
  return S_OK;
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC /* etc */, LPSTGMEDIUM /* medium */)
{
  // Seems Windows doesn't call it, so we will not implement it.
  return E_UNEXPECTED;
}


STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC etc)
{
  if ((m_Etc.tymed & etc->tymed) &&
       m_Etc.cfFormat == etc->cfFormat &&
       m_Etc.dwAspect == etc->dwAspect)
    return S_OK;
  return DV_E_FORMATETC;
}

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD direction, LPENUMFORMATETC FAR* enumFormatEtc)
{
  if(direction != DATADIR_GET)
    return E_NOTIMPL;
  return CreateEnumFormatEtc(1, &m_Etc, enumFormatEtc);
}

////////////////////////////////////////////////////////

class CDropSource: 
  public IDropSource,
  public CMyUnknownImp
{
  DWORD m_Effect;
public:
  MY_UNKNOWN_IMP1_MT(IDropSource)
  STDMETHOD(QueryContinueDrag)(BOOL escapePressed, DWORD keyState);
  STDMETHOD(GiveFeedback)(DWORD effect);


  bool NeedExtract;
  CPanel *Panel;
  CRecordVector<UInt32> Indices;
  UString Folder;
  CDataObject *DataObjectSpec;
  CMyComPtr<IDataObject> DataObject;
  
  bool NeedPostCopy;
  HRESULT Result;
  UStringVector Messages;

  CDropSource(): NeedPostCopy(false), Panel(0), Result(S_OK), m_Effect(DROPEFFECT_NONE) {}
};

STDMETHODIMP CDropSource::QueryContinueDrag(BOOL escapePressed, DWORD keyState)
{
  if(escapePressed == TRUE)
    return DRAGDROP_S_CANCEL;
  if((keyState & MK_LBUTTON) == 0)
  {
    if (m_Effect == DROPEFFECT_NONE)
      return DRAGDROP_S_CANCEL;
    Result = S_OK;
    bool needExtract = NeedExtract;
    // MoveMode = (((keyState & MK_SHIFT) != 0) && MoveIsAllowed);
    if (!DataObjectSpec->Path.IsEmpty())
    {
      needExtract = false;
      NeedPostCopy = true;
    }
    if (needExtract)
    {
      Result = Panel->CopyTo(Indices, Folder, 
          false, // moveMode, 
          false, // showMessages
          &Messages);
      if (Result != S_OK || !Messages.IsEmpty())
        return DRAGDROP_S_CANCEL;
    }
    return DRAGDROP_S_DROP;
  }
  return S_OK;
}

STDMETHODIMP CDropSource::GiveFeedback(DWORD effect)
{
  m_Effect = effect;
  return DRAGDROP_S_USEDEFAULTCURSORS;
}

static bool CopyNamesToHGlobal(NMemory::CGlobal &hgDrop, const UStringVector &names)
{
  size_t totalLength = 1;

  #ifndef _UNICODE
  if (!g_IsNT)
  {
    AStringVector namesA;
    int i;
    for (i = 0; i < names.Size(); i++)
      namesA.Add(GetSystemString(names[i]));
    for (i = 0; i < names.Size(); i++)
      totalLength += namesA[i].Length() + 1;
    
    if (!hgDrop.Alloc(GHND | GMEM_SHARE, totalLength * sizeof(CHAR) + sizeof(DROPFILES)))
      return false;
    
    NMemory::CGlobalLock dropLock(hgDrop);
    DROPFILES* dropFiles = (DROPFILES*)dropLock.GetPointer();
    if (dropFiles == 0)
      return false;
    dropFiles->fNC = FALSE;
    dropFiles->pt.x = 0;
    dropFiles->pt.y = 0;
    dropFiles->pFiles = sizeof(DROPFILES);
    dropFiles->fWide = FALSE;
    CHAR *p = (CHAR *)((BYTE *)dropFiles + sizeof(DROPFILES));
    for (i = 0; i < names.Size(); i++)
    {
      const AString &s = namesA[i];
      int fullLength = s.Length() + 1;
      MyStringCopy(p, (const char *)s);
      p += fullLength;
      totalLength -= fullLength;
    }
    *p = 0;
  }
  else
  #endif
  {
    int i;
    for (i = 0; i < names.Size(); i++)
      totalLength += names[i].Length() + 1;
    
    if (!hgDrop.Alloc(GHND | GMEM_SHARE, totalLength * sizeof(WCHAR) + sizeof(DROPFILES)))
      return false;
    
    NMemory::CGlobalLock dropLock(hgDrop);
    DROPFILES* dropFiles = (DROPFILES*)dropLock.GetPointer();
    if (dropFiles == 0)
      return false;
    dropFiles->fNC = FALSE;
    dropFiles->pt.x = 0;
    dropFiles->pt.y = 0;
    dropFiles->pFiles = sizeof(DROPFILES);
    dropFiles->fWide = TRUE;
    WCHAR *p = (WCHAR *)((BYTE *)dropFiles + sizeof(DROPFILES));
    for (i = 0; i < names.Size(); i++)
    {
      const UString &s = names[i];
      int fullLength = s.Length() + 1;
      MyStringCopy(p, (const WCHAR *)s);
      p += fullLength;
      totalLength -= fullLength;
    }
    *p = 0;
  }
  return true;
}

void CPanel::OnDrag(LPNMLISTVIEW /* nmListView */)
{
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  if (!DoesItSupportOperations())
    return;
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  if (indices.Size() == 0)
    return;

  // CSelectedState selState;
  // SaveSelectedState(selState);

  UString dirPrefix; 
  NFile::NDirectory::CTempDirectoryW tempDirectory;

  bool isFSFolder = IsFSFolder();
  if (isFSFolder)
    dirPrefix = _currentFolderPrefix;
  else
  {
    tempDirectory.Create(kTempDirPrefix);
    dirPrefix = tempDirectory.GetPath();
    NFile::NName::NormalizeDirPathPrefix(dirPrefix);
  }

  CDataObject *dataObjectSpec = new CDataObject;
  CMyComPtr<IDataObject> dataObject = dataObjectSpec;

  {
    UStringVector names;
    for (int i = 0; i < indices.Size(); i++)
    {
      UInt32 index = indices[i];
      UString s;
      if (isFSFolder)
        s = GetItemRelPath(index);
      else
        s = GetItemName(index);
      names.Add(dirPrefix + s);
    }
    if (!CopyNamesToHGlobal(dataObjectSpec->hGlobal, names))
      return;
  }

  CDropSource *dropSourceSpec = new CDropSource;
  CMyComPtr<IDropSource> dropSource = dropSourceSpec;
  dropSourceSpec->NeedExtract = !isFSFolder;
  dropSourceSpec->Panel = this;
  dropSourceSpec->Indices = indices;
  dropSourceSpec->Folder = dirPrefix;
  dropSourceSpec->DataObjectSpec = dataObjectSpec;
  dropSourceSpec->DataObject = dataObjectSpec;

  bool moveIsAllowed = isFSFolder;

  DWORD effectsOK = DROPEFFECT_COPY;
  if (moveIsAllowed)
    effectsOK |= DROPEFFECT_MOVE;
  DWORD effect;
  _panelCallback->DragBegin();
  HRESULT res = DoDragDrop(dataObject, dropSource, effectsOK, &effect);
  _panelCallback->DragEnd();
  bool canceled = (res == DRAGDROP_S_CANCEL);
  if (res == DRAGDROP_S_DROP)
  {
    res = dropSourceSpec->Result;
    if (dropSourceSpec->NeedPostCopy)
      if (!dataObjectSpec->Path.IsEmpty())
      {
        NFile::NName::NormalizeDirPathPrefix(dataObjectSpec->Path);
        res = CopyTo(indices, dataObjectSpec->Path, 
          (effect == DROPEFFECT_MOVE),// dropSourceSpec->MoveMode, 
          false, // showErrorMessages
          &dropSourceSpec->Messages);
      }
    /*
    if (effect == DROPEFFECT_MOVE)
      RefreshListCtrl(selState);
    */
  }
  else
  {
    if (res != DRAGDROP_S_CANCEL && res != S_OK)
      MessageBoxError(res);
    res = dropSourceSpec->Result;
  }

  if (!dropSourceSpec->Messages.IsEmpty())
  {
    CMessagesDialog messagesDialog;
    messagesDialog.Messages = &dropSourceSpec->Messages;
    messagesDialog.Create((*this));
  }
  if (res != S_OK && res != E_ABORT)
    MessageBoxError(res);
  if (res == S_OK && dropSourceSpec->Messages.IsEmpty() && !canceled)
    KillSelection();
}

void CDropTarget::QueryGetData(IDataObject *dataObject)
{
  FORMATETC etc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  m_DropIsAllowed = (dataObject->QueryGetData(&etc) == S_OK);

}

static void MySetDropHighlighted(HWND hWnd, int index, bool enable)
{
  LVITEM item;
  item.mask = LVIF_STATE;
  item.iItem = index;
  item.iSubItem = 0;
  item.state = enable ? LVIS_DROPHILITED : 0;
  item.stateMask = LVIS_DROPHILITED;
  item.pszText = 0;
  ListView_SetItem(hWnd, &item);
}

void CDropTarget::RemoveSelection()
{
  if (m_SelectionIndex >= 0 && m_Panel != 0)
    MySetDropHighlighted(m_Panel->_listView, m_SelectionIndex, false);
  m_SelectionIndex = -1;
}

void CDropTarget::PositionCursor(POINTL ptl)
{
  m_SubFolderIndex = -1;
  POINT pt;
  pt.x = ptl.x;
  pt.y = ptl.y;

  RemoveSelection();
  m_IsAppTarget = true;
  m_Panel = NULL;

  m_PanelDropIsAllowed = true;
  if (!m_DropIsAllowed)
    return;
  {
    POINT pt2 = pt;
    App->_window.ScreenToClient(&pt2);
    for (int i = 0; i < kNumPanelsMax; i++)
      if (App->IsPanelVisible(i))
        if (App->Panels[i].IsEnabled())
          if (ChildWindowFromPointEx(App->_window, pt2, 
              CWP_SKIPINVISIBLE | CWP_SKIPDISABLED) == (HWND)App->Panels[i])
          {
            m_Panel = &App->Panels[i];
            m_IsAppTarget = false;
            if (i == SrcPanelIndex)
            {
              m_PanelDropIsAllowed = false;
              return;
            }
            break;
          }
    if (m_IsAppTarget)
    {
      if (TargetPanelIndex >= 0)
        m_Panel = &App->Panels[TargetPanelIndex];
      return;
    }
  }

  /*
  m_PanelDropIsAllowed = m_Panel->DoesItSupportOperations();
  if (!m_PanelDropIsAllowed)
    return;
  */

  if (!m_Panel->IsFSFolder() && !m_Panel->IsFSDrivesFolder())
    return;

  if (WindowFromPoint(pt) != (HWND)m_Panel->_listView)
    return;

  LVHITTESTINFO info;
  m_Panel->_listView.ScreenToClient(&pt);
  info.pt = pt;
  int index = ListView_HitTest(m_Panel->_listView, &info);
  if (index < 0)
    return;
  int realIndex = m_Panel->GetRealItemIndex(index);
  if (realIndex == kParentIndex)
    return;
  if (!m_Panel->IsItemFolder(realIndex))
    return; 
  m_SubFolderIndex = realIndex;
  m_SubFolderName = m_Panel->GetItemName(m_SubFolderIndex);
  MySetDropHighlighted(m_Panel->_listView, index, true);
  m_SelectionIndex = index;
}

bool CDropTarget::IsFsFolderPath() const
{
  if (!m_IsAppTarget && m_Panel != 0)
    return (m_Panel->IsFSFolder() || (m_Panel->IsFSDrivesFolder() && m_SelectionIndex >= 0));
  return false;
}

static void ReadUnicodeStrings(const wchar_t *p, size_t size, UStringVector &names)
{
  names.Clear();
  UString name;
  for (;size > 0; size--)
  {
    wchar_t c = *p++;
    if (c == 0)
    {
      if (name.IsEmpty())
        break;
      names.Add(name);
      name.Empty();
    }
    else
      name += c;
  }
}

static void ReadAnsiStrings(const char *p, size_t size, UStringVector &names)
{
  names.Clear();
  AString name;
  for (;size > 0; size--)
  {
    char c = *p++;
    if (c == 0)
    {
      if (name.IsEmpty())
        break;
      names.Add(GetUnicodeString(name));
      name.Empty();
    }
    else
      name += c;
  }
}

static void GetNamesFromDataObject(IDataObject *dataObject, UStringVector &names)
{
  names.Clear();
  FORMATETC etc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  STGMEDIUM medium;
  HRESULT res = dataObject->GetData(&etc, &medium);
  if (res != S_OK)
    return;
  if (medium.tymed != TYMED_HGLOBAL)
    return;
  {
    NMemory::CGlobal global;
    global.Attach(medium.hGlobal);
    size_t blockSize = GlobalSize(medium.hGlobal);
    NMemory::CGlobalLock dropLock(medium.hGlobal);
    const DROPFILES* dropFiles = (DROPFILES*)dropLock.GetPointer();
    if (dropFiles == 0)
      return;
    if (blockSize < dropFiles->pFiles)
      return;
    size_t size = blockSize - dropFiles->pFiles;
    const void *namesData = (const Byte *)dropFiles + dropFiles->pFiles;
    if (dropFiles->fWide)
      ReadUnicodeStrings((const wchar_t *)namesData, size / sizeof(wchar_t), names);
    else
      ReadAnsiStrings((const char *)namesData, size, names);
  }
}

bool CDropTarget::IsItSameDrive() const
{
  if (m_Panel == 0)
    return false;
  if (!IsFsFolderPath())
    return false;
  UString drive;
  if (m_Panel->IsFSFolder())
  {
    drive = m_Panel->GetDriveOrNetworkPrefix();
    if (drive.IsEmpty())
      return false;
  }
  else if (m_Panel->IsFSDrivesFolder() && m_SelectionIndex >= 0)
    drive = m_SubFolderName + L'\\';
  else
    return false;

  if (m_SourcePaths.Size() == 0)
    return false;
  for (int i = 0; i < m_SourcePaths.Size(); i++)
  {
    const UString &path = m_SourcePaths[i];
    if (drive.CompareNoCase(path.Left(drive.Length())) != 0)
      return false;
  }
  return true;

}

DWORD CDropTarget::GetEffect(DWORD keyState, POINTL /* pt */, DWORD allowedEffect)
{
  if (!m_DropIsAllowed || !m_PanelDropIsAllowed)
    return DROPEFFECT_NONE;

  if (!IsFsFolderPath())
    allowedEffect &= ~DROPEFFECT_MOVE;

  if (!m_SetPathIsOK)
    allowedEffect &= ~DROPEFFECT_MOVE;

  DWORD effect = 0;
  if (keyState & MK_CONTROL)
    effect = allowedEffect & DROPEFFECT_COPY;
  else if (keyState & MK_SHIFT)
    effect = allowedEffect & DROPEFFECT_MOVE;
  if(effect == 0)
  {
    if(allowedEffect & DROPEFFECT_COPY) 
    effect = DROPEFFECT_COPY;
    if(allowedEffect & DROPEFFECT_MOVE) 
    {
      if (IsItSameDrive())
        effect = DROPEFFECT_MOVE;
    }
  }
  if(effect == 0)
    return DROPEFFECT_NONE;
  return effect;
}

UString CDropTarget::GetTargetPath() const
{
  if (!IsFsFolderPath())
    return UString();
  UString path = m_Panel->_currentFolderPrefix;
  if (m_Panel->IsFSDrivesFolder())
    path.Empty();
  if (m_SubFolderIndex >= 0 && !m_SubFolderName.IsEmpty())
  {
    path += m_SubFolderName;
    path += L"\\";
  }
  return path;
}

bool CDropTarget::SetPath(bool enablePath) const
{
  UINT setFolderFormat = RegisterClipboardFormat(kSvenZipSetFolderFormat); 
  
  FORMATETC etc = { (CLIPFORMAT)setFolderFormat, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  STGMEDIUM medium;
  medium.tymed = etc.tymed;
  medium.pUnkForRelease = 0;
  UString path;
  if (enablePath)
    path = GetTargetPath();
  size_t size = path.Length() + 1;
  medium.hGlobal = GlobalAlloc(GHND | GMEM_SHARE, size * sizeof(wchar_t));
  if (medium.hGlobal == 0)
    return false;
  wchar_t *dest = (wchar_t *)GlobalLock(medium.hGlobal);
  if (dest == 0)
  {
    GlobalUnlock(medium.hGlobal);
    return false;
  }
  MyStringCopy(dest, (const wchar_t *)path);
  GlobalUnlock(medium.hGlobal);
  bool res = m_DataObject->SetData(&etc, &medium, FALSE) == S_OK;
  GlobalFree(medium.hGlobal);
  return res;
}

bool CDropTarget::SetPath()
{
  m_SetPathIsOK = SetPath(m_DropIsAllowed && m_PanelDropIsAllowed && IsFsFolderPath());
  return m_SetPathIsOK;
}

STDMETHODIMP CDropTarget::DragEnter(IDataObject * dataObject, DWORD keyState, 
      POINTL pt, DWORD *effect)
{
  GetNamesFromDataObject(dataObject, m_SourcePaths);
  QueryGetData(dataObject);
  m_DataObject = dataObject;
  return DragOver(keyState, pt, effect);
}


STDMETHODIMP CDropTarget::DragOver(DWORD keyState, POINTL pt, DWORD *effect)
{
  PositionCursor(pt);
  SetPath();
  *effect = GetEffect(keyState, pt, *effect);
  return S_OK;
}


STDMETHODIMP CDropTarget::DragLeave()
{
  RemoveSelection();
  SetPath(false);
  m_DataObject.Release();
  return S_OK;
}

// We suppose that there was ::DragOver for same POINTL_pt before ::Drop
// So SetPath() is same as in Drop.

STDMETHODIMP CDropTarget::Drop(IDataObject *dataObject, DWORD keyState, 
      POINTL pt, DWORD * effect)
{
  QueryGetData(dataObject);
  PositionCursor(pt);
  m_DataObject = dataObject;
  bool needDrop = true;
  if(m_DropIsAllowed && m_PanelDropIsAllowed)
    if (IsFsFolderPath())
      needDrop = !SetPath();
  *effect = GetEffect(keyState, pt, *effect);
  if(m_DropIsAllowed && m_PanelDropIsAllowed)
  {
    if (needDrop)
    {
      UString path = GetTargetPath();
      if (m_IsAppTarget && m_Panel)
        if (m_Panel->IsFSFolder())
          path = m_Panel->_currentFolderPrefix;
      m_Panel->DropObject(dataObject, path);
    }
  }
  RemoveSelection();
  m_DataObject.Release();
  return S_OK;
}

void CPanel::DropObject(IDataObject *dataObject, const UString &folderPath)
{
  UStringVector names;
  GetNamesFromDataObject(dataObject, names);
  CompressDropFiles(names, folderPath);
}

/*
void CPanel::CompressDropFiles(HDROP dr)
{
  UStringVector fileNames;
  {
    NShell::CDrop drop(true);
    drop.Attach(dr);
    drop.QueryFileNames(fileNames);
  }
  CompressDropFiles(fileNamesUnicode);
}
*/

static bool IsFolderInTemp(const UString &path)
{
  UString tempPath;
  if (!NFile::NDirectory::MyGetTempPath(tempPath))
    return false;
  if (tempPath.IsEmpty())
    return false;
  return (tempPath.CompareNoCase(path.Left(tempPath.Length())) == 0);
}

static bool AreThereNamesFromTemp(const UStringVector &fileNames)
{
  UString tempPath;
  if (!NFile::NDirectory::MyGetTempPath(tempPath))
    return false;
  if (tempPath.IsEmpty())
    return false;
  for (int i = 0; i < fileNames.Size(); i++)
    if (tempPath.CompareNoCase(fileNames[i].Left(tempPath.Length())) == 0)
      return true;
  return false;
}

void CPanel::CompressDropFiles(const UStringVector &fileNames, const UString &folderPath)
{
  if (fileNames.Size() == 0)
    return;
  const UString archiveName = CreateArchiveName(fileNames.Front(), 
      (fileNames.Size() > 1), false);
  bool createNewArchive = true;
  if (!IsFSFolder())
    createNewArchive = !DoesItSupportOperations();

  if (createNewArchive)
  {
    UString folderPath2 = folderPath;
    if (folderPath2.IsEmpty())
    {
      NFile::NDirectory::GetOnlyDirPrefix(fileNames.Front(), folderPath2);
      if (IsFolderInTemp(folderPath2))
        folderPath2 = L"C:\\"; // fix it
    }
    CompressFiles(folderPath2, archiveName, L"", fileNames, 
      false, // email
      true, // showDialog
      AreThereNamesFromTemp(fileNames) // waitFinish
      );
  }
  else
    CopyFromAsk(fileNames);
}
