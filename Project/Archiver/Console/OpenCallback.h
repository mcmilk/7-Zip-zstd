// OpenCallback.h

#pragma once

#ifndef __OPENCALLBACK_H
#define __OPENCALLBACK_H

#include "Common/String.h"
#include "Windows/FileFind.h"

#include "../Format/Common/ArchiveInterface.h"
#include "Interface/CryptoInterface.h"

class COpenCallbackImp: 
  public IArchiveOpenCallback,
  public IArchiveOpenVolumeCallback,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(COpenCallbackImp)
  COM_INTERFACE_ENTRY(IArchiveOpenCallback)
  COM_INTERFACE_ENTRY(IArchiveOpenVolumeCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COpenCallbackImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetTotal)(const UINT64 *files, const UINT64 *bytes);
  STDMETHOD(SetCompleted)(const UINT64 *files, const UINT64 *bytes);

  // IArchiveOpenVolumeCallback
  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

private:
  CSysString _folderPrefix;
  NWindows::NFile::NFind::CFileInfo _fileInfo;
public:
  bool PasswordIsDefined;
  UString Password;
  COpenCallbackImp(): PasswordIsDefined(false) {}
  void LoadFileInfo(const CSysString &folderPrefix,  const CSysString &fileName)
  {
    _folderPrefix = folderPrefix;
    if (!NWindows::NFile::NFind::FindFile(_folderPrefix + fileName, _fileInfo))
      throw 1;
  }
};

#endif
