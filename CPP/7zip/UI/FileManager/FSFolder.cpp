// FSFolder.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Common/UTFConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileIO.h"
#include "Windows/PropVariant.h"

#include "../../PropID.h"

#include "FSDrives.h"
#include "FSFolder.h"

#ifndef UNDER_CE
#include "NetFolder.h"
#endif

#include "SysIconUtils.h"

namespace NWindows {
namespace NFile {

bool GetLongPath(LPCWSTR path, UString &longPath);

}}

using namespace NWindows;
using namespace NFile;
using namespace NFind;

namespace NFsFolder {

static STATPROPSTG kProps[] =
{
  { NULL, kpidName, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME},
  { NULL, kpidAttrib, VT_UI4},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidPrefix, VT_BSTR}
};

HRESULT CFSFolder::Init(const UString &path, IFolderFolder *parentFolder)
{
  _parentFolder = parentFolder;
  _path = path;

  _findChangeNotification.FindFirst(_path, false,
      FILE_NOTIFY_CHANGE_FILE_NAME |
      FILE_NOTIFY_CHANGE_DIR_NAME |
      FILE_NOTIFY_CHANGE_ATTRIBUTES |
      FILE_NOTIFY_CHANGE_SIZE |
      FILE_NOTIFY_CHANGE_LAST_WRITE /*|
      FILE_NOTIFY_CHANGE_LAST_ACCESS |
      FILE_NOTIFY_CHANGE_CREATION |
      FILE_NOTIFY_CHANGE_SECURITY */);
  if (!_findChangeNotification.IsHandleAllocated())
  {
    DWORD lastError = GetLastError();
    CFindFile findFile;
    CFileInfoW fi;
    if (!findFile.FindFirst(_path + UString(L"*"), fi))
      return lastError;
  }
  return S_OK;
}

HRESULT GetFolderSize(const UString &path, UInt64 &numFolders, UInt64 &numFiles, UInt64 &size, IProgress *progress)
{
  RINOK(progress->SetCompleted(NULL));
  numFiles = numFolders = size = 0;
  CEnumeratorW enumerator(path + UString(WSTRING_PATH_SEPARATOR L"*"));
  CFileInfoW fi;
  while (enumerator.Next(fi))
  {
    if (fi.IsDir())
    {
      UInt64 subFolders, subFiles, subSize;
      RINOK(GetFolderSize(path + UString(WCHAR_PATH_SEPARATOR) + fi.Name, subFolders, subFiles, subSize, progress));
      numFolders += subFolders;
      numFolders++;
      numFiles += subFiles;
      size += subSize;
    }
    else
    {
      numFiles++;
      size += fi.Size;
    }
  }
  return S_OK;
}

HRESULT CFSFolder::LoadSubItems(CDirItem &dirItem, const UString &path)
{
  {
    CEnumeratorW enumerator(path + L"*");
    CDirItem fi;
    while (enumerator.Next(fi))
    {
      #ifndef UNDER_CE
      fi.CompressedSizeIsDefined = false;
      /*
      if (!GetCompressedFileSize(_path + fi.Name,
      fi.CompressedSize))
      fi.CompressedSize = fi.Size;
      */
      #endif
      if (fi.IsDir())
      {
        // fi.Size = GetFolderSize(_path + fi.Name);
        fi.Size = 0;
      }
      dirItem.Files.Add(fi);
    }
  }
  if (!_flatMode)
    return S_OK;

  for (int i = 0; i < dirItem.Files.Size(); i++)
  {
    CDirItem &item = dirItem.Files[i];
    if (item.IsDir())
      LoadSubItems(item, path + item.Name + WCHAR_PATH_SEPARATOR);
  }
  return S_OK;
}

void CFSFolder::AddRefs(CDirItem &dirItem)
{
  int i;
  for (i = 0; i < dirItem.Files.Size(); i++)
  {
    CDirItem &item = dirItem.Files[i];
    item.Parent = &dirItem;
    _refs.Add(&item);
  }
  if (!_flatMode)
    return;
  for (i = 0; i < dirItem.Files.Size(); i++)
  {
    CDirItem &item = dirItem.Files[i];
    if (item.IsDir())
      AddRefs(item);
  }
}

