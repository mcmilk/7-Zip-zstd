// Main.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"

#include "Common/CommandLineParser.h"
#include "Common/StringConvert.h"

#include "Windows/DLL.h"
#include "Windows/Error.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/NtCheck.h"
#include "Windows/ResourceString.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../Archive/IArchive.h"
#include "../../UI/Common/Extract.h"
#include "../../UI/Common/ExitCode.h"
#include "../../UI/Explorer/MyMessages.h"
#include "../../UI/GUI/ExtractGUI.h"
#include "../../UI/GUI/ExtractRes.h"

HINSTANCE g_hInstance;

#ifdef UNDER_CE
bool g_LVN_ITEMACTIVATE_Support = true;
#endif

static const wchar_t *kUnknownExceptionMessage = L"ERROR: Unknown Error!";

void ErrorMessageForHRESULT(HRESULT res)
{
  ShowErrorMessage(HResultToMessage(res));
}

int APIENTRY WinMain2()
{
  UString password;
  bool assumeYes = false;
  bool outputFolderDefined = false;
  UString outputFolder;
  UStringVector commandStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);

  #ifndef UNDER_CE
  if (commandStrings.Size() > 0)
    commandStrings.Delete(0);
  #endif

  for (int i = 0; i < commandStrings.Size(); i++)
  {
    const UString &s = commandStrings[i];
    if (s.CompareNoCase(L"-y") == 0)
      assumeYes = true;
    else if (s.Left(2).CompareNoCase(L"-o") == 0)
    {
      outputFolder = s.Mid(2);
      NWindows::NFile::NName::NormalizeDirPathPrefix(outputFolder);
      outputFolderDefined = !outputFolder.IsEmpty();
    }
    else if (s.Left(2).CompareNoCase(L"-p") == 0)
    {
      password = s.Mid(2);
    }
  }

  UString path;
  NWindows::NDLL::MyGetModuleFileName(g_hInstance, path);

  UString fullPath;
  int fileNamePartStartIndex;
  if (!NWindows::NFile::NDirectory::MyGetFullPathName(path, fullPath, fileNamePartStartIndex))
  {
    ShowErrorMessage(L"Error 1329484");
    return 1;
  }

  CCodecs *codecs = new CCodecs;
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  HRESULT result = codecs->Load();
  if (result != S_OK)
  {
    ErrorMessageForHRESULT(result);
    return 1;
  }

  // COpenCallbackGUI openCallback;

  // openCallback.PasswordIsDefined = !password.IsEmpty();
  // openCallback.Password = password;

  CExtractCallbackImp *ecs = new CExtractCallbackImp;
  CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;
  ecs->Init();

  #ifndef _NO_CRYPTO
  ecs->PasswordIsDefined = !password.IsEmpty();
  ecs->Password = password;
  #endif

  CExtractOptions eo;
  eo.OutputDir = outputFolderDefined ? outputFolder :
      fullPath.Left(fileNamePartStartIndex);
  eo.YesToAll = assumeYes;
  eo.OverwriteMode = assumeYes ?
      NExtract::NOverwriteMode::kWithoutPrompt :
      NExtract::NOverwriteMode::kAskBefore;
  eo.PathMode = NExtract::NPathMode::kFullPathnames;
  eo.TestMode = false;
  
  UStringVector v1, v2;
  v1.Add(fullPath);
  v2.Add(fullPath);
  NWildcard::CCensorNode wildcardCensor;
  wildcardCensor.AddItem(true, L"*", true, true, true);

  bool messageWasDisplayed = false;
  result = ExtractGUI(codecs, CIntVector(), v1, v2,
      wildcardCensor, eo, (assumeYes ? false: true), messageWasDisplayed, ecs);

  if (result == S_OK)
  {
    if (!ecs->IsOK())
      return NExitCode::kFatalError;
    return 0;
  }
  if (result == E_ABORT)
    return NExitCode::kUserBreak;
  if (!messageWasDisplayed)
  {
    if (result == S_FALSE)
      ShowErrorMessage(L"Error in archive");
    else
      ErrorMessageForHRESULT(result);
  }
  if (result == E_OUTOFMEMORY)
    return NExitCode::kMemoryError;
  return NExitCode::kFatalError;
}

#define NT_CHECK_FAIL_ACTION ShowErrorMessage(L"Unsupported Windows version"); return NExitCode::kFatalError;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */,
  #ifdef UNDER_CE
  LPWSTR
  #else
  LPSTR
  #endif
  /* lpCmdLine */, int /* nCmdShow */)
{
  g_hInstance = (HINSTANCE)hInstance;

  NT_CHECK

  try
  {
    return WinMain2();
  }
  catch(const CNewException &)
  {
    ErrorMessageForHRESULT(E_OUTOFMEMORY);
    return NExitCode::kMemoryError;
  }
  catch(...)
  {
    ShowErrorMessage(kUnknownExceptionMessage);
    return NExitCode::kFatalError;
  }
}

