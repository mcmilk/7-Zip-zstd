// FSFolder.cpp

#include "StdAfx.h"

#include "FSFolder.h"

#include "Common/StringConvert.h"
#include "Interface/PropID.h"
#include "Interface/EnumStatProp.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"
#include "Windows/FileIO.h"

#include "SysIconUtils.h"
#include "FSDrives.h"
#include "NetFolder.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

static STATPROPSTG kProperties[] = 
{
  { NULL, kpidName, VT_BSTR},
  // { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidCreationTime, VT_FILETIME},
  { NULL, kpidLastAccessTime, VT_FILETIME},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4}
};

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

HRESULT CFSFolder::Init(const CSysString &path, IFolderFolder *parentFolder)
{
  _parentFolder = parentFolder;
  _path = path;
  _fileCodePage = GetCurrentFileCodePage();

  if (_findChangeNotification.FindFirst(_path, false, 
      FILE_NOTIFY_CHANGE_FILE_NAME | 
      FILE_NOTIFY_CHANGE_DIR_NAME |
      FILE_NOTIFY_CHANGE_ATTRIBUTES |
      FILE_NOTIFY_CHANGE_SIZE |
      FILE_NOTIFY_CHANGE_LAST_WRITE /*|
      FILE_NOTIFY_CHANGE_LAST_ACCESS |
      FILE_NOTIFY_CHANGE_CREATION |
      FILE_NOTIFY_CHANGE_SECURITY */) == INVALID_HANDLE_VALUE)
  {
    DWORD lastError = GetLastError();
    // return GetLastError();
    CFindFile findFile;
    CFileInfo fileInfo;
    if (!findFile.FindFirst(_path + TEXT("*"), fileInfo))
      return lastError;
    _findChangeNotificationDefined = false;
  }
  else
    _findChangeNotificationDefined = true;

  return S_OK;
}

static HRESULT GetFolderSize(const CSysString &path, UINT64 &size, IProgress *progress)
{
  RETURN_IF_NOT_S_OK(progress->SetCompleted(NULL));
  size = 0;
  CEnumerator enumerator(path + CSysString(TEXT("\\*")));
  CFileInfo fileInfo;
  while (enumerator.Next(fileInfo))
  {
    if (fileInfo.IsDirectory())
    {
      UINT64 subSize;
      RETURN_IF_NOT_S_OK(GetFolderSize(path + CSysString(TEXT("\\")) + fileInfo.Name, 
          subSize, progress));
      size += subSize;
    }
    else
      size += fileInfo.Size;
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::LoadItems()
{
  // OutputDebugString(TEXT("Start\n"));
  INT32 dummy;
  WasChanged(&dummy);
  _files.Clear();
  CEnumerator enumerator(_path + TEXT("*"));
  CFileInfoEx fileInfo;
  while (enumerator.Next(fileInfo))
  {
    fileInfo.CompressedSizeIsDefined = false;
    /*
    if (!GetCompressedFileSize(_path + fileInfo.Name, 
        fileInfo.CompressedSize))
      fileInfo.CompressedSize = fileInfo.Size;
    */
    if (fileInfo.IsDirectory())
    {
      // fileInfo.Size = GetFolderSize(_path + fileInfo.Name);
      fileInfo.Size = 0;
    }
    _files.Add(fileInfo);
  }
  // OutputDebugString(TEXT("Finish\n"));
  return S_OK;
}

STDMETHODIMP CFSFolder::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _files.Size();
  return S_OK;
}

/*
STDMETHODIMP CFSFolder::GetNumberOfSubFolders(UINT32 *aNumSubFolders)
{
  UINT32 aNumSubFoldersLoc = 0;
  for (int i = 0; i < _files.Size(); i++)
    if (_files[i].IsDirectory())
      aNumSubFoldersLoc++;
  *aNumSubFolders = aNumSubFoldersLoc;
  return S_OK;
}
*/

STDMETHODIMP CFSFolder::GetProperty(UINT32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant propVariant;
  if (itemIndex >= _files.Size())
    return E_INVALIDARG;
  CFileInfoEx &fileInfo = _files[itemIndex];
  switch(propID)
  {
    case kpidIsFolder:
      propVariant = fileInfo.IsDirectory();
      break;
    case kpidName:
      propVariant = GetUnicodeString(fileInfo.Name);
      break;
    case kpidSize:
      propVariant = fileInfo.Size;
      break;
    case kpidPackedSize:
      if (!fileInfo.CompressedSizeIsDefined)
      {
        fileInfo.CompressedSizeIsDefined = true;
        if (fileInfo.IsDirectory () || 
            !GetCompressedFileSize(_path + fileInfo.Name, 
                fileInfo.CompressedSize))
          fileInfo.CompressedSize = fileInfo.Size;
      }
      propVariant = fileInfo.CompressedSize;
      break;
    case kpidAttributes:
      propVariant = (UINT32)fileInfo.Attributes;
      break;
    case kpidCreationTime:
      propVariant = fileInfo.CreationTime;
      break;
    case kpidLastAccessTime:
      propVariant = fileInfo.LastAccessTime;
      break;
    case kpidLastWriteTime:
      propVariant = fileInfo.LastWriteTime;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}

HRESULT CFSFolder::BindToFolderSpec(const TCHAR *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  CComObjectNoLock<CFSFolder> *folderSpec = new CComObjectNoLock<CFSFolder>;
  CComPtr<IFolderFolder> aSubFolder = folderSpec;
  RETURN_IF_NOT_S_OK(folderSpec ->Init(_path + name + TEXT('\\'), 0));
  *resultFolder = aSubFolder.Detach();
  return S_OK;
}


STDMETHODIMP CFSFolder::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  const NFind::CFileInfo &fileInfo = _files[index];
  if (!fileInfo.IsDirectory())
    return E_INVALIDARG;
  return BindToFolderSpec(fileInfo.Name, resultFolder);
}

STDMETHODIMP CFSFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  return BindToFolderSpec(GetSystemString(name, GetCurrentFileCodePage()), resultFolder);
}

