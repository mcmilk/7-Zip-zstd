// ExtractEngine.h

#include "StdAfx.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/StringConvert.h"

#include "ExtractEngine.h"
#include "FarUtils.h"
#include "Messages.h"
#include "OverwriteDialogFar.h"

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

STDMETHODIMP CExtractCallBackImp::SetTotal(UInt64 size)
{
  _total = size;
  _totalIsDefined = true;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetCompleted(const UInt64 *completeValue)
{
  if (WasEscPressed())
    return E_ABORT;
  _processedIsDefined = (completeValue != NULL);
  if (_processedIsDefined)
    _processed = *completeValue;
  if (m_ProgressBox != 0)
    m_ProgressBox->Progress(
      _totalIsDefined ? &_total: NULL,
      _processedIsDefined ? &_processed: NULL, AString());
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
    Int32 *answer)
{
  NOverwriteDialog::CFileInfo oldFileInfo, newFileInfo;
  oldFileInfo.TimeIsDefined = (existTime != 0);
  if (oldFileInfo.TimeIsDefined)
    oldFileInfo.Time = *existTime;
  oldFileInfo.SizeIsDefined = (existSize != NULL);
  if (oldFileInfo.SizeIsDefined)
    oldFileInfo.Size = *existSize;
  oldFileInfo.Name = existName;

  newFileInfo.TimeIsDefined = (newTime != 0);
  if (newFileInfo.TimeIsDefined)
    newFileInfo.Time = *newTime;
  newFileInfo.SizeIsDefined = (newSize != NULL);
  if (newFileInfo.SizeIsDefined)
    newFileInfo.Size = *newSize;
  newFileInfo.Name = newName;
  
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
    return E_FAIL;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::PrepareOperation(const wchar_t *name, bool /* isFolder */, Int32 /* askExtractMode */, const UInt64 * /* position */)
{
  if (WasEscPressed())
    return E_ABORT;
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

static void ReduceString(UString &s, unsigned size)
{
  if (s.Len() > size)
  {
    s.Delete(size / 2, s.Len() - size);
    s.Insert(size / 2, L" ... ");
  }
}

STDMETHODIMP CExtractCallBackImp::SetOperationResult(Int32 operationResult, bool encrypted)
{
  switch (operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT messageID = 0;
      switch (operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
          messageID = NMessageID::kExtractUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          messageID = encrypted ?
            NMessageID::kExtractCRCFailedEncrypted :
            NMessageID::kExtractCRCFailed;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          messageID = encrypted ?
            NMessageID::kExtractDataErrorEncrypted :
            NMessageID::kExtractDataError;
          break;
      }
      UString name = m_CurrentFilePath;
      ReduceString(name, 70);
      AString s;
      if (messageID != 0)
      {
        s = g_StartupInfo.GetMsgString(messageID);
        s.Replace(" '%s'", "");
      }
      else if (operationResult == NArchive::NExtract::NOperationResult::kUnavailable)
        s = "Unavailable data";
      else if (operationResult == NArchive::NExtract::NOperationResult::kUnexpectedEnd)
        s = "Unexpected end of data";
      else if (operationResult == NArchive::NExtract::NOperationResult::kDataAfterEnd)
        s = "There are some data after the end of the payload data";
      else if (operationResult == NArchive::NExtract::NOperationResult::kIsNotArc)
        s = "Is not archive";
      else if (operationResult == NArchive::NExtract::NOperationResult::kHeadersError)
        s = "kHeaders Error";
      else
      {
        char temp[16];
        ConvertUInt32ToString(operationResult, temp);
        s = "Error #";
        s += temp;
      }
      s += "\n";
      s += UnicodeStringToMultiByte(name, m_CodePage);
      if (g_StartupInfo.ShowMessageLines(s) == -1)
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
  return StringToBstr(m_Password, password);
}
