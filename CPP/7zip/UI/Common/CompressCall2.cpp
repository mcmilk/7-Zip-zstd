// CompressCall.cpp

#include "StdAfx.h"

#include "Common/MyException.h"

#include "../../UI/common/ArchiveCommandLine.h"

#include "../../UI/GUI/BenchmarkDialog.h"
#include "../../UI/GUI/ExtractGUI.h"
#include "../../UI/GUI/UpdateGUI.h"

#include "../../UI/GUI/ExtractRes.h"

#include "CompressCall.h"

#define MY_TRY_BEGIN try {
#define MY_TRY_FINISH } \
  catch(CSystemException &e) { result = e.ErrorCode; } \
  catch(...) { result = E_FAIL; } \
  if (result != S_OK && result != E_ABORT) \
    ErrorMessageHRESULT(result);

#define CREATE_CODECS \
  CCodecs *codecs = new CCodecs; \
  CMyComPtr<IUnknown> compressCodecsInfo = codecs; \
  result = codecs->Load(); \
  if (result != S_OK) \
    throw CSystemException(result);

UString GetQuotedString(const UString &s)
{
  return UString(L'\"') + s + UString(L'\"');
}

static void ErrorMessage(LPCWSTR message)
{
  MessageBoxW(g_HWND, message, L"7-Zip", MB_ICONERROR);
}

static void ErrorMessageHRESULT(HRESULT res)
{
  ErrorMessage(HResultToMessage(res));
}

static void ErrorLangMessage(UINT resourceID, UInt32 langID)
{
  ErrorMessage(LangString(resourceID, langID));
}

HRESULT CompressFiles(
    const UString &arcPathPrefix,
    const UString &arcName,
    const UString &arcType,
    const UStringVector &names,
    bool email, bool showDialog, bool /* waitFinish */)
{
  HRESULT result;
  MY_TRY_BEGIN
  CREATE_CODECS

  CUpdateCallbackGUI callback;
  
  callback.Init();

  CUpdateOptions uo;
  uo.EMailMode = email;
  uo.SetAddActionCommand();

  CIntVector formatIndices;
  if (!codecs->FindFormatForArchiveType(arcType, formatIndices))
  {
    ErrorLangMessage(IDS_UNSUPPORTED_ARCHIVE_TYPE, 0x0200060D);
    return E_FAIL;
  }
  if (!uo.Init(codecs, formatIndices, arcPathPrefix + arcName))
  {
    ErrorLangMessage(IDS_UPDATE_NOT_SUPPORTED, 0x02000601);
    return E_FAIL;
  }

  NWildcard::CCensor censor;
  for (int i = 0; i < names.Size(); i++)
    censor.AddItem(true, names[i], false);

  bool messageWasDisplayed = false;
  result = UpdateGUI(codecs, censor, uo, showDialog, messageWasDisplayed, &callback, g_HWND);
  
  if (result != S_OK)
  {
    if (result != E_ABORT && messageWasDisplayed)
      return E_FAIL;
    throw CSystemException(result);
  }
  if (callback.FailedFiles.Size() > 0)
  {
    if (!messageWasDisplayed)
      throw CSystemException(E_FAIL);
    return E_FAIL;
  }
  MY_TRY_FINISH
  return S_OK;
}

static HRESULT ExtractGroupCommand(const UStringVector &arcPaths,
    bool showDialog, const UString &outFolder, bool testMode)
{
  HRESULT result;
  MY_TRY_BEGIN
  CREATE_CODECS

  CExtractOptions eo;
  eo.OutputDir = outFolder;
  eo.TestMode = testMode;

  CExtractCallbackImp *ecs = new CExtractCallbackImp;
  CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;
  
  ecs->Init();
  
  // eo.CalcCrc = options.CalcCrc;
  
  UStringVector arcPathsSorted;
  UStringVector arcFullPathsSorted;
  {
    NWildcard::CCensor acrCensor;
    for (int i = 0; i < arcPaths.Size(); i++)
      acrCensor.AddItem(true, arcPaths[i], false);
    EnumerateDirItemsAndSort(acrCensor, arcPathsSorted, arcFullPathsSorted);
  }
  
  CIntVector formatIndices;

  NWildcard::CCensor censor;
  censor.AddItem(true, L"*", false);
  
  bool messageWasDisplayed = false;
  result = ExtractGUI(codecs, formatIndices, arcPathsSorted, arcFullPathsSorted,
      censor.Pairs.Front().Head, eo, showDialog, messageWasDisplayed, ecs, g_HWND);
  if (result != S_OK)
  {
    if (result != E_ABORT && messageWasDisplayed)
      return E_FAIL;
    throw CSystemException(result);
  }
  return ecs->IsOK() ? S_OK : E_FAIL;
  MY_TRY_FINISH
  return result;
}

HRESULT ExtractArchives(const UStringVector &arcPaths, const UString &outFolder, bool showDialog)
{
  return ExtractGroupCommand(arcPaths, showDialog, outFolder, false);
}

HRESULT TestArchives(const UStringVector &arcPaths)
{
  return ExtractGroupCommand(arcPaths, true, UString(), true);
}

HRESULT Benchmark()
{
  HRESULT result;
  MY_TRY_BEGIN
  CREATE_CODECS

  #ifdef EXTERNAL_CODECS
  CObjectVector<CCodecInfoEx> externalCodecs;
  RINOK(LoadExternalCodecs(codecs, externalCodecs));
  #endif
  result = Benchmark(
      #ifdef EXTERNAL_CODECS
      codecs, &externalCodecs,
      #endif
      (UInt32)-1, (UInt32)-1, g_HWND);
  MY_TRY_FINISH
  return result;
}