STDMETHODIMP CFSFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  if (_parentFolder)
  {
    CComPtr<IFolderFolder> parentFolder = _parentFolder;
    *resultFolder = parentFolder.Detach();
    return S_OK;
  }
  if (_path.IsEmpty())
    return E_INVALIDARG;
  int pos = _path.ReverseFind(TEXT('\\'));
  if (pos < 0 || pos != _path.Length() - 1)
    return E_FAIL;
  CSysString parrentPath = _path.Left(pos);
  pos = parrentPath.ReverseFind(TEXT('\\'));
  if (pos < 0)
  {
    parrentPath.Empty();
    CComObjectNoLock<CFSDrives> *drivesFolderSpec = new 
      CComObjectNoLock<CFSDrives>;
    CComPtr<IFolderFolder> drivesFolder = drivesFolderSpec;
    drivesFolderSpec->Init();
    *resultFolder = drivesFolder.Detach();
    return S_OK;
  }
  CSysString parrentPathReduced = parrentPath.Left(pos);
  parrentPath = parrentPath.Left(pos + 1);
  pos = parrentPathReduced.ReverseFind(TEXT('\\'));
  if (pos == 1)
  {
    if (parrentPath[0] != TEXT('\\'))
      return E_FAIL;
    CComObjectNoLock<CNetFolder> *netFolderSpec = new CComObjectNoLock<CNetFolder>;
    CComPtr<IFolderFolder> netFolder = netFolderSpec;
    netFolderSpec->Init(GetUnicodeString(parrentPath));
    *resultFolder = netFolder.Detach();
    return S_OK;
  }
  CComObjectNoLock<CFSFolder> *parentFolderSpec = new  CComObjectNoLock<CFSFolder>;
  CComPtr<IFolderFolder> parentFolder = parentFolderSpec;
  RETURN_IF_NOT_S_OK(parentFolderSpec->Init(parrentPath, 0));
  *resultFolder = parentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CFSFolder::GetName(BSTR *name)
{
  return E_NOTIMPL;
  /*
  CComBSTR aBSTRName = m_ProxyFolderItem->m_Name;
  *name = aBSTRName.Detach();
  return S_OK;
  */
}

STDMETHODIMP CFSFolder::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
}

STDMETHODIMP CFSFolder::GetTypeID(BSTR *name)
{
  CComBSTR temp = L"FSFolder";
  *name = temp.Detach();
  return S_OK;
}

STDMETHODIMP CFSFolder::GetPath(BSTR *path)
{
  CComBSTR temp = GetUnicodeString(_path, _fileCodePage);
  *path = temp.Detach();
  return S_OK;
}


STDMETHODIMP CFSFolder::WasChanged(INT32 *wasChanged)
{
  bool wasChangedMain = false;
  while (true)
  {
    if (!_findChangeNotificationDefined)
    {
      *wasChanged = BoolToInt(false);
      return S_OK;
    }

    DWORD waitResult = ::WaitForSingleObject(_findChangeNotification, 0);
    bool wasChangedLoc = (waitResult == WAIT_OBJECT_0);
    if (wasChangedLoc)
    {
      _findChangeNotification.FindNext();
      wasChangedMain = true;
    }
    else 
      break;
  }
  *wasChanged = BoolToInt(wasChangedMain);
  return S_OK;
}
 
STDMETHODIMP CFSFolder::Clone(IFolderFolder **resultFolder)
{
  CComObjectNoLock<CFSFolder> *fsFolderSpec = new  CComObjectNoLock<CFSFolder>;
  CComPtr<IFolderFolder> folderNew = fsFolderSpec;
  fsFolderSpec->Init(_path, 0);
  *resultFolder = folderNew.Detach();
  return S_OK;
}

