// GUI.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/NewHandler.h"
#include "Common/StringConvert.h"
#include "Common/CommandLineParser.h"

#include "Windows/COM.h"
#include "Windows/FileMapping.h"
#include "Windows/FileDir.h"
#include "Windows/Synchronization.h"
#include "Windows/FileName.h"

#include "../../IStream.h"
#include "../../IPassword.h"

// #include "../../Compress/Interface/CompressInterface.h"
// #include "../../FileManager/FolderInterface.h"
#include "../../FileManager/StringUtils.h"

#include "../Resource/Extract/resource.h"
#include "../Agent/Agent.h"
// #include "../Common/FolderArchiveInterface.h"
#include "../Explorer/MyMessages.h"

#include "Test.h"
#include "Extract.h"
#include "Compress.h"

using namespace NWindows;
using namespace NCommandLineParser;

HINSTANCE g_hInstance;

static const int kNumSwitches = 5;

namespace NKey {
enum Enum
{
  // kHelp1 = 0,
  // kHelp2,
  // kDisablePercents,
  kArchiveType,
  // kYes,
  // kPassword,
  // kProperty,
  kOutputDir,
  // kWorkingDir,
  kInclude,
  // kExclude,
  // kUpdate,
  // kRecursed,
  // kSfx,
  // kOverwrite,
  kEmail,
  kShowDialog
  // kMap
};

}

static const int kSomeCludePostStringMinSize = 2; // at least <@|!><N>ame must be
static const int kSomeCludeAfterRecursedPostStringMinSize = 2; // at least <@|!><N>ame must be
static const wchar_t *kRecursedPostCharSet = L"0-";
static const wchar_t *kOverwritePostCharSet = L"asut";

static const CSwitchForm kSwitchForms[kNumSwitches] = 
  {
    // { L"?",  NSwitchType::kSimple, false },
    // { L"H",  NSwitchType::kSimple, false },
    // { L"BD", NSwitchType::kSimple, false },
    { L"T",  NSwitchType::kUnLimitedPostString, false, 1 },
    // { L"Y",  NSwitchType::kSimple, false },
    // { L"P",  NSwitchType::kUnLimitedPostString, false, 0 },
    // { L"M", NSwitchType::kUnLimitedPostString, true, 1 },
    { L"O",  NSwitchType::kUnLimitedPostString, false, 1 },
    // { L"W",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"I",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    // { L"X",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    // { L"U",  NSwitchType::kUnLimitedPostString, true, 1},
    // { L"R",  NSwitchType::kPostChar, false, 0, 0, kRecursedPostCharSet },
    // { L"SFX",  NSwitchType::kUnLimitedPostString, false, 0 },
    // { L"AO",  NSwitchType::kPostChar, false, 1, 1, kOverwritePostCharSet},
    { L"SEML",  NSwitchType::kSimple, false },
    { L"AD",  NSwitchType::kSimple, false }
    // { L"MAP=",  NSwitchType::kUnLimitedPostString, false, 1 }
  };


static bool inline IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

static void MyMessageBoxError(const char *message)
{
  ::MessageBoxA(0, message, "Error", 0);
}

static wchar_t *kIncorrectCommandMessage = L"Incorrect command";

static void ErrorMessage(const wchar_t *message)
{
  MessageBoxW(0, message, L"7-Zip GUI", MB_ICONERROR);
}

static bool ParseIncludeMap(const UString &switchParam, UStringVector &fileNames)
{
  int splitPos = switchParam.Find(L':');
  if (splitPos < 0)
  {
    ErrorMessage(L"Bad switch");
    return false;
  }
  UString mappingName = switchParam.Left(splitPos);
  
  UString switchParam2 = switchParam.Mid(splitPos + 1);
  splitPos = switchParam2.Find(L':');
  if (splitPos < 0)
  {
    ErrorMessage(L"Bad switch");
    return false;
  }
  
  UString mappingSize = switchParam2.Left(splitPos);
  UString eventName = switchParam2.Mid(splitPos + 1);
  
  wchar_t *endptr;
  UINT32 dataSize = wcstoul(mappingSize, &endptr, 10);
  
  {
    CFileMapping fileMapping;
    if (!fileMapping.Open(FILE_MAP_READ, false, 
      GetSystemString(mappingName)))
    {
      // ShowLastErrorMessage(0);
      ErrorMessage(L"Can not open mapping");
      return false;
    }
    LPVOID data = fileMapping.MapViewOfFile(FILE_MAP_READ, 0, dataSize);
    if (data == NULL)
    {
      ErrorMessage(L"MapViewOfFile error");
      return false;
    }
    try
    {
      const wchar_t *curData = (const wchar_t *)data;
      if (*curData != 0)
      {
        ErrorMessage(L"Incorrect mapping data");
        return false;
      }
      UINT32 numChars = dataSize /2;
      UString name;
      
      for (int i = 1; i < numChars; i++)
      {
        wchar_t c = curData[i];
        if (c == L'\0')
        {
          fileNames.Add(name);
          name.Empty();
        }
        else
          name += c;
      }
      if (!name.IsEmpty())
      {
        ErrorMessage(L"data error");
        return false;
      }
    }
    catch(...)
    {
      UnmapViewOfFile(data);
      throw;
    }
    UnmapViewOfFile(data);
  }
  
  {
    NSynchronization::CEvent event;
    event.Open(EVENT_MODIFY_STATE, false, GetSystemString(eventName));
    event.Set();
  }
  return true;
}

