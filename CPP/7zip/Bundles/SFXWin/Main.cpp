// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/StringConvert.h"
#include "Common/CommandLineParser.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/DLL.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../Archive/IArchive.h"
#include "../../UI/Common/Extract.h"
#include "../../UI/Common/ExitCode.h"
#include "../../UI/Explorer/MyMessages.h"
#include "../../UI/GUI/ExtractGUI.h"

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

static const wchar_t *kMemoryExceptionMessage = L"ERROR: Can't allocate required memory!";
static const wchar_t *kUnknownExceptionMessage = L"ERROR: Unknown Error!";

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
    MyMessageBox(L"Error 1329484");
    return 1;
  }

  CCodecs *codecs = new CCodecs;
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  HRESULT result = codecs->Load();
  if (result != S_OK)
  {
    ShowErrorMessage(0, result);
    return S_OK;
  }

  COpenCallbackGUI openCallback;

  openCallback.PasswordIsDefined = !password.IsEmpty();
  openCallback.Password = password;

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

  result = ExtractGUI(codecs, v1, v2,
    wildcardCensor, eo, (assumeYes ? false: true), &openCallback, ecs);

  /*
  HRESULT result = ExtractArchive(NULL, path, assumeYes, !assumeYes, 
      outputFolderDefined ? outputFolder : 
      fullPath.Left(fileNamePartStartIndex));
  */
  if (result == S_OK)
  {
    return 0;
  }
  if (result == E_ABORT)
    return NExitCode::kUserBreak;
  if (result == S_FALSE)
    MyMessageBox(L"Error in archive");
  else
    ShowErrorMessage(0, result);
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
    MyMessageBox(kMemoryExceptionMessage);
    return (NExitCode::kMemoryError);
  }
  catch(...)
  {
    MyMessageBox(kUnknownExceptionMessage);
    return NExitCode::kFatalError;
  }
}