STDMETHODIMP CFSFolder::LoadItems()
{
  // OutputDebugString(TEXT("Start\n"));
  Int32 dummy;
  WasChanged(&dummy);
  Clear();
  RINOK(LoadSubItems(_root, _path));
  AddRefs(_root);

  // OutputDebugString(TEXT("Finish\n"));
  _commentsAreLoaded = false;
  return S_OK;
}

static const wchar_t *kDescriptionFileName = L"descript.ion";

bool CFSFolder::LoadComments()
{
  if (_commentsAreLoaded)
    return true;
  _comments.Clear();
  _commentsAreLoaded = true;
  NIO::CInFile file;
  if (!file.Open(_path + kDescriptionFileName))
    return false;
  UInt64 length;
  if (!file.GetLength(length))
    return false;
  if (length >= (1 << 28))
    return false;
  AString s;
  char *p = s.GetBuffer((int)((size_t)length + 1));
  UInt32 processedSize;
  file.Read(p, (UInt32)length, processedSize);
  p[length] = 0;
  s.ReleaseBuffer();
  if (processedSize != length)
    return false;
  file.Close();
  UString unicodeString;
  if (!ConvertUTF8ToUnicode(s, unicodeString))
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
  NIO::COutFile file;
  if (!file.Create(_path + kDescriptionFileName, true))
    return false;
  UString unicodeString;
  _comments.SaveToString(unicodeString);
  AString utfString;
  ConvertUnicodeToUTF8(unicodeString, utfString);
  UInt32 processedSize;
  if (!IsAscii(unicodeString))
  {
    Byte bom [] = { 0xEF, 0xBB, 0xBF, 0x0D, 0x0A };
    file.Write(bom , sizeof(bom), processedSize);
  }
  file.Write(utfString, utfString.Length(), processedSize);
  _commentsAreLoaded = false;
  return true;
}

STDMETHODIMP CFSFolder::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _refs.Size();
  return S_OK;
}

/*
STDMETHODIMP CFSFolder::GetNumberOfSubFolders(UInt32 *numSubFolders)
{
  UInt32 numSubFoldersLoc = 0;
  for (int i = 0; i < _files.Size(); i++)
    if (_files[i].IsDir())
      numSubFoldersLoc++;
  *numSubFolders = numSubFoldersLoc;
  return S_OK;
}
*/

#ifndef UNDER_CE
static bool MyGetCompressedFileSizeW(LPCWSTR fileName, UInt64 &size)
{
  DWORD highPart;
  DWORD lowPart = ::GetCompressedFileSizeW(fileName, &highPart);
  if (lowPart == INVALID_FILE_SIZE && ::GetLastError() != NO_ERROR)
  {
    #ifdef WIN_LONG_PATH
    {
      UString longPath;
      if (GetLongPath(fileName, longPath))
        lowPart = ::GetCompressedFileSizeW(longPath, &highPart);
    }
    #endif
    if (lowPart == INVALID_FILE_SIZE && ::GetLastError() != NO_ERROR)
      return false;
  }
  size = (UInt64(highPart) << 32) | lowPart;
  return true;
}
#endif

