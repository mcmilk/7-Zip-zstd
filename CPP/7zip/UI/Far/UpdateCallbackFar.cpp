// UpdateCallbackFar.cpp

#include "StdAfx.h"

#ifndef Z7_ST
#include "../../../Windows/Synchronization.h"
#endif

#include "../../../Common/StringConvert.h"

#include "FarUtils.h"
#include "UpdateCallbackFar.h"

using namespace NWindows;
using namespace NFar;

#ifndef Z7_ST
static NSynchronization::CCriticalSection g_CriticalSection;
#define MT_LOCK NSynchronization::CCriticalSectionLock lock(g_CriticalSection);
#else
#define MT_LOCK
#endif

static HRESULT CheckBreak2()
{
  return WasEscPressed() ? E_ABORT : S_OK;
}


Z7_COM7F_IMF(CUpdateCallback100Imp::ScanProgress(UInt64 numFolders, UInt64 numFiles, UInt64 totalSize, const wchar_t *path, Int32 /* isDir */))
{
  MT_LOCK

  if (_percent)
  {
    _percent->FilesTotal = numFolders + numFiles;
    _percent->Total = totalSize;
    _percent->Command = "Scanning";
    _percent->FileName = path;
    _percent->Print();
    _percent->Print();
  }
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::ScanError(const wchar_t *path, HRESULT errorCode))
{
  if (ShowSysErrorMessage(errorCode, path) == -1)
    return E_ABORT;
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::SetNumFiles(UInt64 numFiles))
{
  MT_LOCK

  if (_percent)
  {
    _percent->FilesTotal = numFiles;
    _percent->Print();
  }
  return CheckBreak2();
}


Z7_COM7F_IMF(CUpdateCallback100Imp::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */))
{
  return S_OK;
}

Z7_COM7F_IMF(CUpdateCallback100Imp::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */))
{
  MT_LOCK
  return CheckBreak2();
}



Z7_COM7F_IMF(CUpdateCallback100Imp::SetTotal(UInt64 size))
{
  MT_LOCK

  if (_percent)
  {
    _percent->Total = size;
    _percent->Print();
  }
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::SetCompleted(const UInt64 *completeValue))
{
  MT_LOCK

  if (_percent)
  {
    if (completeValue)
      _percent->Completed = *completeValue;
    _percent->Print();
  }
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::CompressOperation(const wchar_t *name))
{
  MT_LOCK
  
  if (_percent)
  {
    _percent->Command = "Adding";
    _percent->FileName = name;
    _percent->Print();
  }
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::DeleteOperation(const wchar_t *name))
{
  MT_LOCK

  if (_percent)
  {
    _percent->Command = "Deleting";
    _percent->FileName = name;
    _percent->Print();
  }
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::OperationResult(Int32 /* opRes */))
{
  MT_LOCK
  
  if (_percent)
  {
    _percent->Files++;
  }
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message))
{
  MT_LOCK

  if (g_StartupInfo.ShowErrorMessage(UnicodeStringToMultiByte(message, CP_OEMCP)) == -1)
    return E_ABORT;
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::OpenFileError(const wchar_t *path, HRESULT errorCode))
{
  if (ShowSysErrorMessage(errorCode, path) == -1)
    return E_ABORT;
  return CheckBreak2();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::ReadingFileError(const wchar_t *path, HRESULT errorCode))
{
  if (ShowSysErrorMessage(errorCode, path) == -1)
    return E_ABORT;
  return CheckBreak2();
}

void SetExtractErrorMessage(Int32 opRes, Int32 encrypted, AString &s);

Z7_COM7F_IMF(CUpdateCallback100Imp::ReportExtractResult(Int32 opRes, Int32 isEncrypted, const wchar_t *name))
{
  MT_LOCK

  if (opRes != NArchive::NExtract::NOperationResult::kOK)
  {
    AString s;
    SetExtractErrorMessage(opRes, isEncrypted, s);
    if (PrintErrorMessage(s, name) == -1)
      return E_ABORT;
  }

  return CheckBreak2();
}


