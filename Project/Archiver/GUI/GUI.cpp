// GUI.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/NewHandler.h"
#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/FileMapping.h"
#include "Windows/Synchronization.h"

#include "Interface/IInOutStreams.h"
#include "Interface/CryptoInterface.h"

#include "../../Compress/Interface/CompressInterface.h"

#include "../../FileManager/FolderInterface.h"
#include "../../FileManager/StringUtils.h"

#include "../Resource/Extract/resource.h"
#include "../Agent/Handler.h"

#include "../Common/FolderArchiveInterface.h"

#include "../Explorer/TestEngine.h"
#include "../Explorer/ExtractEngine.h"
#include "../Explorer/CompressEngine.h"
#include "../Explorer/MyMessages.h"

using namespace NWindows;

HINSTANCE g_hInstance;

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
  NCOM::CComInitializer comInitializer;
  try
  {
    UString programString, commandsString;
    SplitStringToTwoStrings(GetCommandLineW(), programString, commandsString);
    commandsString.Trim();
    UString paramString, tailString;
    SplitStringToTwoStrings(commandsString, paramString, tailString);
    paramString.Trim();
    paramString.MakeLower();
    UString archiveName, tailString2;
    SplitStringToTwoStrings(tailString, archiveName, tailString2);
    if (paramString.IsEmpty())
    {
      MessageBoxW(0, L"Use some command", L"7-Zip GUI", 0);
      return 0;
    }
    if (paramString == UString(L"t"))
    {
      HRESULT result = TestArchive(0, GetSystemString(archiveName));
      if (result == S_FALSE)
        MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      else if (result != S_OK)
        ShowErrorMessage(0, result);
    }
    else if (paramString == UString(L"x"))
    {
      HRESULT result = ExtractArchive(0, GetSystemString(archiveName), false);
      if (result == S_FALSE)
        MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      else if (result != S_OK)
        ShowErrorMessage(0, result);
    }
    else if (paramString == UString(L"a"))
    {
      UString mapString, emailString;
      SplitStringToTwoStrings(tailString, mapString, emailString);
      bool emailMode = (emailString.CompareNoCase(L"-email") == 0);
      UString switchName = L"-map=";
      if (switchName.CompareNoCase(mapString.Left(switchName.Length())) != 0)
      {
        MessageBoxW(0, L"Bad switch", L"7-Zip GUI", 0);
        return 1;
      }
      UString switchParam = mapString.Mid(switchName.Length());
      
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

      HRESULT result = CompressArchive(fileNames, emailMode);
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



