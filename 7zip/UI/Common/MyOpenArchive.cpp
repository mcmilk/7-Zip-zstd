// MyOpenArchive.cpp

#include "StdAfx.h"

#include "MyOpenArchive.h"
#include "OpenCallbackConsole.h"

#include "Windows/Defs.h"
#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"

#include "../Common/OpenArchive.h"

using namespace NWindows;

HRESULT MyOpenArchive(const UString &archiveName, 
    const NFile::NFind::CFileInfoW &archiveFileInfo,
    #ifndef EXCLUDE_COM
    HMODULE *module,
    #endif
    IInArchive **archive,
    UString &defaultItemName,
    bool &passwordEnabled, 
    UString &password)
{
  COpenCallbackConsole openCallbackConsole;
  COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
  CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  openCallbackSpec->Callback = &openCallbackConsole;
  if (passwordEnabled)
  {
    openCallbackConsole.PasswordIsDefined = passwordEnabled;
    openCallbackConsole.Password = password;
  }

  UString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(archiveName, fullName, fileNamePartStartIndex);
  openCallbackSpec->Init(
      fullName.Left(fileNamePartStartIndex), 
      fullName.Mid(fileNamePartStartIndex));

  CArchiverInfo archiverInfo;
  HRESULT result = OpenArchive(archiveName, 
      #ifndef EXCLUDE_COM
      module,
      #endif
      archive, 
      archiverInfo, 
      defaultItemName,
      openCallback);
  RINOK(result);
  passwordEnabled = openCallbackConsole.PasswordIsDefined;
  password = openCallbackConsole.Password;
  return S_OK;
}

HRESULT MyOpenArchive(const UString &archiveName, 
    const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
    #ifndef EXCLUDE_COM
    HMODULE *module0,
    HMODULE *module1,
    #endif
    IInArchive **archive0,
    IInArchive **archive1,
    UString &defaultItemName0,
    UString &defaultItemName1,
    bool &passwordEnabled, 
    UString &password)
{
  COpenCallbackConsole openCallbackConsole;
  COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
  CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  openCallbackSpec->Callback = &openCallbackConsole;
  if (passwordEnabled)
  {
    openCallbackConsole.PasswordIsDefined = passwordEnabled;
    openCallbackConsole.Password = password;
  }

  UString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(archiveName, fullName, fileNamePartStartIndex);
  openCallbackSpec->Init(
      fullName.Left(fileNamePartStartIndex), 
      fullName.Mid(fileNamePartStartIndex));

  CArchiverInfo archiverInfo0, archiverInfo1;
  HRESULT result = OpenArchive(archiveName, 
      #ifndef EXCLUDE_COM
      module0,
      module1,
      #endif
      archive0, 
      archive1, 
      archiverInfo0, 
      archiverInfo1, 
      defaultItemName0,
      defaultItemName1,
      openCallback);
  RINOK(result);
  passwordEnabled = openCallbackConsole.PasswordIsDefined;
  password = openCallbackConsole.Password;
  return S_OK;
}
