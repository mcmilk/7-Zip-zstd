// OpenCallback.h

#ifndef __OPENCALLBACK_H
#define __OPENCALLBACK_H

#include "Common/MyString.h"
#include "Common/MyCom.h"
#include "Windows/FileFind.h"

#include "../../IPassword.h"

#include "../../Archive/IArchive.h"

class COpenArchiveCallback: 
  public IArchiveOpenCallback,
  public IArchiveOpenVolumeCallback,
  public IArchiveOpenSetSubArchiveName,
  public IProgress,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
  UString _folderPrefix;
  NWindows::NFile::NFind::CFileInfoW _fileInfo;
public:
  bool PasswordIsDefined;
  UString Password;
  bool PasswordWasAsked;
  HWND ParentWindow;

  bool _subArchiveMode;
  UString _subArchiveName;

public:
  MY_UNKNOWN_IMP5(
    IArchiveOpenCallback,
    IArchiveOpenVolumeCallback,
    IArchiveOpenSetSubArchiveName,
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

  STDMETHOD(SetSubArchiveName(const wchar_t *name))
  {
    _subArchiveMode = true;
    _subArchiveName = name;
    return  S_OK;
  }

  COpenArchiveCallback()
  {
    _subArchiveMode = false;
    PasswordIsDefined = false;
    PasswordWasAsked = false;
  }
  /*
  void Init()
  {
    PasswordIsDefined = false;
    _subArchiveMode = false;
  }
  */
  void LoadFileInfo(const UString &folderPrefix,  const UString &fileName)
  {
    _folderPrefix = folderPrefix;
    if (!NWindows::NFile::NFind::FindFile(_folderPrefix + fileName, _fileInfo))
      throw 1;
  }
  void ShowMessage(const UINT64 *completed);
};

#endif