HRESULT CFSFolder::GetItemFullSize(int index, UINT64 &size, IProgress *progress)
{
  const CFileInfo &fileInfo = _files[index];
  if (fileInfo.IsDirectory())
  {
    /*
    CComPtr<IFolderFolder> aSubFolder;
    RETURN_IF_NOT_S_OK(BindToFolder(index, &aSubFolder));
    CComPtr<IFolderReload> aFolderReload;
    aSubFolder.QueryInterface(&aFolderReload);
    aFolderReload->Reload();
    UINT32 numItems;
    RETURN_IF_NOT_S_OK(aSubFolder->GetNumberOfItems(&numItems));  
    CComPtr<IFolderGetItemFullSize> aGetItemFullSize;
    aSubFolder.QueryInterface(&aGetItemFullSize);
    for (UINT32 i = 0; i < numItems; i++)
    {
      UINT64 size;
      RETURN_IF_NOT_S_OK(aGetItemFullSize->GetItemFullSize(i, &size));
      *totalSize += size;
    }
    */
    return GetFolderSize(_path + fileInfo.Name, size, progress);
  }
  size = fileInfo.Size;
  return S_OK;
}

STDMETHODIMP CFSFolder::GetItemFullSize(UINT32 index, PROPVARIANT *value, IProgress *progress)
{
  NCOM::CPropVariant propVariant;
  if (index >= _files.Size())
    return E_INVALIDARG;
  UINT64 size = 0;
  HRESULT result = GetItemFullSize(index, size, progress);
  propVariant = size;
  propVariant.Detach(value);
  return result;
}

HRESULT CFSFolder::GetComplexName(const wchar_t *name, CSysString &resultPath)
{
  UString newName = name;
  resultPath = _path + GetSystemString(newName, _fileCodePage);
  if (newName.Length() < 1)
    return S_OK;
  if (newName[0] == L'\\')
  {
    resultPath = GetSystemString(newName, _fileCodePage);
    return S_OK;
  }
  if (newName.Length() < 2)
    return S_OK;
  if (newName[1] == L':')
    resultPath = GetSystemString(newName, _fileCodePage);
  return S_OK;
}

STDMETHODIMP CFSFolder::CreateFolder(const wchar_t *name, IProgress *progress)
{
  CSysString processedName;
  RETURN_IF_NOT_S_OK(GetComplexName(name, processedName));
  if(NDirectory::MyCreateDirectory(processedName))
    return S_OK;
  if(::GetLastError() == ERROR_ALREADY_EXISTS)
    return ::GetLastError();
  if (!NDirectory::CreateComplexDirectory(processedName))
    return ::GetLastError();
  return S_OK;
}

STDMETHODIMP CFSFolder::CreateFile(const wchar_t *name, IProgress *progress)
{
  CSysString processedName;
  RETURN_IF_NOT_S_OK(GetComplexName(name, processedName));
  NIO::COutFile outFile;
  if (!outFile.Open(processedName))
    return ::GetLastError();
  return S_OK;
}

STDMETHODIMP CFSFolder::Rename(UINT32 index, const wchar_t *newName, IProgress *progress)
{
  const CFileInfo &fileInfo = _files[index];
  if (!::MoveFile(_path + fileInfo.Name, _path + GetSystemString(newName, _fileCodePage)))
    return GetLastError();
  return S_OK;
}

STDMETHODIMP CFSFolder::Delete(const UINT32 *indices, UINT32 numItems,
    IProgress *progress)
{
  RETURN_IF_NOT_S_OK(progress->SetTotal(numItems));
  for (UINT32 i = 0; i < numItems; i++)
  {
    int index = indices[i];
    const CFileInfo &fileInfo = _files[indices[i]];
    const CSysString fullPath = _path + fileInfo.Name;
    bool result;
    if (fileInfo.IsDirectory())
      result = NDirectory::RemoveDirectoryWithSubItems(fullPath);
    else
      result = NDirectory::DeleteFileAlways(fullPath);
    if (!result)
      return GetLastError();
    UINT64 completed = i;
    RETURN_IF_NOT_S_OK(progress->SetCompleted(&completed));
  }
  return S_OK;
}

/*
STDMETHODIMP CFSFolder::GetSystemIconIndex(UINT32 index, INT32 *iconIndex)
{
  if (index >= _files.Size())
    return E_INVALIDARG;
  const CFileInfoEx &fileInfo = _files[index];
  *iconIndex = GetRealIconIndex(fileInfo.Attributes, _path + fileInfo.Name);
  return S_OK;
}
*/
// static const LPCTSTR kInvalidFileChars = TEXT("\\/:*?\"<>|");

