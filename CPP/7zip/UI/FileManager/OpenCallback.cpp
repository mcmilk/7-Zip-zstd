// OpenCallback.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

#include "OpenCallback.h"
#include "PasswordDialog.h"

using namespace NWindows;

STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 *numFiles, const UInt64 *numBytes)
{
  RINOK(ProgressDialog.Sync.ProcessStopAndPause());
  {
    NSynchronization::CCriticalSectionLock lock(_criticalSection);
    if (numFiles != NULL)
    {
      ProgressDialog.Sync.SetNumFilesTotal(*numFiles);
      ProgressDialog.Sync.SetBytesProgressMode(false);
    }
    if (numBytes != NULL)
      ProgressDialog.Sync.SetNumBytesTotal(*numBytes);
  }
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *numFiles, const UInt64 *numBytes)
{
  RINOK(ProgressDialog.Sync.ProcessStopAndPause());
  NSynchronization::CCriticalSectionLock lock(_criticalSection);
  if (numFiles != NULL)
    ProgressDialog.Sync.SetNumFilesCur(*numFiles);
  if (numBytes != NULL)
    ProgressDialog.Sync.SetPos(*numBytes);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 total)
{
  RINOK(ProgressDialog.Sync.ProcessStopAndPause());
  ProgressDialog.Sync.SetNumBytesTotal(total);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *completed)
{
  RINOK(ProgressDialog.Sync.ProcessStopAndPause());
  if (completed != NULL)
    ProgressDialog.Sync.SetPos(*completed);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  if (_subArchiveMode)
  {
    switch(propID)
    {
      case kpidName: prop = _subArchiveName; break;
    }
  }
  else
  {
    switch(propID)
    {
      case kpidName:  prop = _fileInfo.Name; break;
      case kpidIsDir:  prop = _fileInfo.IsDir(); break;
      case kpidSize:  prop = _fileInfo.Size; break;
      case kpidAttrib:  prop = (UInt32)_fileInfo.Attrib; break;
      case kpidCTime:  prop = _fileInfo.CTime; break;
      case kpidATime:  prop = _fileInfo.ATime; break;
      case kpidMTime:  prop = _fileInfo.MTime; break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetStream(const wchar_t *name,
    IInStream **inStream)
{
  *inStream = NULL;
  if (_subArchiveMode)
    return S_FALSE;

  NFile::NFind::CFileInfoW fileInfo;

  UString fullPath = _folderPrefix + name;
  if (!fileInfo.Find(fullPath))
    return S_FALSE;
  _fileInfo = fileInfo;
  if (_fileInfo.IsDir())
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
   
    ProgressDialog.WaitCreating();
    if (dialog.Create(ProgressDialog) == IDCANCEL)
      return E_ABORT;

    Password = dialog.Password;
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}
