// MainCommandLineAr.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/StringConvert.h"
#include "Common/Random.h"

#include "Windows/FileDir.h"
#include "Windows/FileIO.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"

#include "Interface/CryptoInterface.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../Format/Common/ArchiveInterface.h"
#include "ExtractEngine.h"
#include "../../Explorer/MyMessages.h"
#include "Common/TextConfig.h"

HINSTANCE g_hInstance;

using namespace NWindows;

static LPCTSTR kTempDirPrefix = _T("7zS"); 

const LPCWSTR aDefaultExt = L".exe";

static bool ReadDataString(LPCTSTR aFileName, LPCSTR aStartID, 
    LPCSTR anEndID, AString &aString)
{
  aString.Empty();
  NFile::NIO::CInFile anInFile;
  if (!anInFile.Open(aFileName))
    return false;
  const kBufferSize = (1 << 12);

  BYTE aBuffer[kBufferSize];
  int aSignatureStartSize = strlen(aStartID);
  int aSignatureEndSize = strlen(anEndID);
  
  UINT32 aNumBytesPrev = 0;
  bool aWriteMode = false;
  UINT64 aPosTotal = 0;
  while(true)
  {
    if (aPosTotal > (1 << 20))
      return (aString.IsEmpty());
    UINT32 aNumReadBytes = kBufferSize - aNumBytesPrev;
    UINT32 aProcessedSize;
    if (!anInFile.Read(aBuffer + aNumBytesPrev, aNumReadBytes, aProcessedSize))
      return false;
    if (aProcessedSize == 0)
      return true;
    UINT32 aNumBytesInBuffer = aNumBytesPrev + aProcessedSize;
    UINT32 aPos = 0;
    while (true)
    { 
      if (aWriteMode)
      {
        if (aPos > aNumBytesInBuffer - aSignatureEndSize)
          break;
        if (memcmp(aBuffer + aPos, anEndID, aSignatureEndSize) == 0)
          return true;
        char aByte = aBuffer[aPos];
        if (aByte == 0)
          return false;
        aString += aByte;
        aPos++;
      }
      else
      {
        if (aPos > aNumBytesInBuffer - aSignatureStartSize)
          break;
        if (memcmp(aBuffer + aPos, aStartID, aSignatureStartSize) == 0)
        {
          aWriteMode = true;
          aPos += aSignatureStartSize;
        }
        else
          aPos++;
      }
    }
    aNumBytesPrev = aNumBytesInBuffer - aPos;
    aPosTotal += aPos;
    memmove(aBuffer, aBuffer + aPos, aNumBytesPrev);
  }
}

static void GetArchiveName(
    const UString &aCommandLine, 
    UString &anArchiveName, 
    UString &aSwitches)
{
  anArchiveName.Empty();
  aSwitches.Empty();
  bool aQuoteMode = false;
  for (int i = 0; i < aCommandLine.Length(); i++)
  {
    wchar_t aChar = aCommandLine[i];
    if (aChar == L'\"')
      aQuoteMode = !aQuoteMode;
    else if (aChar == L' ' && !aQuoteMode)
    {
      if (!aQuoteMode)
      {
        i++;
        break;
      }
    }
    else 
      anArchiveName += aChar;
  }
  aSwitches = aCommandLine.Mid(i);
}

static char aStartID[] = ",!@Install@!UTF-8!";
static char anEndID[] = ",!@InstallEnd@!";

class CInstallIDInit
{
public:
  CInstallIDInit()
  {
    aStartID[0] = ';';
    anEndID[0] = ';';
  };
} g_CInstallIDInit;


class CCurrentDirRestorer
{
  CSysString m_CurrentDirectory;
public:
  CCurrentDirRestorer()
    { NWindows::NFile::NDirectory::MyGetCurrentDirectory(m_CurrentDirectory); }
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
  UString anArchiveName, aSwitches;
  GetArchiveName(GetCommandLineW(), anArchiveName, aSwitches);
  CSysString aFullPath;

  if (anArchiveName.Right(4).CompareNoCase(aDefaultExt) != 0)
    anArchiveName += aDefaultExt;

