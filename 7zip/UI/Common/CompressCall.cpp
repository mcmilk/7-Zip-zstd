// CompressCall.cpp

#include "StdAfx.h"

#include "CompressCall.h"

#include "Common/Random.h"
#include "Common/IntToString.h"
#include "Common/MyCom.h"
#include "Common/StringConvert.h"

#include "Windows/Synchronization.h"
#include "Windows/FileMapping.h"
#include "Windows/FileDir.h"

#include "../../FileManager/ProgramLocation.h"

using namespace NWindows;

static LPCWSTR kShowDialogSwitch = L" -ad";
static LPCWSTR kEmailSwitch = L" -seml.";
static LPCWSTR kMapSwitch = L" -i#";
static LPCWSTR kArchiveNoNameSwitch = L" -an";
static LPCWSTR kArchiveMapSwitch = L" -ai#";
static LPCWSTR kStopSwitchParsing = L" --";


static bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

HRESULT MyCreateProcess(const UString &params, 
    LPCTSTR curDir, 
    NWindows::NSynchronization::CEvent *event)
{
  STARTUPINFO startupInfo;
  startupInfo.cb = sizeof(startupInfo);
  startupInfo.lpReserved = 0;
  startupInfo.lpDesktop = 0;
  startupInfo.lpTitle = 0;
  startupInfo.dwFlags = 0;
  startupInfo.cbReserved2 = 0;
  startupInfo.lpReserved2 = 0;
  
  PROCESS_INFORMATION processInformation;
  BOOL result = ::CreateProcess(NULL, (TCHAR *)(const TCHAR *)
    GetSystemString(params), 
    NULL, NULL, FALSE, 0, NULL, 
    curDir, 
    &startupInfo, &processInformation);
  if (result == 0)
    return ::GetLastError();
  else
  {
    if (event != NULL)
    {
      HANDLE handles[] = {processInformation.hProcess, *event };
      ::WaitForMultipleObjects(sizeof(handles) / sizeof(handles[0]),
          handles, FALSE, INFINITE);
    }
    ::CloseHandle(processInformation.hThread);
    ::CloseHandle(processInformation.hProcess);
  }
  return S_OK;
}

static UString GetQuotedString(const UString &s)
{
  return UString(L"\"") + s + UString(L"\"");
}

static UString Get7zGuiPath()
{
  UString path;
  UString folder;
  if (GetProgramFolderPath(folder))
    path += folder;
  if (IsItWindowsNT())
    path += L"7zgn.exe";
  else
    path += L"7zg.exe";
  // path += L"7z.exe";
  return GetQuotedString(path);
}

static HRESULT CreateMap(const UStringVector &names, 
    const UString &id,   
    CFileMapping &fileMapping, NSynchronization::CEvent &event,
    UString &params)
{
  UInt32 extraSize = 2;
  UInt32 dataSize = 0;
  for (int i = 0; i < names.Size(); i++)
    dataSize += (names[i].Length() + 1) * sizeof(wchar_t);
  UInt32 totalSize = extraSize + dataSize;
  
  UString mappingName;
  UString eventName;
  
  CRandom random;
  random.Init(GetTickCount());
  while(true)
  {
    int number = random.Generate();
    wchar_t temp[32];
    ConvertUInt64ToString(UInt32(number), temp);
    mappingName = id;
    mappingName += L"Mapping";
    mappingName += temp;
    if (!fileMapping.Create(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, totalSize, GetSystemString(mappingName)))
      return E_FAIL;
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
      break;
    fileMapping.Close();
  }
  
  while(true)
  {
    int number = random.Generate();
    wchar_t temp[32];
    ConvertUInt64ToString(UInt32(number), temp);
    eventName = id;
    eventName += L"MappingEndEvent";
    eventName += temp;
    if (!event.Create(true, false, GetSystemString(eventName)))
      return E_FAIL;
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
      break;
    event.Close();
  }

  params += mappingName;
  params += L":";
  wchar_t string[10];
  ConvertUInt64ToString(totalSize, string);
  params += string;
  
  params += L":";
  params += eventName;

  LPVOID data = fileMapping.MapViewOfFile(FILE_MAP_WRITE, 0, totalSize);
  if (data == NULL)
    return E_FAIL;
  try
  {
    wchar_t *curData = (wchar_t *)data;
    *curData = 0;
    curData++;
    for (int i = 0; i < names.Size(); i++)
    {
      const UString &s = names[i];
      memcpy(curData, (const wchar_t *)s, s.Length() * sizeof(wchar_t));
      curData += s.Length();
      *curData++ = L'\0';
    }
  }
  catch(...)
  {
    UnmapViewOfFile(data);
    throw;
  }
  // UnmapViewOfFile(data);
  return S_OK;
}

