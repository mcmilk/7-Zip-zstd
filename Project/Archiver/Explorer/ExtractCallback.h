// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "../Common/IArchiveHandler2.h"
#include "Common/String.h"

#include "ExtractDialog.h"
#include "ProgressDialog.h"
#include "MessagesDialog.h"

#include "Windows/ResourceString.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
#endif

#include "../Format/Common/FormatCryptoInterface.h"

class CExtractCallBackImp: 
  public IExtractCallback2,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallBackImp)
  COM_INTERFACE_ENTRY(IExtractCallback2)
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
  STDMETHOD(OperationResult)(INT32 aResultEOperationResult);
  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CComPtr<IArchiveHandler100> m_ArchiveHandler;
  CSysString m_DirectoryPath;

  NExtractionDialog::CModeInfo m_ExtractModeInfo;

  // bool m_MessagesDialogWasCreated;
  CSysString m_DiskFilePath;
  
  bool m_ExtractMode;

  UString m_CurrentFilePath;

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
  // CProgressDialog m_ProcessDialog;
  HRESULT StartProgressDialog(bool aTestMode = false)
  {
    m_ThreadID = GetCurrentThreadId();
    m_ProgressDialog.Create(m_ParentWindow);
    if (aTestMode)
    {
      #ifdef LANG        
      m_ProgressDialog.SetText(LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90));
      #else
      m_ProgressDialog.SetText(NWindows::MyLoadString(IDS_PROGRESS_TESTING));
      #endif
    }
    else
    {
      #ifdef LANG        
      m_ProgressDialog.SetText(LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890));
      #else
      m_ProgressDialog.SetText(NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING));
      #endif
    }

    m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);    
    // m_ProgressDialog.Start(m_ParentWindow, PROGDLG_MODAL | PROGDLG_AUTOTIME);
    return S_OK;
  }

  ~CExtractCallBackImp();
  void Init(IArchiveHandler100 *anArchiveHandler, 
      NExtractionDialog::CModeInfo anExtractModeInfo,
      bool aPasswordIsDefined, const UString &aPassword);
  void DestroyWindows();
};

#endif
