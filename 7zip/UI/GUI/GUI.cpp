// GUI.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/NewHandler.h"
#include "Common/StringConvert.h"
#include "Common/CommandLineParser.h"

#include "Windows/COM.h"
#include "Windows/FileMapping.h"
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

static const int kNumSwitches = 18;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kDisablePercents,
  kArchiveType,
  kYes,
  kPassword,
  kProperty,
  kOutputDir,
  kWorkingDir,
  kInclude,
  kExclude,
  kUpdate,
  kRecursed,
  kSfx,
  kOverwrite,
  kEmail,
  kShowDialog,
  kMap
};

}

static const int kSomeCludePostStringMinSize = 2; // at least <@|!><N>ame must be
static const int kSomeCludeAfterRecursedPostStringMinSize = 2; // at least <@|!><N>ame must be
static const wchar_t *kRecursedPostCharSet = L"0-";
static const wchar_t *kOverwritePostCharSet = L"asu";

static const CSwitchForm kSwitchForms[kNumSwitches] = 
  {
    { L"?",  NSwitchType::kSimple, false },
    { L"H",  NSwitchType::kSimple, false },
    { L"BD", NSwitchType::kSimple, false },
    { L"T",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"Y",  NSwitchType::kSimple, false },
    { L"P",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"M", NSwitchType::kUnLimitedPostString, true, 1 },
    { L"O",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"W",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"I",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"X",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"U",  NSwitchType::kUnLimitedPostString, true, 1},
    { L"R",  NSwitchType::kPostChar, false, 0, 0, kRecursedPostCharSet },
    { L"SFX",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"AO",  NSwitchType::kPostChar, false, 1, 1, kOverwritePostCharSet},
    { L"EMAIL",  NSwitchType::kSimple, false },
    { L"SHOWDIALOG",  NSwitchType::kSimple, false },
    { L"MAP=",  NSwitchType::kUnLimitedPostString, false, 1 }
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

void GetArguments(UStringVector &strings)
{
  UString s = GetCommandLineW();
  // MessageBoxW(0, s, 0, 0);
  s.Trim();
  UString s1, s2;
  strings.Clear();
  while (true)
  {
    SplitStringToTwoStrings(s, s1, s2);
    s1.Trim();
    s2.Trim();
    if (!s1.IsEmpty())
      strings.Add(s1);
    if (s2.IsEmpty())
      return;
    s = s2;
  }
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
  g_hInstance = hInstance;

  #ifdef UNICODE
  if (!IsItWindowsNT())
  {
    // g_StdOut << "This program requires Windows NT/2000/XP";
    return 0;
  }
  #endif
  // setlocale(LC_COLLATE, ".ACP");
  int result = 0;
  // CNewHandlerSetter newHandlerSetter;
  // NCOM::CComInitializer comInitializer;
  try
  {
    UStringVector commandStrings;
    GetArguments(commandStrings);
    if (commandStrings.Size() > 0)
      commandStrings.Delete(0);
    CParser parser(kNumSwitches);
    try
    {
      parser.ParseStrings(kSwitchForms, commandStrings);
    }
    catch(...) 
    {
      MyMessageBox(L"Incorrect command");
      return 1;
    }

    const UStringVector &nonSwitchStrings = parser._nonSwitchStrings;
    int numNonSwitchStrings = nonSwitchStrings.Size();

    if(numNonSwitchStrings < 1)  
    {
      MyMessageBox(L"Incorrect command");
      return 1;
    }

    UString command = nonSwitchStrings[0];

    if (command == L"t")
    {
      if(numNonSwitchStrings < 2)  
      {
        MyMessageBox(L"Incorrect command");
        return 1;
      }
      CSysString archiveName = GetSystemString(nonSwitchStrings[1]);
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
        MyMessageBox(L"Incorrect command");
        return 1;
      }
      CSysString archiveName = GetSystemString(nonSwitchStrings[1]);

      CSysString outputDir;
      bool outputDirDefined = parser[NKey::kOutputDir].ThereIs;
      if(outputDirDefined)
      {
        outputDir = GetSystemString(parser[NKey::kOutputDir].PostStrings[0]);
        NFile::NName::NormalizeDirPathPrefix(outputDir);
      }
      HRESULT result = ExtractArchive(0, archiveName, 
          false, parser[NKey::kShowDialog].ThereIs, outputDir);
      if (result == S_FALSE)
        MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      else if (result != S_OK)
        ShowErrorMessage(0, result);
    }
    else if (command == L"a")
    {
      if(numNonSwitchStrings < 1 || numNonSwitchStrings > 2)  
      {
        MyMessageBox(L"Incorrect command");
        return 1;
      }
      CSysString archiveName;
      if (numNonSwitchStrings == 2)
        archiveName = GetSystemString(nonSwitchStrings[1]);

      UString mapString, emailString;
      bool emailMode = parser[NKey::kEmail].ThereIs;
      if (!parser[NKey::kMap].ThereIs)
      {
        MessageBoxW(0, L"Bad switch: map is not defined", L"7-Zip GUI", 0);
        return 1;
      }
      UString switchParam = parser[NKey::kMap].PostStrings.Front();
      
      int splitPos = switchParam.Find(L':');
      if (splitPos < 0)
      {
        MessageBoxW(0, L"Bad switch", L"7-Zip GUI", 0);
        return 1;
      }
      UString mappingName = switchParam.Left(splitPos);

      UString switchParam2 = switchParam.Mid(splitPos + 1);
      splitPos = switchParam2.Find(L':');
      if (splitPos < 0)
      {
        MessageBoxW(0, L"Bad switch", L"7-Zip GUI", 0);
        return 1;
      }

      UString mappingSize = switchParam2.Left(splitPos);
      UString eventName = switchParam2.Mid(splitPos + 1);

      wchar_t *endptr;
      UINT32 dataSize = wcstoul(mappingSize, &endptr, 10);

      CSysStringVector fileNames;

      {
        CFileMapping fileMapping;
        if (!fileMapping.Open(FILE_MAP_READ, false, 
            GetSystemString(mappingName)))
        {
          // ShowLastErrorMessage(0);
          MessageBoxW(0, L"Can not open mapping", L"7-Zip GUI", 0);
          return 1;
        }
        LPVOID data = fileMapping.MapViewOfFile(FILE_MAP_READ, 0, dataSize);
        if (data == NULL)
        {
          MessageBoxW(0, L"MapViewOfFile error", L"7-Zip GUI", 0);
          return 1;
        }
        try
        {
          const wchar_t *curData = (const wchar_t *)data;
          if (*curData != 0)
          {
            MessageBoxW(0, L"Incorrect mapping data", L"7-Zip GUI", 0);
            return 1;
          }
          UINT32 numChars = dataSize /2;
          UString name;
          
          for (int i = 1; i < numChars; i++)
          {
            wchar_t c = curData[i];
            if (c == L'\0')
            {
              fileNames.Add(GetSystemString(name));
              name.Empty();
            }
            else
              name += c;
          }
          if (!name.IsEmpty())
          {
            MessageBoxW(0, L"data error", L"7-Zip GUI", 0);
            return 1;
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

      HRESULT result = CompressArchive(fileNames, archiveName, emailMode);
      // if (result != S_OK)
      //   ShowErrorMessage(result);
    }
    else
    {
      MessageBoxW(0, L"Use correct command", L"7-Zip GUI", 0);
      return 0;
    }
    return 0;
  }
  catch(const CNewException)
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



