// ExtractEngine.h

#include "StdAfx.h"

#include "ExtractEngine.h"
#include "Far/FarUtils.h"

#include "Messages.h"

#include "OverwriteDialog.h"

#include "Common/WildCard.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"

using namespace NWindows;
using namespace NFar;

extern void PrintMessage(const char *aMessage);

CExtractCallBackImp::~CExtractCallBackImp()
{
}

void CExtractCallBackImp::Init(
    UINT aCodePage,
    CProgressBox *aProgressBox, 
    bool aPasswordIsDefined, 
    const UString &aPassword)
{
  m_PasswordIsDefined = aPasswordIsDefined;
  m_Password = aPassword;
  m_CodePage = aCodePage;
  m_ProgressBox = aProgressBox;
}

STDMETHODIMP CExtractCallBackImp::SetTotal(UINT64 aSize)
{
  if (m_ProgressBox != 0)
  {
    m_ProgressBox->SetTotal(aSize);
    m_ProgressBox->PrintCompeteValue(0);
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetCompleted(const UINT64 *aCompleteValue)
{
  if(WasEscPressed())
    return E_ABORT;
  if (m_ProgressBox != 0 && aCompleteValue != NULL)
    m_ProgressBox->PrintCompeteValue(*aCompleteValue);
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::AskOverwrite(
    const wchar_t *anExistName, const FILETIME *anExistTime, const UINT64 *anExistSize,
    const wchar_t *aNewName, const FILETIME *aNewTime, const UINT64 *aNewSize,
    INT32 *anAnswer)
{
  NOverwriteDialog::CFileInfo anOldFileInfo, aNewFileInfo;
  anOldFileInfo.Time = *anExistTime;
  if (anOldFileInfo.SizeIsDefined = (anExistSize != NULL))
    anOldFileInfo.Size = *anExistSize;
  anOldFileInfo.Name = UnicodeStringToMultiByte(anExistName, m_CodePage);

 
  aNewFileInfo.Time = *aNewTime;
  
  if (aNewFileInfo.SizeIsDefined = (aNewSize != NULL))
    aNewFileInfo.Size = *aNewSize;
  aNewFileInfo.Name = UnicodeStringToMultiByte(aNewName, m_CodePage);
  
  NOverwriteDialog::NResult::EEnum aResult = 
    NOverwriteDialog::Execute(anOldFileInfo, aNewFileInfo);
  
  switch(aResult)
  {
  case NOverwriteDialog::NResult::kCancel:
    // *anAnswer = NOverwriteAnswer::kCancel;
    // break;
    return E_ABORT;
  case NOverwriteDialog::NResult::kNo:
    *anAnswer = NOverwriteAnswer::kNo;
    break;
  case NOverwriteDialog::NResult::kNoToAll:
    *anAnswer = NOverwriteAnswer::kNoToAll;
    break;
  case NOverwriteDialog::NResult::kYesToAll:
    *anAnswer = NOverwriteAnswer::kYesToAll;
    break;
  case NOverwriteDialog::NResult::kYes:
    *anAnswer = NOverwriteAnswer::kYes;
    break;
  case NOverwriteDialog::NResult::kAutoRename:
    *anAnswer = NOverwriteAnswer::kAutoRename;
    break;
  default:
    throw 20413;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::PrepareOperation(const wchar_t *aName, INT32 anAskExtractMode)
{
  m_CurrentFilePath = aName;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::MessageError(const wchar_t *aMessage)
{
  CSysString aString = UnicodeStringToMultiByte(aMessage, CP_OEMCP);
  if (g_StartupInfo.ShowMessage(aString) == -1)
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetOperationResult(INT32 anOperationResult)
{
  switch(anOperationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT anIDMessage;
      switch(anOperationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          anIDMessage = NMessageID::kExtractUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          anIDMessage = NMessageID::kExtractCRCFailed;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          anIDMessage = NMessageID::kExtractDataError;
          break;
        default:
          return E_FAIL;
      }
      char aBuffer[512];
      sprintf(aBuffer, g_StartupInfo.GetMsgString(anIDMessage), 
          GetSystemString(m_CurrentFilePath, m_CodePage));
      if (g_StartupInfo.ShowMessage(aBuffer) == -1)
        return E_ABORT;
    }
  }
  return S_OK;
}

extern HRESULT GetPassword(UString &password);

STDMETHODIMP CExtractCallBackImp::CryptoGetTextPassword(BSTR *aPassword)
{
  if (!m_PasswordIsDefined)
  {
    RETURN_IF_NOT_S_OK(GetPassword(m_Password));
    m_PasswordIsDefined = true;
  }
  CComBSTR aTempName = m_Password;
  *aPassword = aTempName.Detach();

  return S_OK;
}
