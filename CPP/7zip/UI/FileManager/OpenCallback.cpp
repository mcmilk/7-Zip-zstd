// OpenCallback.cpp

#include "StdAfx.h"

#include "OpenCallback.h"

#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

#include "PasswordDialog.h"

STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 * /* numFiles */, const UINT64 * /* numBytes */)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 * /* numFiles */, const UINT64 * /* numBytes */)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 /* total */)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 * /* completed */)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
  if (_subArchiveMode)
  {
    switch(propID)
    {
      case kpidName:
        propVariant = _subArchiveName;
        break;
    }
    propVariant.Detach(value);
    return S_OK;
  }
  switch(propID)
  {
  case kpidName:
    propVariant = _fileInfo.Name;
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
  if (_subArchiveMode)
    return S_FALSE;

  NWindows::NFile::NFind::CFileInfoW fileInfo;

  UString fullPath = _folderPrefix + name;
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
  PasswordWasAsked = true;
  if (!PasswordIsDefined)
  {
    CPasswordDialog dialog;
   
    if (dialog.Create(ParentWindow) == IDCANCEL)
      return E_ABORT;

    Password = dialog.Password;
    PasswordIsDefined = true;
  }
  CMyComBSTR tempName(Password);
  *password = tempName.Detach();
  return S_OK;
}
