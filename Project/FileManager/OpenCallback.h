// OpenCallback.h

#pragma once

#ifndef __OPENCALLBACK_H
#define __OPENCALLBACK_H

#include "Common/String.h"
#include "Interface/CryptoInterface.h"
#include "Windows/FileFind.h"

#include "../Archiver/Format/Common/ArchiveInterface.h"

class COpenArchiveCallback: 
  public IArchiveOpenCallback,
  public IArchiveOpenVolumeCallback,
  public IProgress,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
  CSysString _folderPrefix;
  NWindows::NFile::NFind::CFileInfo _fileInfo;
public:
  bool _passwordIsDefined;
  UString _password;
  HWND _parentWindow;

public:
BEGIN_COM_MAP(COpenArchiveCallback)
  COM_INTERFACE_ENTRY(IArchiveOpenCallback)
  COM_INTERFACE_ENTRY(IArchiveOpenVolumeCallback)
  COM_INTERFACE_ENTRY(IProgress)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COpenArchiveCallback)

DECLARE_NO_REGISTRY()

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
