// ExtractCallback.cpp

#include "StdAfx.h"


#include "../../../Common/ComTry.h"
#include "../../../Common/IntToString.h"
#include "../../../Common/Lang.h"
#include "../../../Common/StringConvert.h"

#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/PropVariantConv.h"

#include "../../Common/FilePathAutoRename.h"
#include "../../Common/StreamUtils.h"
#include "../Common/ExtractingFilePath.h"

#ifndef _SFX
#include "../Common/ZipRegistry.h"
#endif

#include "../GUI/ExtractRes.h"

#include "ExtractCallback.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "OverwriteDialog.h"
#ifndef _NO_CRYPTO
#include "PasswordDialog.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NFind;

CExtractCallbackImp::~CExtractCallbackImp() {}

void CExtractCallbackImp::Init()
{
  NumArchiveErrors = 0;
  ThereAreMessageErrors = false;
  #ifndef _SFX
  NumFolders = NumFiles = 0;
  NeedAddFile = false;
  #endif
}

void CExtractCallbackImp::AddError_Message(LPCWSTR s)
{
  ThereAreMessageErrors = true;
  ProgressDialog->Sync.AddError_Message(s);
}

#ifndef _SFX

STDMETHODIMP CExtractCallbackImp::SetNumFiles(UInt64
  #ifndef _SFX
  numFiles
  #endif
  )
{
  #ifndef _SFX
  ProgressDialog->Sync.Set_NumFilesTotal(numFiles);
  #endif
  return S_OK;
}

#endif

STDMETHODIMP CExtractCallbackImp::SetTotal(UInt64 total)
{
  ProgressDialog->Sync.Set_NumBytesTotal(total);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UInt64 *value)
{
  return ProgressDialog->Sync.Set_NumBytesCur(value);
}

HRESULT CExtractCallbackImp::Open_CheckBreak()
{
  return ProgressDialog->Sync.CheckStop();
}

HRESULT CExtractCallbackImp::Open_SetTotal(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  // if (numFiles != NULL) ProgressDialog->Sync.SetNumFilesTotal(*numFiles);
  return S_OK;
}

HRESULT CExtractCallbackImp::Open_SetCompleted(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  // if (numFiles != NULL) ProgressDialog->Sync.SetNumFilesCur(*numFiles);
  return ProgressDialog->Sync.CheckStop();
}

#ifndef _NO_CRYPTO

HRESULT CExtractCallbackImp::Open_CryptoGetTextPassword(BSTR *password)
{
  return CryptoGetTextPassword(password);
}

HRESULT CExtractCallbackImp::Open_GetPasswordIfAny(bool &passwordIsDefined, UString &password)
{
  passwordIsDefined = PasswordIsDefined;
  password = Password;
  return S_OK;
}

bool CExtractCallbackImp::Open_WasPasswordAsked()
{
  return PasswordWasAsked;
}

void CExtractCallbackImp::Open_ClearPasswordWasAskedFlag()
{
  PasswordWasAsked = false;
}

#endif


#ifndef _SFX
STDMETHODIMP CExtractCallbackImp::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  ProgressDialog->Sync.Set_Ratio(inSize, outSize);
  return S_OK;
}
#endif

/*
STDMETHODIMP CExtractCallbackImp::SetTotalFiles(UInt64 total)
{
  ProgressDialog->Sync.SetNumFilesTotal(total);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompletedFiles(const UInt64 *value)
{
  if (value != NULL)
    ProgressDialog->Sync.SetNumFilesCur(*value);
  return S_OK;
}
*/

STDMETHODIMP CExtractCallbackImp::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
    Int32 *answer)
{
  COverwriteDialog dialog;

  dialog.OldFileInfo.SetTime(existTime);
  dialog.OldFileInfo.SetSize(existSize);
  dialog.OldFileInfo.Name = existName;

  dialog.NewFileInfo.SetTime(newTime);
  dialog.NewFileInfo.SetSize(newSize);
  dialog.NewFileInfo.Name = newName;
  
  ProgressDialog->WaitCreating();
  INT_PTR writeAnswer = dialog.Create(*ProgressDialog);
  
  switch (writeAnswer)
  {
    case IDCANCEL:        *answer = NOverwriteAnswer::kCancel; return E_ABORT;
    case IDYES:           *answer = NOverwriteAnswer::kYes; break;
    case IDNO:            *answer = NOverwriteAnswer::kNo; break;
    case IDB_YES_TO_ALL:  *answer = NOverwriteAnswer::kYesToAll; break;
    case IDB_NO_TO_ALL:   *answer = NOverwriteAnswer::kNoToAll; break;
    case IDB_AUTO_RENAME: *answer = NOverwriteAnswer::kAutoRename; break;
    default: return E_FAIL;
  }
  return S_OK;
}


