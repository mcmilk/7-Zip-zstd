// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "../Common/IArchiveHandler2.h"
#include "Common/String.h"

#include "../Resource/ProgressDialog/ProgressDialog.h"
#include "resource.h"

#include "Windows/ResourceString.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
#endif

#include "../Format/Common/FormatCryptoInterface.h"

class CExtractCallBackImp: 
  public IExtractCallback3,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallBackImp)
  COM_INTERFACE_ENTRY(IExtractCallback3)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallBackImp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IExtractCallBack2
  STDMETHOD(AskOverwrite)(
      const wchar_t *anExistName, const FILETIME *anExistTime, const UINT64 *anExistSize,
      const wchar_t *aNewName, const FILETIME *aNewTime, const UINT64 *aNewSize,
      INT32 *aResult);
  STDMETHOD (PrepareOperation)(const wchar_t *aName, INT32 anAskExtractMode);

  STDMETHOD(MessageError)(const wchar_t *aMessage);
  STDMETHOD(OperationResult)(INT32 aResultEOperationResult);

  // IExtractCallBack3
  STDMETHOD(AskWrite)(
      const wchar_t *aSrcPath, INT32 aSrcIsFolder, 
      const FILETIME *aSrcTime, const UINT64 *aSrcSize,
      const wchar_t *aDestPath, 
      BSTR *aDestPathResult, 
      INT32 *aResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CSysString m_DirectoryPath;

  CSysString m_DiskFilePath;
  
  bool m_ExtractMode;

  UString m_CurrentFilePath;
  NExtractionMode::NOverwrite::EEnum m_OverwriteMode;

  bool m_PasswordIsDefined;
  UString m_Password;
  CProgressDialog m_ProgressDialog;


  void CreateComplexDirectory(const UStringVector &aDirPathParts);
  void GetPropertyValue(LPITEMIDLIST anItemIDList, PROPID aPropId, 
      PROPVARIANT *aValue);
  bool IsEncrypted(LPITEMIDLIST anItemIDList);
  void AddErrorMessage(LPCTSTR aMessage);
public:
  CSysStringVector m_Messages;
  HWND m_ParentWindow;
  DWORD m_ThreadID;
  UINT m_FileCodePage;
  // CProgressDialog m_ProcessDialog;
  HRESULT StartProgressDialog(const CSysString &aTitle)
  {
    m_ThreadID = GetCurrentThreadId();
    m_ProgressDialog.Create(m_ParentWindow);
    m_ProgressDialog.SetText(aTitle);

    m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);    
    // m_ProgressDialog.Start(m_ParentWindow, PROGDLG_MODAL | PROGDLG_AUTOTIME);
    return S_OK;
  }

  ~CExtractCallBackImp();
  void Init(NExtractionMode::NOverwrite::EEnum anOverwriteMode,
      bool aPasswordIsDefined, const UString &aPassword);
  void DestroyWindows();
};

#endif
