// FSFolder.cpp

#include "StdAfx.h"

#include "FSFolder.h"

#include "Common/StringConvert.h"
#include "Common/StdInStream.h"
#include "Common/StdOutStream.h"
#include "Common/UTFConvert.h"

#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"
#include "Windows/FileIO.h"

#include "../PropID.h"

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
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidCreationTime, VT_FILETIME},
  { NULL, kpidLastAccessTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidComment, VT_BSTR}
};

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

HRESULT CFSFolder::Init(const UString &path, IFolderFolder *parentFolder)
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
    CFileInfoW fileInfo;
    if (!findFile.FindFirst(_path + UString(L"*"), fileInfo))
      return lastError;
    _findChangeNotificationDefined = false;
  }
  else
    _findChangeNotificationDefined = true;

  return S_OK;
}

static HRESULT GetFolderSize(const UString &path, UINT64 &size, IProgress *progress)
{
  RINOK(progress->SetCompleted(NULL));
  size = 0;
  CEnumeratorW enumerator(path + UString(L"\\*"));
  CFileInfoW fileInfo;
  while (enumerator.Next(fileInfo))
  {
    if (fileInfo.IsDirectory())
    {
      UINT64 subSize;
      RINOK(GetFolderSize(path + UString(L"\\") + fileInfo.Name, 
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
  CEnumeratorW enumerator(_path + L"*");
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
  _commentsAreLoaded = false;
  return S_OK;
}

bool CFSFolder::LoadComments()
{
  if (_commentsAreLoaded)
    return true;
  _comments.Clear();
  _commentsAreLoaded = true;
  CStdInStream file;
  if (!file.Open(GetSystemString(_path + L"descript.ion", _fileCodePage)))
    return false;
  AString string;
  file.ReadToString(string);
  file.Close();
  UString unicodeString;
  if (!ConvertUTF8ToUnicode(string, unicodeString))
    return false;
  return _comments.ReadFromString(unicodeString);
}

static bool IsAscii(const UString &testString)
{
  for (int i = 0; i < testString.Length(); i++)
    if (testString[i] >= 0x80)
      return false;
  return true;
}

bool CFSFolder::SaveComments()
{
  CStdOutStream file;
  if (!file.Open(GetSystemString(_path + L"descript.ion", _fileCodePage)))
    return false;
  UString unicodeString;
  _comments.SaveToString(unicodeString);
  AString utfString;
  ConvertUnicodeToUTF8(unicodeString, utfString);
  if (!IsAscii(unicodeString))
  {
    file << char(0xEF) << char(0xBB) << char(0xBF) << char('\n');
  }
  file << utfString;
  file.Close();
  _commentsAreLoaded = false;
  return true;
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
  if (itemIndex >= (UINT32)_files.Size())
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
            !MyGetCompressedFileSizeW(_path + fileInfo.Name, 
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
    case kpidComment:
      LoadComments();
      UString comment;
      if (_comments.GetValue(GetUnicodeString(fileInfo.Name), comment))
        propVariant = comment;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}

HRESULT CFSFolder::BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  CFSFolder *folderSpec = new CFSFolder;
  CMyComPtr<IFolderFolder> subFolder = folderSpec;
  RINOK(folderSpec ->Init(_path + name + UString(L'\\'), 0));
  *resultFolder = subFolder.Detach();
  return S_OK;
}


STDMETHODIMP CFSFolder::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  const NFind::CFileInfoW &fileInfo = _files[index];
  if (!fileInfo.IsDirectory())
    return E_INVALIDARG;
  return BindToFolderSpec(fileInfo.Name, resultFolder);
}

STDMETHODIMP CFSFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  return BindToFolderSpec(name, resultFolder);
}

STDMETHODIMP CFSFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  if (_parentFolder)
  {
    CMyComPtr<IFolderFolder> parentFolder = _parentFolder;
    *resultFolder = parentFolder.Detach();
    return S_OK;
  }
  if (_path.IsEmpty())
    return E_INVALIDARG;
  int pos = _path.ReverseFind(TEXT('\\'));
  if (pos < 0 || pos != _path.Length() - 1)
    return E_FAIL;
  UString parentPath = _path.Left(pos);
  pos = parentPath.ReverseFind(TEXT('\\'));
  if (pos < 0)
  {
    parentPath.Empty();
    CFSDrives *drivesFolderSpec = new CFSDrives;
    CMyComPtr<IFolderFolder> drivesFolder = drivesFolderSpec;
    drivesFolderSpec->Init();
    *resultFolder = drivesFolder.Detach();
    return S_OK;
  }
  UString parentPathReduced = parentPath.Left(pos);
  parentPath = parentPath.Left(pos + 1);
  pos = parentPathReduced.ReverseFind(TEXT('\\'));
  if (pos == 1)
  {
    if (parentPath[0] != TEXT('\\'))
      return E_FAIL;
    CNetFolder *netFolderSpec = new CNetFolder;
    CMyComPtr<IFolderFolder> netFolder = netFolderSpec;
    netFolderSpec->Init(GetUnicodeString(parentPath));
    *resultFolder = netFolder.Detach();
    return S_OK;
  }
  CFSFolder *parentFolderSpec = new CFSFolder;
  CMyComPtr<IFolderFolder> parentFolder = parentFolderSpec;
  RINOK(parentFolderSpec->Init(parentPath, 0));
  *resultFolder = parentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CFSFolder::GetName(BSTR *name)
{
  return E_NOTIMPL;
  /*
  CMyComBSTR aBSTRName = m_ProxyFolderItem->m_Name;
  *name = aBSTRName.Detach();
  return S_OK;
  */
}

STDMETHODIMP CFSFolder::GetNumberOfProperties(UINT32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CFSFolder::GetPropertyInfo(UINT32 index,     
    BSTR *name, PROPID *propID, VARTYPE *varType)
{
  if (index >= sizeof(kProperties) / sizeof(kProperties[0]))
    return E_INVALIDARG;
  const STATPROPSTG &prop = kProperties[index];
  *propID = prop.propid;
  *varType = prop.vt;
  *name = 0;
  return S_OK;
}


STDMETHODIMP CFSFolder::GetTypeID(BSTR *name)
{
  CMyComBSTR temp = L"FSFolder";
  *name = temp.Detach();
  return S_OK;
}

STDMETHODIMP CFSFolder::GetPath(BSTR *path)
{
  CMyComBSTR temp = GetUnicodeString(_path, _fileCodePage);
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
  CFSFolder *fsFolderSpec = new CFSFolder;
  CMyComPtr<IFolderFolder> folderNew = fsFolderSpec;
  fsFolderSpec->Init(_path, 0);
  *resultFolder = folderNew.Detach();
  return S_OK;
}

HRESULT CFSFolder::GetItemFullSize(int index, UINT64 &size, IProgress *progress)
{
  const CFileInfoW &fileInfo = _files[index];
  if (fileInfo.IsDirectory())
  {
    /*
    CMyComPtr<IFolderFolder> subFolder;
    RINOK(BindToFolder(index, &subFolder));
    CMyComPtr<IFolderReload> aFolderReload;
    subFolder.QueryInterface(&aFolderReload);
    aFolderReload->Reload();
    UINT32 numItems;
    RINOK(subFolder->GetNumberOfItems(&numItems));  
    CMyComPtr<IFolderGetItemFullSize> aGetItemFullSize;
    subFolder.QueryInterface(&aGetItemFullSize);
    for (UINT32 i = 0; i < numItems; i++)
    {
      UINT64 size;
      RINOK(aGetItemFullSize->GetItemFullSize(i, &size));
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
  if (index >= (UINT32)_files.Size())
    return E_INVALIDARG;
  UINT64 size = 0;
  HRESULT result = GetItemFullSize(index, size, progress);
  propVariant = size;
  propVariant.Detach(value);
  return result;
}

HRESULT CFSFolder::GetComplexName(const wchar_t *name, UString &resultPath)
{
  UString newName = name;
  resultPath = _path + newName;
  if (newName.Length() < 1)
    return S_OK;
  if (newName[0] == L'\\')
  {
    resultPath = newName;
    return S_OK;
  }
  if (newName.Length() < 2)
    return S_OK;
  if (newName[1] == L':')
    resultPath = newName;
  return S_OK;
}

STDMETHODIMP CFSFolder::CreateFolder(const wchar_t *name, IProgress *progress)
{
  UString processedName;
  RINOK(GetComplexName(name, processedName));
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
  UString processedName;
  RINOK(GetComplexName(name, processedName));
  NIO::COutFile outFile;
  if (!outFile.Create(processedName, false))
    return ::GetLastError();
  return S_OK;
}

STDMETHODIMP CFSFolder::Rename(UINT32 index, const wchar_t *newName, IProgress *progress)
{
  const CFileInfoW &fileInfo = _files[index];
  if (!NDirectory::MyMoveFile(_path + fileInfo.Name, _path + newName))
    return GetLastError();
  return S_OK;
}

STDMETHODIMP CFSFolder::Delete(const UINT32 *indices, UINT32 numItems,
    IProgress *progress)
{
  RINOK(progress->SetTotal(numItems));
  for (UINT32 i = 0; i < numItems; i++)
  {
    int index = indices[i];
    const CFileInfoW &fileInfo = _files[indices[i]];
    const UString fullPath = _path + fileInfo.Name;
    bool result;
    if (fileInfo.IsDirectory())
      result = NDirectory::RemoveDirectoryWithSubItems(fullPath);
    else
      result = NDirectory::DeleteFileAlways(fullPath);
    if (!result)
      return GetLastError();
    UINT64 completed = i;
    RINOK(progress->SetCompleted(&completed));
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::SetProperty(UINT32 index, PROPID propID, 
    const PROPVARIANT *value, IProgress *progress)
{
  if (index >= (UINT32)_files.Size())
    return E_INVALIDARG;
  CFileInfoEx &fileInfo = _files[index];
  switch(propID)
  {
    case kpidComment:
    {
      UString filename = GetUnicodeString(fileInfo.Name);
      filename.Trim();
      if (value->vt == VT_EMPTY)
        _comments.DeletePair(filename);
      else if (value->vt == VT_BSTR)
      {
        CTextPair pair;
        pair.ID = filename;
        pair.ID.Trim();
        pair.Value = value->bstrVal;
        pair.Value.Trim();
        if (pair.Value.IsEmpty())
          _comments.DeletePair(filename);
        else
          _comments.AddPair(pair);
      }
      else
        return E_INVALIDARG;
      SaveComments();
      break;
    }
    default:
      return E_NOTIMPL;
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::GetSystemIconIndex(UINT32 index, INT32 *iconIndex)
{
  if (index >= (UINT32)_files.Size())
    return E_INVALIDARG;
  const CFileInfoEx &fileInfo = _files[index];
  *iconIndex = 0;
  int iconIndexTemp;
  if (GetRealIconIndex(GetSystemString(_path + fileInfo.Name), 
      fileInfo.Attributes, iconIndexTemp) != 0)
  {
    *iconIndex = iconIndexTemp;
    return S_OK;
  }
  return GetLastError();
}

// static const LPCTSTR kInvalidFileChars = TEXT("\\/:*?\"<>|");

