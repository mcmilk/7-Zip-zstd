// OpenCallback.cpp

#include "StdAfx.h"

#include "OpenCallback.h"

#include "Common/StringConvert.h"
#include "Resource/PasswordDialog/PasswordDialog.h"
#include "Windows/PropVariant.h"
#include "../Common/FileStreams.h"

STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 *numFiles, const UINT64 *numBytes)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 *numFiles, const UINT64 *numBytes)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 total)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 *completed)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
  switch(propID)
  {
  case kpidName:
    propVariant = GetUnicodeString(_fileInfo.Name);
    break;
  case kpidIsFolder:
    propVariant = _fileInfo.IsDirectory();
    break;
  case kpidSize:
    propVariant = _fileInfo.Size;
    break;
  case kpidAttributes:
    propVariant = (UINT32)_fileInfo.Attributes;
    break;
  case kpidLastAccessTime:
    propVariant = _fileInfo.LastAccessTime;
    break;
  case kpidCreationTime:
    propVariant = _fileInfo.CreationTime;
    break;
  case kpidLastWriteTime:
    propVariant = _fileInfo.LastWriteTime;
    break;
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetStream(const wchar_t *name, 
    IInStream **inStream)
{
  *inStream = NULL;
  NWindows::NFile::NFind::CFileInfo fileInfo;

  CSysString fullPath = _folderPrefix + GetSystemString(name);
  if (!NWindows::NFile::NFind::FindFile(fullPath, fileInfo))
    return S_FALSE;
  _fileInfo = fileInfo;
  if (_fileInfo.IsDirectory())
    return S_FALSE;
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<IInStream> inStreamTemp = inFile;
  if (!inFile->Open(fullPath))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!_passwordIsDefined)
  {
    CPasswordDialog dialog;
   
    if (dialog.Create(_parentWindow) == IDCANCEL)
      return E_ABORT;

    _password = GetUnicodeString((LPCTSTR)dialog._password);
    _passwordIsDefined = true;
  }
  CMyComBSTR tempName = _password;
  *password = tempName.Detach();

  return S_OK;
}
