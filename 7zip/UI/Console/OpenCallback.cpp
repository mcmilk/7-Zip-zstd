// OpenCallback.cpp

#include "StdAfx.h"

#include "OpenCallback.h"

#include "Common/StdOutStream.h"
#include "Common/StdInStream.h"
#include "Common/StringConvert.h"

#include "../../Common/FileStreams.h"

#include "Windows/PropVariant.h"

#include "ConsoleClose.h"

STDMETHODIMP COpenCallbackImp::SetTotal(const UINT64 *files, const UINT64 *bytes)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP COpenCallbackImp::SetCompleted(const UINT64 *files, const UINT64 *bytes)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}
  
STDMETHODIMP COpenCallbackImp::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
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

STDMETHODIMP COpenCallbackImp::GetStream(const wchar_t *name, 
    IInStream **inStream)
{
  *inStream = NULL;
  UString fullPath = _folderPrefix + name;
  if (!NWindows::NFile::NFind::FindFile(fullPath, _fileInfo))
    return S_FALSE;
  if (_fileInfo.IsDirectory())
    return S_FALSE;
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<IInStream> inStreamTemp = inFile;
  if (!inFile->Open(fullPath))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
}

STDMETHODIMP COpenCallbackImp::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    g_StdOut << "\nEnter password:";
    AString oemPassword = g_StdIn.ScanStringUntilNewLine();
    Password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
    PasswordIsDefined = true;
  }
  CMyComBSTR temp(Password);
  *password = temp.Detach();
  return S_OK;
}
  
