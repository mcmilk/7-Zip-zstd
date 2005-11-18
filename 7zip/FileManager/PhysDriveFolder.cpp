// PhysDriveFolder.cpp

#include "StdAfx.h"

#include "PhysDriveFolder.h"

#include "Common/Alloc.h"

#include "Windows/PropVariant.h"
#include "Windows/FileDevice.h"
#include "Windows/FileSystem.h"

#include "../PropID.h"

using namespace NWindows;

static const UInt32 kBufferSize = (4 << 20);

static STATPROPSTG kProperties[] = 
{
  { NULL, kpidName, VT_BSTR},
  { NULL, kpidSize, VT_UI8}
};

CPhysDriveFolder::~CPhysDriveFolder()
{
  if (_buffer != 0)
    MyFree(_buffer);
}

HRESULT CPhysDriveFolder::Init(const UString &path)
{
  _prefix = L"\\\\.\\";
  _path = path;
  NFile::NDevice::CInFile inFile;
  if (!inFile.Open(GetFullPath()))
    return GetLastError();
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::LoadItems()
{
  _driveType = NFile::NSystem::MyGetDriveType(_path + L"\\");
  _name = _path.Left(1);
  _name += L'.';
  if (_driveType == DRIVE_CDROM)
    _name += L"iso";
  else
    _name += L"img";
  Int32 dummy;
  WasChanged(&dummy);
  return GetLength(_length);
}

STDMETHODIMP CPhysDriveFolder::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::GetProperty(UInt32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant propVariant;
  if (itemIndex >= 1)
    return E_INVALIDARG;
  switch(propID)
  {
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidName:
      propVariant = _name;
      break;
    case kpidSize:
      propVariant = _length;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)
  { return E_NOTIMPL; }

STDMETHODIMP CPhysDriveFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
  { return E_NOTIMPL; }

STDMETHODIMP CPhysDriveFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::GetName(BSTR *name)
  { return E_NOTIMPL; }

STDMETHODIMP CPhysDriveFolder::GetNumberOfProperties(UInt32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::GetPropertyInfo(UInt32 index,     
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


STDMETHODIMP CPhysDriveFolder::GetTypeID(BSTR *name)
{
  CMyComBSTR temp = L"PhysDrive";
  *name = temp.Detach();
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::GetPath(BSTR *path)
{
  UString tempPath = GetFullPath() + L"\\";
  CMyComBSTR temp = tempPath;
  *path = temp.Detach();
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::WasChanged(Int32 *wasChanged)
{
  bool wasChangedMain = false;
  *wasChanged = BoolToInt(wasChangedMain);
  return S_OK;
}
 
STDMETHODIMP CPhysDriveFolder::Clone(IFolderFolder **resultFolder)
{
  CPhysDriveFolder *folderSpec = new CPhysDriveFolder;
  CMyComPtr<IFolderFolder> folderNew = folderSpec;
  folderSpec->Init(_path);
  *resultFolder = folderNew.Detach();
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::GetItemFullSize(UInt32 index, PROPVARIANT *value, IProgress *progress)
{
  NCOM::CPropVariant propVariant;
  if (index >= 1)
    return E_INVALIDARG;
  UInt64 size = 0;
  HRESULT result = GetLength(size);
  propVariant = size;
  propVariant.Detach(value);
  return result;
}

STDMETHODIMP CPhysDriveFolder::CreateFolder(const wchar_t *name, IProgress *progress)
  { return E_NOTIMPL; }

STDMETHODIMP CPhysDriveFolder::CreateFile(const wchar_t *name, IProgress *progress)
  { return E_NOTIMPL; }

STDMETHODIMP CPhysDriveFolder::Rename(UInt32 index, const wchar_t *newName, IProgress *progress)
  { return E_NOTIMPL; }

STDMETHODIMP CPhysDriveFolder::Delete(const UInt32 *indices, UInt32 numItems, IProgress *progress)
  { return E_NOTIMPL; }

STDMETHODIMP CPhysDriveFolder::SetProperty(UInt32 index, PROPID propID, 
    const PROPVARIANT *value, IProgress *progress)
{
  if (index >= 1)
    return E_INVALIDARG;
  return E_NOTIMPL;
}

HRESULT CPhysDriveFolder::GetLength(UInt64 &length) const
{
  NFile::NDevice::CInFile inFile;
  if (!inFile.Open(GetFullPath()))
    return GetLastError();
  if (!inFile.GetLengthSmart(length))
    return GetLastError();
  return S_OK;
}

STDMETHODIMP CPhysDriveFolder::CopyTo(const UInt32 *indices, UInt32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  if (numItems == 0)
    return S_OK;
  UString destPath = path;
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool directName = (destPath[destPath.Length() - 1] != L'\\');
  if (directName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }
  else
    destPath += _name;

  UInt64 fileSize;
  if (GetLength(fileSize) == S_OK)
  {
    RINOK(callback->SetTotal(fileSize));
  }

  Int32 writeAskResult;
  CMyComBSTR destPathResult;
  RINOK(callback->AskWrite(GetFullPath(), BoolToInt(false), NULL, &fileSize, 
      destPath, &destPathResult, &writeAskResult));
  if (!IntToBool(writeAskResult))
    return S_OK;

  RINOK(callback->SetCurrentFilePath(GetFullPathWithName()));
  
  NFile::NDevice::CInFile inFile;
  if (!inFile.Open(GetFullPath()))
    return GetLastError();
  NFile::NIO::COutFile outFile;
  if (!outFile.Create(destPathResult, true))
    return GetLastError();
  if (_buffer == 0)
  {
    _buffer = MyAlloc(kBufferSize);
    if (_buffer == 0)
      return E_OUTOFMEMORY;
  }
  UInt64 pos = 0;
  UInt32 bufferSize = kBufferSize;
  if (_driveType == DRIVE_REMOVABLE)
    bufferSize = (18 << 10) * 4;
  pos = 0;
  while(pos < fileSize)
  {
    RINOK(callback->SetCompleted(&pos));
    UInt32 curSize = (UInt32)MyMin(fileSize - pos, (UInt64)bufferSize);
    UInt32 processedSize;
    if (!inFile.Read(_buffer, curSize, processedSize))
      return GetLastError();
    if (processedSize == 0)
      break;
    curSize = processedSize;
    if (!outFile.Write(_buffer, curSize, processedSize))
      return GetLastError();
    if (curSize != processedSize)
      return E_FAIL;
    pos += curSize;
  }
  return S_OK;
}

/////////////////////////////////////////////////
// Move Operations

STDMETHODIMP CPhysDriveFolder::MoveTo(
    const UInt32 *indices, 
    UInt32 numItems, 
    const wchar_t *path,
    IFolderOperationsExtractCallback *callback)
{
  return E_NOTIMPL;
}

STDMETHODIMP CPhysDriveFolder::CopyFrom(
    const wchar_t *fromFolderPath,
    const wchar_t **itemsPaths, UInt32 numItems, IProgress *progress)
{
  return E_NOTIMPL;
}
