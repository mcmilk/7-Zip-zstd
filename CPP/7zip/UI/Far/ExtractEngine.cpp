// ExtractEngine.h

#include "StdAfx.h"

#include <stdio.h>

#include "ExtractEngine.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"

#include "FarUtils.h"
#include "Messages.h"
#include "OverwriteDialog.h"

using namespace NWindows;
using namespace NFar;

extern void PrintMessage(const char *message);

CExtractCallBackImp::~CExtractCallBackImp()
{
}

void CExtractCallBackImp::Init(
    UINT codePage,
    CProgressBox *progressBox, 
    bool passwordIsDefined, 
    const UString &password)
{
  m_PasswordIsDefined = passwordIsDefined;
  m_Password = password;
  m_CodePage = codePage;
  m_ProgressBox = progressBox;
}

STDMETHODIMP CExtractCallBackImp::SetTotal(UINT64 size)
{
  if (m_ProgressBox != 0)
  {
    m_ProgressBox->SetTotal(size);
    m_ProgressBox->PrintCompeteValue(0);
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetCompleted(const UINT64 *completeValue)
{
  if(WasEscPressed())
    return E_ABORT;
  if (m_ProgressBox != 0 && completeValue != NULL)
    m_ProgressBox->PrintCompeteValue(*completeValue);
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
    INT32 *answer)
{
  NOverwriteDialog::CFileInfo oldFileInfo, newFileInfo;
  oldFileInfo.Time = *existTime;
  oldFileInfo.SizeIsDefined = (existSize != NULL);
  if (oldFileInfo.SizeIsDefined)
    oldFileInfo.Size = *existSize;
  oldFileInfo.Name = GetSystemString(existName, m_CodePage);

  newFileInfo.TimeIsDefined = (newTime != 0);
  if (newFileInfo.TimeIsDefined)
    newFileInfo.Time = *newTime;
  
  newFileInfo.SizeIsDefined = (newSize != NULL);
  if (newFileInfo.SizeIsDefined)
    newFileInfo.Size = *newSize;
  newFileInfo.Name = GetSystemString(newName, m_CodePage);
  
  NOverwriteDialog::NResult::EEnum result = 
    NOverwriteDialog::Execute(oldFileInfo, newFileInfo);
  
  switch(result)
  {
  case NOverwriteDialog::NResult::kCancel:
    // *answer = NOverwriteAnswer::kCancel;
    // break;
    return E_ABORT;
  case NOverwriteDialog::NResult::kNo:
    *answer = NOverwriteAnswer::kNo;
    break;
  case NOverwriteDialog::NResult::kNoToAll:
    *answer = NOverwriteAnswer::kNoToAll;
    break;
  case NOverwriteDialog::NResult::kYesToAll:
    *answer = NOverwriteAnswer::kYesToAll;
    break;
  case NOverwriteDialog::NResult::kYes:
    *answer = NOverwriteAnswer::kYes;
    break;
  case NOverwriteDialog::NResult::kAutoRename:
    *answer = NOverwriteAnswer::kAutoRename;
    break;
  default:
    throw 20413;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::PrepareOperation(const wchar_t *name, bool /* isFolder */, INT32 /* askExtractMode */, const UINT64 * /* position */)
{
  m_CurrentFilePath = name;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::MessageError(const wchar_t *message)
{
  AString s = UnicodeStringToMultiByte(message, CP_OEMCP);
  if (g_StartupInfo.ShowMessage((const char *)s) == -1)
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetOperationResult(INT32 operationResult, bool encrypted)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT idMessage;
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          idMessage = NMessageID::kExtractUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          idMessage = encrypted ? 
            NMessageID::kExtractCRCFailedEncrypted :
            NMessageID::kExtractCRCFailed;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          idMessage = encrypted ? 
            NMessageID::kExtractDataErrorEncrypted :
            NMessageID::kExtractDataError;
          break;
        default:
          return E_FAIL;
      }
      char buffer[512];
      const AString s = GetSystemString(m_CurrentFilePath, m_CodePage);
      sprintf(buffer, g_StartupInfo.GetMsgString(idMessage), (const char *)s);
      if (g_StartupInfo.ShowMessage(buffer) == -1)
        return E_ABORT;
    }
  }
  return S_OK;
}

extern HRESULT GetPassword(UString &password);

STDMETHODIMP CExtractCallBackImp::CryptoGetTextPassword(BSTR *password)
{
  if (!m_PasswordIsDefined)
  {
    RINOK(GetPassword(m_Password));
    m_PasswordIsDefined = true;
  }
  CMyComBSTR tempName = m_Password;
  *password = tempName.Detach();

  return S_OK;
}
