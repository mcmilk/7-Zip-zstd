// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/StringConvert.h"
#include "Common/Random.h"
#include "Common/TextConfig.h"
#include "Common/CommandLineParser.h"

#include "Windows/FileDir.h"
#include "Windows/FileIO.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/DLL.h"

#include "../../IPassword.h"
#include "../../ICoder.h"
#include "../../Archive/IArchive.h"
#include "../../UI/Explorer/MyMessages.h"

#include "ExtractEngine.h"

HINSTANCE g_hInstance;

using namespace NWindows;

static LPCTSTR kTempDirPrefix = TEXT("7zS"); 

static bool ReadDataString(LPCWSTR fileName, LPCSTR startID, 
    LPCSTR endID, AString &stringResult)
{
  stringResult.Empty();
  NFile::NIO::CInFile inFile;
  if (!inFile.Open(fileName))
    return false;
  const int kBufferSize = (1 << 12);

  BYTE buffer[kBufferSize];
  int signatureStartSize = lstrlenA(startID);
  int signatureEndSize = lstrlenA(endID);
  
  UINT32 numBytesPrev = 0;
  bool writeMode = false;
  UINT64 posTotal = 0;
  while(true)
  {
    if (posTotal > (1 << 20))
      return (stringResult.IsEmpty());
    UINT32 numReadBytes = kBufferSize - numBytesPrev;
    UINT32 processedSize;
    if (!inFile.Read(buffer + numBytesPrev, numReadBytes, processedSize))
      return false;
    if (processedSize == 0)
      return true;
    UINT32 numBytesInBuffer = numBytesPrev + processedSize;
    UINT32 pos = 0;
    while (true)
    { 
      if (writeMode)
      {
        if (pos > numBytesInBuffer - signatureEndSize)
          break;
        if (memcmp(buffer + pos, endID, signatureEndSize) == 0)
          return true;
        char b = buffer[pos];
        if (b == 0)
          return false;
        stringResult += b;
        pos++;
      }
      else
      {
        if (pos > numBytesInBuffer - signatureStartSize)
          break;
        if (memcmp(buffer + pos, startID, signatureStartSize) == 0)
        {
          writeMode = true;
          pos += signatureStartSize;
        }
        else
          pos++;
      }
    }
    numBytesPrev = numBytesInBuffer - pos;
    posTotal += pos;
    memmove(buffer, buffer + pos, numBytesPrev);
  }
}

static char kStartID[] = ",!@Install@!UTF-8!";
static char kEndID[] = ",!@InstallEnd@!";

class CInstallIDInit
{
public:
  CInstallIDInit()
  {
    kStartID[0] = ';';
    kEndID[0] = ';';
  };
} g_CInstallIDInit;


class CCurrentDirRestorer
{
  CSysString m_CurrentDirectory;
public:
  CCurrentDirRestorer()
    { NFile::NDirectory::MyGetCurrentDirectory(m_CurrentDirectory); }
  ~CCurrentDirRestorer()
    { RestoreDirectory();}
  bool RestoreDirectory()
    { return BOOLToBool(::SetCurrentDirectory(m_CurrentDirectory)); }
};

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  InitCommonControls();
  g_hInstance = (HINSTANCE)hInstance;
  UString archiveName, switches;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), archiveName, switches);

  UString fullPath;
  NDLL::MyGetModuleFileName(g_hInstance, fullPath);

  AString config;
  if (!ReadDataString(fullPath, kStartID, kEndID, config))
  {
    MyMessageBox(L"Can't load config info");
    return 1;
  }
  switches.Trim();
  bool assumeYes = false;
  if (switches.Left(2).CompareNoCase(UString(L"-y")) == 0)
  {
    assumeYes = true;
    switches = switches.Mid(2);
    switches.Trim();
  }

  UString appLaunched;
  if (!config.IsEmpty())
  {
    CObjectVector<CTextConfigPair> pairs;
    if (!GetTextConfig(config, pairs))
    {
      MyMessageBox(L"Config failed");
      return 1;
    }
    UString friendlyName = GetTextConfigValue(pairs, L"Title");
    UString installPrompt = GetTextConfigValue(pairs, L"BeginPrompt");

    if (!installPrompt.IsEmpty() && !assumeYes)
    {
      if (MessageBoxW(0, installPrompt, friendlyName, MB_YESNO | 
          MB_ICONQUESTION) != IDYES)
        return 0;
    }
    appLaunched = GetTextConfigValue(pairs, L"RunProgram");
  }

  NFile::NDirectory::CTempDirectory tempDir;
  if (!tempDir.Create(kTempDirPrefix))
  {
    MyMessageBox(L"Can not create temp folder archive");
    return 1;
  }

  HRESULT result = ExtractArchive(fullPath, GetUnicodeString(tempDir.GetPath()));
  if (result != S_OK)
  {
    if (result == S_FALSE)
      MyMessageBox(L"Can not open archive");
    else if (result != E_ABORT)
      ShowErrorMessage(result);
    return  1;
  }

  CCurrentDirRestorer currentDirRestorer;

  if (!SetCurrentDirectory(tempDir.GetPath()))
    return 1;

  
  if (appLaunched.IsEmpty())
  {
    appLaunched = L"Setup.exe";
    if (!NFile::NFind::DoesFileExist(GetSystemString(appLaunched)))
      return 1;
  }
  STARTUPINFO startupInfo;
  startupInfo.cb = sizeof(startupInfo);
  startupInfo.lpReserved = 0;
  startupInfo.lpDesktop = 0;
  startupInfo.lpTitle = 0;
  startupInfo.dwFlags = 0;
  startupInfo.cbReserved2 = 0;
  startupInfo.lpReserved2 = 0;
  
  PROCESS_INFORMATION processInformation;

  CSysString shortPath;
  if (!NFile::NDirectory::MyGetShortPathName(tempDir.GetPath(), shortPath))
    return 1;

  UString appLaunchedSysU = appLaunched;
  appLaunchedSysU.Replace(TEXT(L"%%T"), GetUnicodeString(shortPath));

  appLaunchedSysU += L' ';
  appLaunchedSysU += switches;
  
  CSysString tempDirPathNormalized = shortPath;
  NFile::NName::NormalizeDirPathPrefix(shortPath);
  // CSysString appLaunchedSys = shortPath + GetSystemString(appLaunchedSysU);
  CSysString appLaunchedSys = CSysString(TEXT(".\\")) + GetSystemString(appLaunchedSysU);
  
  BOOL createResult = CreateProcess(NULL, (LPTSTR)(LPCTSTR)appLaunchedSys, 
      NULL, NULL, FALSE, 0, NULL, NULL /*tempDir.GetPath() */, 
      &startupInfo, &processInformation);
  if (createResult == 0)
  {
    ShowLastErrorMessage();
    return 1;
  }
  WaitForSingleObject(processInformation.hProcess, INFINITE);
  ::CloseHandle(processInformation.hThread);
  ::CloseHandle(processInformation.hProcess);
  return 0;
}