Z7_COM7F_IMF(CUpdateCallback100Imp::ReportUpdateOperation(UInt32 op, const wchar_t *name, Int32 /* isDir */))
{
  const char *s;
  switch (op)
  {
    case NUpdateNotifyOp::kAdd: s = "Adding"; break;
    case NUpdateNotifyOp::kUpdate: s = "Updating"; break;
    case NUpdateNotifyOp::kAnalyze: s = "Analyzing"; break;
    case NUpdateNotifyOp::kReplicate: s = "Replicating"; break;
    case NUpdateNotifyOp::kRepack: s = "Repacking"; break;
    case NUpdateNotifyOp::kSkip: s = "Skipping"; break;
    case NUpdateNotifyOp::kHeader: s = "Header creating"; break;
    case NUpdateNotifyOp::kDelete: s = "Deleting"; break;
    default: s = "Unknown operation";
  }

  MT_LOCK

  if (_percent)
  {
    _percent->Command = s;
    _percent->FileName.Empty();
    if (name)
      _percent->FileName = name;
    _percent->Print();
  }
  
  return CheckBreak2();
}


HRESULT CUpdateCallback100Imp::MoveArc_UpdateStatus()
{
  MT_LOCK

  if (_percent)
  {
    AString s;
    s.Add_UInt64(_arcMoving_percents);
    // status.Add_Space();
    s.Add_Char('%');
    const bool totalDefined = (_arcMoving_total != 0 && _arcMoving_total != (UInt64)(Int64)-1);
    if (_arcMoving_current != 0 || totalDefined)
    {
      s += " : ";
      s.Add_UInt64(_arcMoving_current >> 20);
      s += " MiB";
    }
    if (totalDefined)
    {
      s += " / ";
      s.Add_UInt64((_arcMoving_total + ((1 << 20) - 1)) >> 20);
      s += " MiB";
    }
    s += " : temporary archive moving ...";
    _percent->Command =  s;
    _percent->Print();
  }

  return CheckBreak2();
}


Z7_COM7F_IMF(CUpdateCallback100Imp::MoveArc_Start(const wchar_t *srcTempPath, const wchar_t * /* destFinalPath */ , UInt64 size, Int32 /* updateMode */))
{
  MT_LOCK

  _arcMoving_total = size;
  _arcMoving_current = 0;
  _arcMoving_percents = 0;
  // _arcMoving_updateMode = updateMode;
  // _name2 = fs2us(destFinalPath);
  if (_percent)
    _percent->FileName = srcTempPath;
  return MoveArc_UpdateStatus();
}

Z7_COM7F_IMF(CUpdateCallback100Imp::MoveArc_Progress(UInt64 totalSize, UInt64 currentSize))
{
  UInt64 percents = 0;
  if (totalSize != 0)
  {
    if (totalSize < ((UInt64)1 << 57))
      percents = currentSize * 100 / totalSize;
    else
      percents = currentSize / (totalSize / 100);
  }

#ifdef _WIN32
  // Sleep(300); // for debug
#endif
  if (percents == _arcMoving_percents)
    return CheckBreak2();
  _arcMoving_total = totalSize;
  _arcMoving_current = currentSize;
  _arcMoving_percents = percents;
  // if (_arcMoving_percents > 100) return E_FAIL;
  return MoveArc_UpdateStatus();
}


Z7_COM7F_IMF(CUpdateCallback100Imp::MoveArc_Finish())
{
  // _arcMoving_percents = 0;
  if (_percent)
  {
    _percent->Command.Empty();
    _percent->FileName.Empty();
    _percent->Print();
  }
  return CheckBreak2();
}


Z7_COM7F_IMF(CUpdateCallback100Imp::Before_ArcReopen())
{
  // fixme: we can use Clear_Stop_Status() here
  return CheckBreak2();
}


extern HRESULT GetPassword(UString &password);

Z7_COM7F_IMF(CUpdateCallback100Imp::CryptoGetTextPassword(BSTR *password))
{
  MT_LOCK

  *password = NULL;
  if (!PasswordIsDefined)
  {
    RINOK(GetPassword(Password))
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}

Z7_COM7F_IMF(CUpdateCallback100Imp::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password))
{
  MT_LOCK

  *password = NULL;
  *passwordIsDefined = BoolToInt(PasswordIsDefined);
  if (!PasswordIsDefined)
    return S_OK;
  return StringToBstr(Password, password);
}
