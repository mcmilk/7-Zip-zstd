// OpenCallback.h

#pragma once

#ifndef __OPENCALLBACK_H
#define __OPENCALLBACK_H

#include "Common/String.h"
#include "Common/MyCom.h"
#include "Windows/FileFind.h"

#include "../IPassword.h"

#include "../Archive/IArchive.h"

class COpenArchiveCallback: 
  public IArchiveOpenCallback,
  public IArchiveOpenVolumeCallback,
  public IProgress,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
  CSysString _folderPrefix;
  NWindows::NFile::NFind::CFileInfo _fileInfo;
public:
  bool _passwordIsDefined;
  UString _password;
  HWND _parentWindow;

public:
  MY_UNKNOWN_IMP4(
    IArchiveOpenCallback,
    IArchiveOpenVolumeCallback,
    IProgress,
    ICryptoGetTextPassword)

  // IProgress
  STDMETHOD(SetTotal)(UINT64 total);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IArchiveOpenCallback
  STDMETHOD(SetTotal)(const UINT64 *numFiles, const UINT64 *numBytes);
  STDMETHOD(SetCompleted)(const UINT64 *numFiles, const UINT64 *numBytes);

  // IArchiveOpenVolumeCallback
  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  void Init()
  {
    _passwordIsDefined = false;
  }
  void LoadFileInfo(const CSysString &folderPrefix,  const CSysString &fileName)
  {
    _folderPrefix = folderPrefix;
    if (!NWindows::NFile::NFind::FindFile(_folderPrefix + fileName, _fileInfo))
      throw 1;
  }
  void ShowMessage(const UINT64 *completed);
};

#endif