STDMETHODIMP CFSFolder::GetProperty(UInt32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  if (itemIndex >= (UInt32)_refs.Size())
    return E_INVALIDARG;
  CDirItem &fi = *_refs[itemIndex];
  switch(propID)
  {
    case kpidIsDir: prop = fi.IsDir(); break;
    case kpidName: prop = fi.Name; break;
    case kpidSize: if (!fi.IsDir()) prop = fi.Size; break;
    case kpidPackSize:
      #ifdef UNDER_CE
      prop = fi.Size;
      #else
      if (!fi.CompressedSizeIsDefined)
      {
        fi.CompressedSizeIsDefined = true;
        if (fi.IsDir () ||
            !MyGetCompressedFileSizeW(_path + GetRelPath(fi), fi.CompressedSize))
          fi.CompressedSize = fi.Size;
      }
      prop = fi.CompressedSize;
      #endif
      break;
    case kpidAttrib: prop = (UInt32)fi.Attrib; break;
    case kpidCTime: prop = fi.CTime; break;
    case kpidATime: prop = fi.ATime; break;
    case kpidMTime: prop = fi.MTime; break;
    case kpidComment:
    {
      LoadComments();
      UString comment;
      if (_comments.GetValue(GetRelPath(fi), comment))
        prop = comment;
      break;
    }
    case kpidPrefix:
    {
      if (_flatMode)
        prop = GetPrefix(fi);
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT CFSFolder::BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  CFSFolder *folderSpec = new CFSFolder;
  CMyComPtr<IFolderFolder> subFolder = folderSpec;
  RINOK(folderSpec->Init(_path + name + UString(WCHAR_PATH_SEPARATOR), 0));
  *resultFolder = subFolder.Detach();
  return S_OK;
}

UString CFSFolder::GetPrefix(const CDirItem &item) const
{
  UString path;
  CDirItem *cur = item.Parent;
  while (cur->Parent != 0)
  {
    path = cur->Name + UString(WCHAR_PATH_SEPARATOR) + path;
    cur = cur->Parent;
  }
  return path;
}

UString CFSFolder::GetRelPath(const CDirItem &item) const
{
  return GetPrefix(item) + item.Name;
}

STDMETHODIMP CFSFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  const CDirItem &fi = *_refs[index];
  if (!fi.IsDir())
    return E_INVALIDARG;
  return BindToFolderSpec(GetRelPath(fi), resultFolder);
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
  int pos = _path.ReverseFind(WCHAR_PATH_SEPARATOR);
  if (pos < 0 || pos != _path.Length() - 1)
    return E_FAIL;
  UString parentPath = _path.Left(pos);
  pos = parentPath.ReverseFind(WCHAR_PATH_SEPARATOR);
  if (pos < 0)
  {
    #ifdef UNDER_CE
    *resultFolder = 0;
    #else
    CFSDrives *drivesFolderSpec = new CFSDrives;
    CMyComPtr<IFolderFolder> drivesFolder = drivesFolderSpec;
    drivesFolderSpec->Init();
    *resultFolder = drivesFolder.Detach();
    #endif
    return S_OK;
  }
  UString parentPathReduced = parentPath.Left(pos);
  parentPath = parentPath.Left(pos + 1);
  #ifndef UNDER_CE
  pos = parentPathReduced.ReverseFind(WCHAR_PATH_SEPARATOR);
  if (pos == 1)
  {
    if (parentPath[0] != WCHAR_PATH_SEPARATOR)
      return E_FAIL;
    CNetFolder *netFolderSpec = new CNetFolder;
    CMyComPtr<IFolderFolder> netFolder = netFolderSpec;
    netFolderSpec->Init(parentPath);
    *resultFolder = netFolder.Detach();
    return S_OK;
  }
  #endif
  CFSFolder *parentFolderSpec = new CFSFolder;
  CMyComPtr<IFolderFolder> parentFolder = parentFolderSpec;
  RINOK(parentFolderSpec->Init(parentPath, 0));
  *resultFolder = parentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CFSFolder::GetNumberOfProperties(UInt32 *numProperties)
{
  *numProperties = sizeof(kProps) / sizeof(kProps[0]);
  if (!_flatMode)
    (*numProperties)--;
  return S_OK;
}

STDMETHODIMP CFSFolder::GetPropertyInfo IMP_IFolderFolder_GetProp(kProps)

STDMETHODIMP CFSFolder::GetFolderProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidType: prop = L"FSFolder"; break;
    case kpidPath: prop = _path; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CFSFolder::WasChanged(Int32 *wasChanged)
{
  bool wasChangedMain = false;
  for (;;)
  {
    if (!_findChangeNotification.IsHandleAllocated())
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

HRESULT CFSFolder::GetItemsFullSize(const UInt32 *indices, UInt32 numItems,
    UInt64 &numFolders, UInt64 &numFiles, UInt64 &size, IProgress *progress)
{
  numFiles = numFolders = size = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    int index = indices[i];
    if (index >= _refs.Size())
      return E_INVALIDARG;
    const CDirItem &fi = *_refs[index];
    if (fi.IsDir())
    {
      UInt64 subFolders, subFiles, subSize;
      RINOK(GetFolderSize(_path + GetRelPath(fi), subFolders, subFiles, subSize, progress));
      numFolders += subFolders;
      numFolders++;
      numFiles += subFiles;
      size += subSize;
    }
    else
    {
      numFiles++;
      size += fi.Size;
    }
  }
  return S_OK;
}

HRESULT CFSFolder::GetItemFullSize(int index, UInt64 &size, IProgress *progress)
{
  const CDirItem &fi = *_refs[index];
  if (fi.IsDir())
  {
    /*
    CMyComPtr<IFolderFolder> subFolder;
    RINOK(BindToFolder(index, &subFolder));
    CMyComPtr<IFolderReload> aFolderReload;
    subFolder.QueryInterface(&aFolderReload);
    aFolderReload->Reload();
    UInt32 numItems;
    RINOK(subFolder->GetNumberOfItems(&numItems));
    CMyComPtr<IFolderGetItemFullSize> aGetItemFullSize;
    subFolder.QueryInterface(&aGetItemFullSize);
    for (UInt32 i = 0; i < numItems; i++)
    {
      UInt64 size;
      RINOK(aGetItemFullSize->GetItemFullSize(i, &size));
      *totalSize += size;
    }
    */
    UInt64 numFolders, numFiles;
    return GetFolderSize(_path + GetRelPath(fi), numFolders, numFiles, size, progress);
  }
  size = fi.Size;
  return S_OK;
}

STDMETHODIMP CFSFolder::GetItemFullSize(UInt32 index, PROPVARIANT *value, IProgress *progress)
{
  NCOM::CPropVariant prop;
  if (index >= (UInt32)_refs.Size())
    return E_INVALIDARG;
  UInt64 size = 0;
  HRESULT result = GetItemFullSize(index, size, progress);
  prop = size;
  prop.Detach(value);
  return result;
}

HRESULT CFSFolder::GetComplexName(const wchar_t *name, UString &resultPath)
{
  UString newName = name;
  resultPath = _path + newName;
  if (newName.Length() < 1)
    return S_OK;
  if (newName[0] == WCHAR_PATH_SEPARATOR)
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

STDMETHODIMP CFSFolder::CreateFolder(const wchar_t *name, IProgress * /* progress */)
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

STDMETHODIMP CFSFolder::CreateFile(const wchar_t *name, IProgress * /* progress */)
{
  UString processedName;
  RINOK(GetComplexName(name, processedName));
  NIO::COutFile outFile;
  if (!outFile.Create(processedName, false))
    return ::GetLastError();
  return S_OK;
}

STDMETHODIMP CFSFolder::Rename(UInt32 index, const wchar_t *newName, IProgress * /* progress */)
{
  const CDirItem &fi = *_refs[index];
  const UString fullPrefix = _path + GetPrefix(fi);
  if (!NDirectory::MyMoveFile(fullPrefix + fi.Name, fullPrefix + newName))
    return GetLastError();
  return S_OK;
}

STDMETHODIMP CFSFolder::Delete(const UInt32 *indices, UInt32 numItems,IProgress *progress)
{
  RINOK(progress->SetTotal(numItems));
  for (UInt32 i = 0; i < numItems; i++)
  {
    const CDirItem &fi = *_refs[indices[i]];
    const UString fullPath = _path + GetRelPath(fi);
    bool result;
    if (fi.IsDir())
      result = NDirectory::RemoveDirectoryWithSubItems(fullPath);
    else
      result = NDirectory::DeleteFileAlways(fullPath);
    if (!result)
      return GetLastError();
    UInt64 completed = i;
    RINOK(progress->SetCompleted(&completed));
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::SetProperty(UInt32 index, PROPID propID,
    const PROPVARIANT *value, IProgress * /* progress */)
{
  if (index >= (UInt32)_refs.Size())
    return E_INVALIDARG;
  CDirItem &fi = *_refs[index];
  if (fi.Parent->Parent != 0)
    return E_NOTIMPL;
  switch(propID)
  {
    case kpidComment:
    {
      UString filename = fi.Name;
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

STDMETHODIMP CFSFolder::GetSystemIconIndex(UInt32 index, Int32 *iconIndex)
{
  if (index >= (UInt32)_refs.Size())
    return E_INVALIDARG;
  const CDirItem &fi = *_refs[index];
  *iconIndex = 0;
  int iconIndexTemp;
  if (GetRealIconIndex(_path + GetRelPath(fi), fi.Attrib, iconIndexTemp) != 0)
  {
    *iconIndex = iconIndexTemp;
    return S_OK;
  }
  return GetLastError();
}

STDMETHODIMP CFSFolder::SetFlatMode(Int32 flatMode)
{
  _flatMode = IntToBool(flatMode);
  return S_OK;
}

}