STDMETHODIMP CExtractCallbackImp::PrepareOperation(const wchar_t *name, bool isFolder, Int32 /* askExtractMode */, const UInt64 * /* position */)
{
  _isFolder = isFolder;
  return SetCurrentFilePath2(name);
}

STDMETHODIMP CExtractCallbackImp::MessageError(const wchar_t *s)
{
  AddError_Message(s);
  return S_OK;
}

HRESULT CExtractCallbackImp::MessageError(const char *message, const FString &path)
{
  ThereAreMessageErrors = true;
  ProgressDialog->Sync.AddError_Message_Name(GetUnicodeString(message), fs2us(path));
  return S_OK;
}

#ifndef _SFX

STDMETHODIMP CExtractCallbackImp::ShowMessage(const wchar_t *s)
{
  AddError_Message(s);
  return S_OK;
}

#endif

STDMETHODIMP CExtractCallbackImp::SetOperationResult(Int32 opRes, bool encrypted)
{
  switch (opRes)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT messageID = 0;
      UINT id = 0;

      switch (opRes)
      {
        case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
          messageID = IDS_EXTRACT_MESSAGE_UNSUPPORTED_METHOD;
          id = IDS_EXTRACT_MSG_UNSUPPORTED_METHOD;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          messageID = encrypted ?
              IDS_EXTRACT_MESSAGE_DATA_ERROR_ENCRYPTED:
              IDS_EXTRACT_MESSAGE_DATA_ERROR;
          id = IDS_EXTRACT_MSG_DATA_ERROR;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          messageID = encrypted ?
              IDS_EXTRACT_MESSAGE_CRC_ERROR_ENCRYPTED:
              IDS_EXTRACT_MESSAGE_CRC_ERROR;
          id = IDS_EXTRACT_MSG_CRC_ERROR;
          break;
        case NArchive::NExtract::NOperationResult::kUnavailable:
          id = IDS_EXTRACT_MSG_UNAVAILABLE_DATA;
          break;
        case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
          id = IDS_EXTRACT_MSG_UEXPECTED_END;
          break;
        case NArchive::NExtract::NOperationResult::kDataAfterEnd:
          id = IDS_EXTRACT_MSG_DATA_AFTER_END;
          break;
        case NArchive::NExtract::NOperationResult::kIsNotArc:
          id = IDS_EXTRACT_MSG_IS_NOT_ARC;
          break;
        case NArchive::NExtract::NOperationResult::kHeadersError:
          id = IDS_EXTRACT_MSG_HEADERS_ERROR;
          break;
        /*
        default:
          messageID = IDS_EXTRACT_MESSAGE_UNKNOWN_ERROR;
          break;
        */
      }
      if (_needWriteArchivePath)
      {
        if (!_currentArchivePath.IsEmpty())
          AddError_Message(_currentArchivePath);
        _needWriteArchivePath = false;
      }

      UString msg;
      UString msgOld;

      #ifndef _SFX
      if (id != 0)
        LangString_OnlyFromLangFile(id, msg);
      if (messageID != 0 && msg.IsEmpty())
        LangString_OnlyFromLangFile(messageID, msgOld);
      #endif

      UString s;
      if (msg.IsEmpty() && !msgOld.IsEmpty())
        s = MyFormatNew(msgOld, _currentFilePath);
      else
      {
        if (msg.IsEmpty())
          LangString(id, msg);
        if (!msg.IsEmpty())
          s += msg;
        else
        {
          wchar_t temp[16];
          ConvertUInt32ToString(opRes, temp);
          s += L"Error #";
          s += temp;
        }

        if (encrypted)
        {
          // s += L" : ";
          // s += LangString(IDS_EXTRACT_MSG_ENCRYPTED);
          s += L" : ";
          s += LangString(IDS_EXTRACT_MSG_WRONG_PSW);
        }
        s += L" : ";
        s += _currentFilePath;
      }

      AddError_Message(s);
    }
  }
  
  #ifndef _SFX
  if (_isFolder)
    NumFolders++;
  else
    NumFiles++;
  ProgressDialog->Sync.Set_NumFilesCur(NumFiles);
  #endif
  
  return S_OK;
}

