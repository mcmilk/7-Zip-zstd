// ExtractEngine.h

#pragma once

#ifndef __EXTRACTENGINE_H
#define __EXTRACTENGINE_H

#include "../Common/FolderArchiveInterface.h"
#include "Common/String.h"

#include "Far/ProgressBox.h"

#include "Interface/CryptoInterface.h"

class CExtractCallBackImp: 
  public IFolderArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallBackImp)
  COM_INTERFACE_ENTRY(IFolderArchiveExtractCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallBackImp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IExtractCallBack
  STDMETHOD(AskOverwrite)(
      const wchar_t *anExistName, const FILETIME *anExistTime, const UINT64 *anExistSize,
      const wchar_t *aNewName, const FILETIME *aNewTime, const UINT64 *aNewSize,
      INT32 *aResult);
  STDMETHOD (PrepareOperation)(const wchar_t *aName, INT32 anAskExtractMode);

  STDMETHOD(MessageError)(const wchar_t *aMessage);
  STDMETHOD(SetOperationResult)(INT32 aResultEOperationResult);
  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  UString m_CurrentFilePath;

  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    bool IsDirectory;
    UINT32 Attributes;
  } m_ProcessedFileInfo;

  CProgressBox *m_ProgressBox;
  UINT m_CodePage;

  bool m_PasswordIsDefined;
  UString m_Password;

  void CreateComplexDirectory(const UStringVector &aDirPathParts);
  void GetPropertyValue(LPITEMIDLIST anItemIDList, PROPID aPropId, 
      PROPVARIANT *aValue);
  bool IsEncrypted(LPITEMIDLIST anItemIDList);
  void AddErrorMessage(LPCTSTR aMessage);
public:
  ~CExtractCallBackImp();
  void Init(UINT aCodePage, 
      CProgressBox *aProgressBox, 
      bool aPasswordIsDefined, const UString &aPassword);
};

#endif