  if (!NWindows::NFile::NDirectory::MyGetFullPathName(
      GetSystemString(anArchiveName), aFullPath))
  {
    MessageBox(NULL, "can't get archive name", "7-Zip", 0);
    return 1;
  }

  AString aConfig;
  if (!ReadDataString(aFullPath, aStartID, anEndID, aConfig))
  {
    MessageBox(NULL, "Can't load config info", "7-Zip", 0);
    return 1;
  }
  aSwitches.Trim();
  bool anAssumeYes = false;
  if (aSwitches.Left(2) == UString(L"-y"))
  {
    anAssumeYes = true;
    aSwitches = aSwitches.Mid(2);
    aSwitches.Trim();
  }

  UString anAppLaunched;
  if (!aConfig.IsEmpty())
  {
    CObjectVector<CTextConfigPair> aPairs;
    if (!GetTextConfig(aConfig, aPairs))
    {
      MessageBox(NULL, "Config failed", "7-Zip", 0);
      return 1;
    }
    UString aFriendlyName = GetTextConfigValue(aPairs, L"Title");
    UString anInstallPrompt = GetTextConfigValue(aPairs, L"BeginPrompt");

    if (!anInstallPrompt.IsEmpty() && !anAssumeYes)
    {
      if (MessageBoxW(0, anInstallPrompt, aFriendlyName, MB_YESNO | 
          MB_ICONQUESTION) != IDYES)
        return 0;
    }
    anAppLaunched = GetTextConfigValue(aPairs, L"RunProgram");
  }

  NFile::NDirectory::CTempDirectory aTempDir;
  if (!aTempDir.Create(kTempDirPrefix))
  {
    MessageBox(0, "Can not create temp folder archive", "7-Zip", 0);
    return 1;
  }

  HRESULT aResult = ExtractArchive(aFullPath, aTempDir.GetPath());
  if (aResult != S_OK)
  {
    if (aResult == S_FALSE)
      MessageBox(0, "Can not open archive", "7-Zip", 0);
    else 
      ShowErrorMessage(aResult);
    return  1;
  }

  CCurrentDirRestorer aCurrentDirRestorer;

  if (!SetCurrentDirectory(aTempDir.GetPath()))
    return 1;

  
  if (anAppLaunched.IsEmpty())
  {
    anAppLaunched = L"Setup.exe";
    if (!NFile::NFind::DoesFileExist(GetSystemString(anAppLaunched)))
      return 1;
  }
  STARTUPINFO aStartupInfo;
  aStartupInfo.cb = sizeof(aStartupInfo);
  aStartupInfo.lpReserved = 0;
  aStartupInfo.lpDesktop = 0;
  aStartupInfo.lpTitle = 0;
  aStartupInfo.dwFlags = 0;
  aStartupInfo.cbReserved2 = 0;
  aStartupInfo.lpReserved2 = 0;
  
  PROCESS_INFORMATION aProcessInformation;


  CSysString aShortPath;
  if (!NFile::NDirectory::MyGetShortPathName(aTempDir.GetPath(), aShortPath))
    return 1;

  UString anAppLaunchedSysU = anAppLaunched;
  anAppLaunchedSysU.Replace(TEXT(L"%%T"), GetUnicodeString(aShortPath));

  anAppLaunchedSysU += L' ';
  anAppLaunchedSysU += aSwitches;
  
  CSysString aTempDirPathNormalized = aShortPath;
  NFile::NName::NormalizeDirPathPrefix(aShortPath);
  // CSysString anAppLaunchedSys = aShortPath + GetSystemString(anAppLaunchedSysU);
  CSysString anAppLaunchedSys = CSysString(TEXT(".\\")) + GetSystemString(anAppLaunchedSysU);
  
  BOOL aCreateResult = CreateProcess(NULL, (LPTSTR)(LPCTSTR)anAppLaunchedSys, 
    NULL, NULL, FALSE, 0, NULL, NULL /*aTempDir.GetPath() */, 
    &aStartupInfo, &aProcessInformation);
  if (aCreateResult == 0)
  {
    ShowLastErrorMessage();
    return 1;
  }
  WaitForSingleObject(aProcessInformation.hProcess, INFINITE);
  ::CloseHandle(aProcessInformation.hThread);
  ::CloseHandle(aProcessInformation.hProcess);
  return 0;
}