HRESULT CompressFiles(
    const UString &curDir,
    const UString &archiveName,
    const UStringVector &names, 
    // const UString &outFolder, 
    bool email,
    bool showDialog)
{
  /*
  UString curDir;
  if (names.Size() > 0)
  {
    NFile::NDirectory::GetOnlyDirPrefix(names[0], curDir);
  }
  */
  UString params;
  params = Get7zGuiPath();
  params += L" a";
  params += kMapSwitch;
  // params += _fileNames[0];
  
  UInt32 extraSize = 2;
  UInt32 dataSize = 0;
  for (int i = 0; i < names.Size(); i++)
    dataSize += (names[i].Length() + 1) * sizeof(wchar_t);
  UInt32 totalSize = extraSize + dataSize;
  
  UString mappingName;
  UString eventName;
  
  CFileMapping fileMapping;
  CRandom random;
  random.Init(GetTickCount());
  while(true)
  {
    int number = random.Generate();
    wchar_t temp[32];
    ConvertUInt64ToString(UInt32(number), temp);
    mappingName = L"7zCompressMapping";
    mappingName += temp;
    if (!fileMapping.Create(INVALID_HANDLE_VALUE, NULL,
      PAGE_READWRITE, totalSize, GetSystemString(mappingName)))
    {
      // MyMessageBox(IDS_ERROR, 0x02000605);
      return E_FAIL;
    }
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
      break;
    fileMapping.Close();
  }
  
  NSynchronization::CEvent event;
  while(true)
  {
    int number = random.Generate();
    wchar_t temp[32];
    ConvertUInt64ToString(UInt32(number), temp);
    eventName = L"7zCompressMappingEndEvent";
    eventName += temp;
    if (!event.Create(true, false, GetSystemString(eventName)))
    {
      // MyMessageBox(IDS_ERROR, 0x02000605);
      return E_FAIL;
    }
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
      break;
    event.Close();
  }

  params += mappingName;
  params += L":";
  wchar_t string[10];
  ConvertUInt64ToString(totalSize, string);
  params += string;
  
  params += L":";
  params += eventName;

  if (email)
    params += kEmailSwitch;

  if (showDialog)
    params += kShowDialogSwitch;

  params += kStopSwitchParsing;
  params += L" ";
  
  params += GetQuotedString(archiveName);
  
  LPVOID data = fileMapping.MapViewOfFile(FILE_MAP_WRITE, 0, totalSize);
  if (data == NULL)
  {
    // MyMessageBox(IDS_ERROR, 0x02000605);
    return E_FAIL;
  }
  try
  {
    wchar_t *curData = (wchar_t *)data;
    *curData = 0;
    curData++;
    for (int i = 0; i < names.Size(); i++)
    {
      const UString &unicodeString = names[i];
      memcpy(curData, (const wchar_t *)unicodeString , 
        unicodeString .Length() * sizeof(wchar_t));
      curData += unicodeString.Length();
      *curData++ = L'\0';
    }
    // MessageBox(0, params, 0, 0);
    CSysString sysCurDir = GetSystemString(curDir);
    RINOK(MyCreateProcess(params, 
      (sysCurDir.IsEmpty()? 0: (LPCTSTR)sysCurDir), 
      &event));
  }
  catch(...)
  {
    UnmapViewOfFile(data);
    throw;
  }
  UnmapViewOfFile(data);
  

  /*
  CThreadCompressMain *compressor = new CThreadCompressMain();;
  compressor->FileNames = _fileNames;
  CThread thread;
  if (!thread.Create(CThreadCompressMain::MyThreadFunction, compressor))
  throw 271824;
  */
  return S_OK;
}

static HRESULT ExtractGroupCommand(const UStringVector &archivePaths,
    const UString &params)
{
  UString params2 = params;
  params2 += kArchiveNoNameSwitch;
  params2 += kArchiveMapSwitch;
  CFileMapping fileMapping;
  NSynchronization::CEvent event;
  RINOK(CreateMap(archivePaths, L"7zExtract", fileMapping, event, params2));
  return MyCreateProcess(params2, 0, &event);
}

HRESULT ExtractArchives(const UStringVector &archivePaths,
    const UString &outFolder, bool showDialog)
{
  UString params;
  params = Get7zGuiPath();
  params += L" x";
  if (!outFolder.IsEmpty())
  {
    params += L" -o";
    params += GetQuotedString(outFolder);
  }
  if (showDialog)
    params += kShowDialogSwitch;
  return ExtractGroupCommand(archivePaths, params);
}

HRESULT TestArchives(const UStringVector &archivePaths)
{
  UString params;
  params = Get7zGuiPath();
  params += L" t";
  return ExtractGroupCommand(archivePaths, params);
}