////////////////////////////////////////
// IExtractCallbackUI

HRESULT CExtractCallbackImp::BeforeOpen(const wchar_t *name)
{
  #ifndef _SFX
  RINOK(ProgressDialog->Sync.CheckStop());
  ProgressDialog->Sync.Set_TitleFileName(name);
  #endif
  _currentArchivePath = name;
  return S_OK;
}

HRESULT CExtractCallbackImp::SetCurrentFilePath2(const wchar_t *path)
{
  _currentFilePath = path;
  #ifndef _SFX
  ProgressDialog->Sync.Set_FilePath(path);
  #endif
  return S_OK;
}

#ifndef _SFX

HRESULT CExtractCallbackImp::SetCurrentFilePath(const wchar_t *path)
{
  #ifndef _SFX
  if (NeedAddFile)
    NumFiles++;
  NeedAddFile = true;
  ProgressDialog->Sync.Set_NumFilesCur(NumFiles);
  #endif
  return SetCurrentFilePath2(path);
}

#endif

UString HResultToMessage(HRESULT errorCode);

HRESULT CExtractCallbackImp::OpenResult(const wchar_t *name, HRESULT result, bool encrypted)
{
  if (result != S_OK)
  {
    UString s;
    if (result == S_FALSE)
      s = MyFormatNew(encrypted ? IDS_CANT_OPEN_ENCRYPTED_ARCHIVE : IDS_CANT_OPEN_ARCHIVE, name);
    else
    {
      s = name;
      s += L": ";
      s += HResultToMessage(result);
    }
    MessageError(s);
    NumArchiveErrors++;
  }
  _currentArchivePath = name;
  _needWriteArchivePath = true;
  return S_OK;
}

static const UInt32 k_ErrorFlagsIds[] =
{
  IDS_EXTRACT_MSG_IS_NOT_ARC,
  IDS_EXTRACT_MSG_HEADERS_ERROR,
  IDS_EXTRACT_MSG_HEADERS_ERROR,
  IDS_OPEN_MSG_UNAVAILABLE_START,
  IDS_OPEN_MSG_UNCONFIRMED_START,
  IDS_EXTRACT_MSG_UEXPECTED_END,
  IDS_EXTRACT_MSG_DATA_AFTER_END,
  IDS_EXTRACT_MSG_UNSUPPORTED_METHOD,
  IDS_OPEN_MSG_UNSUPPORTED_FEATURE,
  IDS_EXTRACT_MSG_DATA_ERROR,
  IDS_EXTRACT_MSG_CRC_ERROR
};

UString GetOpenArcErrorMessage(UInt32 errorFlags)
{
  UString s;
  for (unsigned i = 0; i < ARRAY_SIZE(k_ErrorFlagsIds); i++)
  {
    UInt32 f = ((UInt32)1 << i);
    if ((errorFlags & f) == 0)
      continue;
    UInt32 id = k_ErrorFlagsIds[i];
    UString m = LangString(id);
    if (m.IsEmpty())
      continue;
    if (f == kpv_ErrorFlags_EncryptedHeadersError)
    {
      m += L" : ";
      m += LangString(IDS_EXTRACT_MSG_WRONG_PSW);
    }
    if (!s.IsEmpty())
      s += L'\n';
    s += m;
    errorFlags &= ~f;
  }
  if (errorFlags != 0)
  {
    char sz[16];
    sz[0] = '0';
    sz[1] = 'x';
    ConvertUInt32ToHex(errorFlags, sz + 2);
    if (!s.IsEmpty())
      s += L'\n';
    s += GetUnicodeString(AString(sz));
  }
  return s;
}

HRESULT CExtractCallbackImp::SetError(int level, const wchar_t *name,
    UInt32 errorFlags, const wchar_t *errors,
    UInt32 warningFlags, const wchar_t *warnings)
{
  NumArchiveErrors++;

  if (_needWriteArchivePath)
  {
    if (!_currentArchivePath.IsEmpty())
      AddError_Message(_currentArchivePath);
    _needWriteArchivePath = false;
  }

  if (level != 0)
  {
    UString s;
    s += name;
    s += L": ";
    MessageError(s);
  }

  if (errorFlags != 0)
    MessageError(GetOpenArcErrorMessage(errorFlags));

  if (errors && wcslen(errors) != 0)
    MessageError(errors);

  if (warningFlags != 0)
    MessageError((UString)L"Warnings: " + GetOpenArcErrorMessage(warningFlags));

  if (warnings && wcslen(warnings) != 0)
    MessageError((UString)L"Warnings: " + warnings);

  return S_OK;
}

