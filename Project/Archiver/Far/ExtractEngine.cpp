// ExtractEngine.h

#include "StdAfx.h"

#include "ExtractEngine.h"
#include "Far/FarUtils.h"

#include "Messages.h"

#include "OverwriteDialog.h"

#include "Common/WildCard.h"
#include "Common/StringConvert.h"

using namespace NWindows;
using namespace NFar;

extern void PrintMessage(const char *aMessage);

CExtractCallBackImp::~CExtractCallBackImp()
{
}

void CExtractCallBackImp::Init(IArchiveHandler100 *anArchiveHandler,
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

STDMETHODIMP CExtractCallBackImp::OperationResult(INT32 anOperationResult)
{
  switch(anOperationResult)
  {
    case NArchiveHandler::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT anIDMessage;
      switch(anOperationResult)
      {
        case NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod:
          anIDMessage = NMessageID::kExtractUnsupportedMethod;
          break;
        case NArchiveHandler::NExtract::NOperationResult::kCRCError:
          anIDMessage = NMessageID::kExtractCRCFailed;
          break;
        case NArchiveHandler::NExtract::NOperationResult::kDataError:
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

STDMETHODIMP CExtractCallBackImp::CryptoGetTextPassword(BSTR *aPassword)
{
  if (!m_PasswordIsDefined)
  {
    CInitDialogItem anInitItems[]=
    {
      { DI_DOUBLEBOX, 3, 1, 72, 4, false, false, 0, false,  NMessageID::kGetPasswordTitle, NULL, NULL }, 
      { DI_TEXT, 5, 2, 0, 0, false, false, DIF_SHOWAMPERSAND, false, NMessageID::kEnterPasswordForFile, NULL, NULL },
      { DI_PSWEDIT, 5, 3, 70, 3, true, false, 0, true, -1, "", NULL }
    };
    
    const kNumItems = sizeof(anInitItems)/sizeof(anInitItems[0]);
    FarDialogItem aDialogItems[kNumItems];
    g_StartupInfo.InitDialogItems(anInitItems, aDialogItems, kNumItems);
    
    // sprintf(DialogItems[1].Data,GetMsg(MGetPasswordForFile),FileName);
    if (g_StartupInfo.ShowDialog(76, 6, NULL, aDialogItems, kNumItems) < 0)
      return (E_ABORT);
    
    AString anOemPassword = aDialogItems[2].Data;
    m_Password = MultiByteToUnicodeString(anOemPassword, CP_OEMCP); 
    m_PasswordIsDefined = true;
  }
  CComBSTR aTempName = m_Password;
  *aPassword = aTempName.Detach();

  return S_OK;
}
