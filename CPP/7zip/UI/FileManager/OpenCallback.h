// OpenCallback.h

#ifndef __OPENCALLBACK_H
#define __OPENCALLBACK_H

#include "Common/MyString.h"
#include "Common/MyCom.h"
#include "Windows/FileFind.h"

#include "../../IPassword.h"

#include "../../Archive/IArchive.h"


#ifdef _SFX
#include "ProgressDialog.h"
#else
#include "ProgressDialog2.h"
#endif


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

  bool _numFilesTotalDefined;
  bool _numBytesTotalDefined;
  NWindows::NSynchronization::CCriticalSection _criticalSection;

public:
  bool PasswordIsDefined;
  UString Password;
  bool PasswordWasAsked;
  HWND ParentWindow;

  bool _subArchiveMode;
  UString _subArchiveName;

public:
  CProgressDialog ProgressDialog;

  MY_UNKNOWN_IMP5(
    IArchiveOpenCallback,
    IArchiveOpenVolumeCallback,
    IArchiveOpenSetSubArchiveName,
    IProgress,
    ICryptoGetTextPassword)

  INTERFACE_IProgress(;)
  INTERFACE_IArchiveOpenCallback(;)
  INTERFACE_IArchiveOpenVolumeCallback(;)

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  STDMETHOD(SetSubArchiveName(const wchar_t *name))
  {
    _subArchiveMode = true;
    _subArchiveName = name;
    return  S_OK;
  }

  COpenArchiveCallback():
    ParentWindow(0)
  {
    _numFilesTotalDefined = false;
    _numBytesTotalDefined = false;

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
  void ShowMessage(const UInt64 *completed);

  INT_PTR StartProgressDialog(const UString &title)
  {
    return ProgressDialog.Create(title, ParentWindow);
  }

};

#endif
