// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/CommandLineParser.h"
#include "Common/StringConvert.h"

#include "Windows/DLL.h"
#include "Windows/Error.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
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
#ifndef _UNICODE
bool g_IsNT = false;
static inline bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo))
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif

static const wchar_t *kUnknownExceptionMessage = L"ERROR: Unknown Error!";

void ErrorMessageForHRESULT(HRESULT res)
{
  UString s;
  if (res == E_OUTOFMEMORY)
    s = NWindows::MyLoadStringW(IDS_MEM_ERROR);
  else
    s = NWindows::NError::MyFormatMessageW(res);
  ShowErrorMessage(s);
}

int APIENTRY WinMain2()
{
  UString password;
  bool assumeYes = false;
  bool outputFolderDefined = false;
  UString outputFolder;
  UStringVector subStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), subStrings);
  for (int i = 1; i < subStrings.Size(); i++)
  {
    const UString &s = subStrings[i];
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
  ecs->PasswordIsDefined = !password.IsEmpty();
  ecs->Password = password;
  
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

  result = ExtractGUI(codecs, CIntVector(), v1, v2,
    wildcardCensor, eo, (assumeYes ? false: true), ecs);

  if (result == S_OK)
  {
    if (ecs->Messages.Size() > 0 || ecs->NumArchiveErrors != 0)
      return NExitCode::kFatalError;
    return 0;
  }
  if (result == E_ABORT)
    return NExitCode::kUserBreak;
  if (result == S_FALSE)
    ShowErrorMessage(L"Error in archive");
  else
    ErrorMessageForHRESULT(result);
  if (result == E_OUTOFMEMORY)
    return NExitCode::kMemoryError;
  return NExitCode::kFatalError;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */, LPSTR /* lpCmdLine */, int /* nCmdShow */)
{
  g_hInstance = (HINSTANCE)hInstance;
  #ifndef _UNICODE
  g_IsNT = IsItWindowsNT();
  #endif
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