HRESULT CExtractCallbackImp::OpenTypeWarning(const wchar_t *name, const wchar_t *okType, const wchar_t *errorType)
{
  UString s = L"Warning:\n";
  s += name;
  s += L"\n";
  if (wcscmp(okType, errorType) == 0)
  {
    s += L"The archive is open with offset";
  }
  else
  {
    s += L"Can not open the file as [";
    s += errorType;
    s += L"] archive\n";
    s += L"The file is open as [";
    s += okType;
    s += L"] archive";
  }
  MessageError(s);
  NumArchiveErrors++;
  return S_OK;
}
  
HRESULT CExtractCallbackImp::ThereAreNoFiles()
{
  return S_OK;
}

HRESULT CExtractCallbackImp::ExtractResult(HRESULT result)
{
  if (result == S_OK)
    return result;
  NumArchiveErrors++;
  if (result == E_ABORT || result == ERROR_DISK_FULL)
    return result;
  MessageError(_currentFilePath);
  MessageError(NError::MyFormatMessage(result));
  return S_OK;
}

#ifndef _NO_CRYPTO

HRESULT CExtractCallbackImp::SetPassword(const UString &password)
{
  PasswordIsDefined = true;
  Password = password;
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::CryptoGetTextPassword(BSTR *password)
{
  PasswordWasAsked = true;
  if (!PasswordIsDefined)
  {
    CPasswordDialog dialog;
    #ifndef _SFX
    bool showPassword = NExtract::Read_ShowPassword();
    dialog.ShowPassword = showPassword;
    #endif
    ProgressDialog->WaitCreating();
    if (dialog.Create(*ProgressDialog) != IDOK)
      return E_ABORT;
    Password = dialog.Password;
    PasswordIsDefined = true;
    #ifndef _SFX
    if (dialog.ShowPassword != showPassword)
      NExtract::Save_ShowPassword(dialog.ShowPassword);
    #endif
  }
  return StringToBstr(Password, password);
}

#endif

#ifndef _SFX

STDMETHODIMP CExtractCallbackImp::AskWrite(
    const wchar_t *srcPath, Int32 srcIsFolder,
    const FILETIME *srcTime, const UInt64 *srcSize,
    const wchar_t *destPath,
    BSTR *destPathResult,
    Int32 *writeAnswer)
{
  UString destPathResultTemp = destPath;

  // RINOK(StringToBstr(destPath, destPathResult));

  *destPathResult = 0;
  *writeAnswer = BoolToInt(false);

  FString destPathSys = us2fs(destPath);
  bool srcIsFolderSpec = IntToBool(srcIsFolder);
  CFileInfo destFileInfo;
  
  if (destFileInfo.Find(destPathSys))
  {
    if (srcIsFolderSpec)
    {
      if (!destFileInfo.IsDir())
      {
        RINOK(MessageError("can not replace file with folder with same name: ", destPathSys));
        return E_ABORT;
      }
      *writeAnswer = BoolToInt(false);
      return S_OK;
    }
  
    if (destFileInfo.IsDir())
    {
      RINOK(MessageError("can not replace folder with file with same name: ", destPathSys));
      return E_FAIL;
    }

    switch (OverwriteMode)
    {
      case NExtract::NOverwriteMode::kSkip:
        return S_OK;
      case NExtract::NOverwriteMode::kAsk:
      {
        Int32 overwiteResult;
        UString destPathSpec = destPath;
        int slashPos = destPathSpec.ReverseFind(L'/');
        #ifdef _WIN32
        int slash1Pos = destPathSpec.ReverseFind(L'\\');
        slashPos = MyMax(slashPos, slash1Pos);
        #endif
        destPathSpec.DeleteFrom(slashPos + 1);
        destPathSpec += fs2us(destFileInfo.Name);

        RINOK(AskOverwrite(
            destPathSpec,
            &destFileInfo.MTime, &destFileInfo.Size,
            srcPath,
            srcTime, srcSize,
            &overwiteResult));
        
        switch (overwiteResult)
        {
          case NOverwriteAnswer::kCancel: return E_ABORT;
          case NOverwriteAnswer::kNo: return S_OK;
          case NOverwriteAnswer::kNoToAll: OverwriteMode = NExtract::NOverwriteMode::kSkip; return S_OK;
          case NOverwriteAnswer::kYes: break;
          case NOverwriteAnswer::kYesToAll: OverwriteMode = NExtract::NOverwriteMode::kOverwrite; break;
          case NOverwriteAnswer::kAutoRename: OverwriteMode = NExtract::NOverwriteMode::kRename; break;
          default:
            return E_FAIL;
        }
      }
    }
    
    if (OverwriteMode == NExtract::NOverwriteMode::kRename)
    {
      if (!AutoRenamePath(destPathSys))
      {
        RINOK(MessageError("can not create name for file: ", destPathSys));
        return E_ABORT;
      }
      destPathResultTemp = fs2us(destPathSys);
    }
    else
      if (!NDir::DeleteFileAlways(destPathSys))
      {
        RINOK(MessageError("can not delete output file: ", destPathSys));
        return E_ABORT;
      }
  }
  *writeAnswer = BoolToInt(true);
  return StringToBstr(destPathResultTemp, destPathResult);
}


STDMETHODIMP CExtractCallbackImp::UseExtractToStream(Int32 *res)
{
  *res = BoolToInt(StreamMode);
  return S_OK;
}

static HRESULT GetTime(IGetProp *getProp, PROPID propID, FILETIME &ft, bool &ftDefined)
{
  ftDefined = false;
  NCOM::CPropVariant prop;
  RINOK(getProp->GetProp(propID, &prop));
  if (prop.vt == VT_FILETIME)
  {
    ft = prop.filetime;
    ftDefined = (ft.dwHighDateTime != 0 || ft.dwLowDateTime != 0);
  }
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}


static HRESULT GetItemBoolProp(IGetProp *getProp, PROPID propID, bool &result)
{
  NCOM::CPropVariant prop;
  result = false;
  RINOK(getProp->GetProp(propID, &prop));
  if (prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}


STDMETHODIMP CExtractCallbackImp::GetStream7(const wchar_t *name,
    Int32 isDir,
    ISequentialOutStream **outStream, Int32 askExtractMode,
    IGetProp *getProp)
{
  COM_TRY_BEGIN
  *outStream = 0;
  _newVirtFileWasAdded = false;
  _hashStreamWasUsed = false;
  _needUpdateStat = false;

  if (_hashStream)
    _hashStreamSpec->ReleaseStream();

  GetItemBoolProp(getProp, kpidIsAltStream, _isAltStream);

  if (!ProcessAltStreams && _isAltStream)
    return S_OK;

  _filePath = name;
  _isFolder = IntToBool(isDir);
  _curSize = 0;
  _curSizeDefined = false;

  UInt64 size = 0;
  bool sizeDefined;
  {
    NCOM::CPropVariant prop;
    RINOK(getProp->GetProp(kpidSize, &prop));
    sizeDefined = ConvertPropVariantToUInt64(prop, size);
  }

  if (sizeDefined)
  {
    _curSize = size;
    _curSizeDefined = true;
  }

  if (askExtractMode != NArchive::NExtract::NAskMode::kExtract &&
      askExtractMode != NArchive::NExtract::NAskMode::kTest)
    return S_OK;

  _needUpdateStat = true;
  
  CMyComPtr<ISequentialOutStream> outStreamLoc;
  
  if (VirtFileSystem && askExtractMode == NArchive::NExtract::NAskMode::kExtract)
  {
    CVirtFile &file = VirtFileSystemSpec->AddNewFile();
    _newVirtFileWasAdded = true;
    file.Name = name;
    file.IsDir = IntToBool(isDir);
    file.IsAltStream = _isAltStream;
    file.Size = 0;

    RINOK(GetTime(getProp, kpidCTime, file.CTime, file.CTimeDefined));
    RINOK(GetTime(getProp, kpidATime, file.ATime, file.ATimeDefined));
    RINOK(GetTime(getProp, kpidMTime, file.MTime, file.MTimeDefined));

    NCOM::CPropVariant prop;
    RINOK(getProp->GetProp(kpidAttrib, &prop));
    if (prop.vt == VT_UI4)
    {
      file.Attrib = prop.ulVal;
      file.AttribDefined = true;
    }
    // else if (isDir) file.Attrib = FILE_ATTRIBUTE_DIRECTORY;

    file.ExpectedSize = 0;
    if (sizeDefined)
      file.ExpectedSize = size;
    outStreamLoc = VirtFileSystem;
  }

  if (_hashStream)
  {
    {
      _hashStreamSpec->SetStream(outStreamLoc);
      outStreamLoc = _hashStream;
      _hashStreamSpec->Init(true);
      _hashStreamWasUsed = true;
    }
  }

  if (outStreamLoc)
    *outStream = outStreamLoc.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CExtractCallbackImp::PrepareOperation7(Int32 askExtractMode)
{
  COM_TRY_BEGIN
  _needUpdateStat = (
      askExtractMode == NArchive::NExtract::NAskMode::kExtract ||
      askExtractMode == NArchive::NExtract::NAskMode::kTest);

  /*
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      if (_testMode)
        askExtractMode = NArchive::NExtract::NAskMode::kTest;
      else
        _extractMode = true;
      break;
  };
  */
  return SetCurrentFilePath2(_filePath);
  COM_TRY_END
}

STDMETHODIMP CExtractCallbackImp::SetOperationResult7(Int32 opRes, bool encrypted)
{
  COM_TRY_BEGIN
  if (VirtFileSystem && _newVirtFileWasAdded)
  {
    // FIXME: probably we must request file size from VirtFileSystem
    // _curSize = VirtFileSystem->GetLastFileSize()
    // _curSizeDefined = true;
    RINOK(VirtFileSystemSpec->CloseMemFile());
  }
  if (_hashStream && _hashStreamWasUsed)
  {
    _hashStreamSpec->_hash->Final(_isFolder, _isAltStream, _filePath);
    _curSize = _hashStreamSpec->GetSize();
    _curSizeDefined = true;
    _hashStreamSpec->ReleaseStream();
    _hashStreamWasUsed = false;
  }
  else if (_hashCalc && _needUpdateStat)
  {
    _hashCalc->SetSize(_curSize);
    _hashCalc->Final(_isFolder, _isAltStream, _filePath);
  }
  return SetOperationResult(opRes, encrypted);
  COM_TRY_END
}


static const size_t k_SizeT_MAX = (size_t)((size_t)0 - 1);

static const UInt32 kBlockSize = ((UInt32)1 << 31);

STDMETHODIMP CVirtFileSystem::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  if (!_fileMode)
  {
    CVirtFile &file = Files.Back();
    size_t rem = file.Data.Size() - (size_t)file.Size;
    bool useMem = true;
    if (rem < size)
    {
      UInt64 b = 0;
      if (file.Data.Size() == 0)
        b = file.ExpectedSize;
      UInt64 a = file.Size + size;
      if (b < a)
        b = a;
      a = (UInt64)file.Data.Size() * 2;
      if (b < a)
        b = a;
      useMem = false;
      if (b <= k_SizeT_MAX && b <= MaxTotalAllocSize)
        useMem = file.Data.ReAlloc_KeepData((size_t)b, (size_t)file.Size);
    }
    if (useMem)
    {
      memcpy(file.Data + file.Size, data, size);
      file.Size += size;
      if (processedSize)
        *processedSize = (UInt32)size;
      return S_OK;
    }
    _fileMode = true;
  }
  RINOK(FlushToDisk(false));
  return _outFileStream->Write(data, size, processedSize);
}

HRESULT CVirtFileSystem::FlushToDisk(bool closeLast)
{
  if (!_outFileStream)
  {
    _outFileStreamSpec = new COutFileStream;
    _outFileStream = _outFileStreamSpec;
  }
  while (_numFlushed < Files.Size())
  {
    const CVirtFile &file = Files[_numFlushed];
    const FString path = DirPrefix + us2fs(GetCorrectFsPath(file.Name));
    if (!_fileIsOpen)
    {
      if (!_outFileStreamSpec->Create(path, false))
      {
        _outFileStream.Release();
        return E_FAIL;
        // MessageBoxMyError(UString(L"Can't create file ") + fs2us(tempFilePath));
      }
      _fileIsOpen = true;
      RINOK(WriteStream(_outFileStream, file.Data, (size_t)file.Size));
    }
    if (_numFlushed == Files.Size() - 1 && !closeLast)
      break;
    if (file.CTimeDefined ||
        file.ATimeDefined ||
        file.MTimeDefined)
      _outFileStreamSpec->SetTime(
          file.CTimeDefined ? &file.CTime : NULL,
          file.ATimeDefined ? &file.ATime : NULL,
          file.MTimeDefined ? &file.MTime : NULL);
    _outFileStreamSpec->Close();
    _numFlushed++;
    _fileIsOpen = false;
    if (file.AttribDefined)
      NDir::SetFileAttrib(path, file.Attrib);
  }
  return S_OK;
}

#endif
