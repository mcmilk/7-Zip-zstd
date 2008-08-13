// OpenCallback.cpp

#include "StdAfx.h"

#include "OpenCallback.h"

#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

#include "PasswordDialog.h"

STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 *numFiles, const UInt64 *numBytes)
{
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    _numFilesTotalDefined = (numFiles != NULL);
    _numBytesTotalDefined = (numBytes != NULL);
    if (_numFilesTotalDefined)
    {
      ProgressDialog.ProgressSynch.SetNumFilesTotal(*numFiles);
      ProgressDialog.ProgressSynch.SetProgress(*numFiles, 0);
    }
    else if (_numBytesTotalDefined)
      ProgressDialog.ProgressSynch.SetProgress(*numBytes, 0);
  }
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *numFiles, const UInt64 *numBytes)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
  RINOK(ProgressDialog.ProgressSynch.ProcessStopAndPause());
  if (numFiles != NULL)
  {
    ProgressDialog.ProgressSynch.SetNumFilesCur(*numFiles);
    if (_numFilesTotalDefined)
      ProgressDialog.ProgressSynch.SetPos(*numFiles);
  }
  if (numBytes != NULL && _numBytesTotalDefined)
    ProgressDialog.ProgressSynch.SetPos(*numBytes);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 total)
{
  ProgressDialog.ProgressSynch.SetProgress(total, 0);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *completed)
{
  RINOK(ProgressDialog.ProgressSynch.ProcessStopAndPause());
  if (completed != NULL)
    ProgressDialog.ProgressSynch.SetPos(*completed);
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
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

  NWindows::NFile::NFind::CFileInfoW fileInfo;

  UString fullPath = _folderPrefix + name;
  if (!NWindows::NFile::NFind::FindFile(fullPath, fileInfo))
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
   
    if (dialog.Create(ProgressDialog) == IDCANCEL)
      return E_ABORT;

    Password = dialog.Password;
    PasswordIsDefined = true;
  }
  CMyComBSTR tempName(Password);
  *password = tempName.Detach();
  return S_OK;
}