static bool ParseIncludeSwitches(CParser &parser, UStringVector &fileNames)
{
  if (parser[NKey::kInclude].ThereIs)
  {
    for (int i = 0; i < parser[NKey::kInclude].PostStrings.Size(); i++)
    {
      UString switchParam = parser[NKey::kInclude].PostStrings[i];
      if (switchParam.Length() < 1)
        return false;
      if (switchParam[0] == L'#')
      {
        if (!ParseIncludeMap(switchParam.Mid(1), fileNames))
          return false;
      }
      else if (switchParam[0] == L'!')
      {
        fileNames.Add(switchParam.Mid(1));
      }
      else 
      {
        ErrorMessage(L"Incorrect command");
        return false;
      }
    }
  }
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;
  for (int i = 2; i < nonSwitchStrings.Size(); i++)
  {
    // ErrorMessage(nonSwitchStrings[i]);
    fileNames.Add(nonSwitchStrings[i]);
  }
  return true;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
  g_hInstance = hInstance;
  InitCommonControls();

  #ifdef UNICODE
  if (!IsItWindowsNT())
  {
    // g_StdOut << "This program requires Windows NT/2000/XP";
    return 0;
  }
  #endif
  // setlocale(LC_COLLATE, ".ACP");
  int result = 0;
  try
  {
    UStringVector commandStrings;
    SplitCommandLine(GetCommandLineW(), commandStrings);
    if (commandStrings.Size() > 0)
      commandStrings.Delete(0);
    CParser parser(kNumSwitches);
    try
    {
      parser.ParseStrings(kSwitchForms, commandStrings);
    }
    catch(...) 
    {
      MyMessageBox(kIncorrectCommandMessage);
      return 1;
    }

    const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;
    int numNonSwitchStrings = nonSwitchStrings.Size();

    if(numNonSwitchStrings < 1)  
    {
      MyMessageBox(kIncorrectCommandMessage);
      return 1;
    }

    UString command = nonSwitchStrings[0];

    if (command == L"t")
    {
      if(numNonSwitchStrings < 2)  
      {
        MyMessageBox(kIncorrectCommandMessage);
        return 1;
      }
      UString archiveName = nonSwitchStrings[1];
      HRESULT result = TestArchive(0, archiveName);
      if (result == S_FALSE)
        MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      else if (result != S_OK)
        ShowErrorMessage(0, result);
    }
    else if (command == L"x")
    {
      if(numNonSwitchStrings < 2)  
      {
        MyMessageBox(kIncorrectCommandMessage);
        return 1;
      }
      UString archiveName = nonSwitchStrings[1];

      UString outputDir;
      bool outputDirDefined = parser[NKey::kOutputDir].ThereIs;
      if(outputDirDefined)
        outputDir = parser[NKey::kOutputDir].PostStrings[0];
      else
        NFile::NDirectory::MyGetCurrentDirectory(outputDir);
      NFile::NName::NormalizeDirPathPrefix(outputDir);
      HRESULT result = ExtractArchive(0, archiveName, 
          false, parser[NKey::kShowDialog].ThereIs, outputDir);
      if (result == S_FALSE)
        MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      else if (result != S_OK)
        ShowErrorMessage(0, result);
    }
    else if (command == L"a")
    {
      if(numNonSwitchStrings < 2)  
      {
        MyMessageBox(kIncorrectCommandMessage);
        return 1;
      }
      const UString &archiveName = nonSwitchStrings[1];

      UString mapString, emailString;
      bool emailMode = parser[NKey::kEmail].ThereIs;
      UStringVector fileNames;
      if (!ParseIncludeSwitches(parser, fileNames))
        return 1;
      if (fileNames.Size() == 0)
      {
        ErrorMessage(L"Incorrect command: No files");
        return 1;
      }
      UString archiveType = L"7z";;
      if (parser[NKey::kArchiveType].ThereIs)
        archiveType = parser[NKey::kArchiveType].PostStrings.Front();
      HRESULT result = CompressArchive(archiveName, fileNames, 
          archiveType, emailMode, parser[NKey::kShowDialog].ThereIs);
      // if (result != S_OK)
      //   ShowErrorMessage(result);
    }
    else
    {
      ErrorMessage(L"Use correct command");
      return 0;
    }
    return 0;
  }
  catch(const CNewException &)
  {
    // MyMessageBoxError(kMemoryExceptionMessage);
    return 1;
  }
  catch(...)
  {
    // g_StdOut << kUnknownExceptionMessage;
    return 2;
  }
  return result;
}



