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

#include "../FileManager/ProgramLocation.h"
#include "../FileManager/RegistryUtils.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif _UNICODE

using namespace NWindows;

static LPCWSTR kShowDialogSwitch = L" -ad";
static LPCWSTR kEmailSwitch = L" -seml.";
static LPCWSTR kMapSwitch = L" -i#";
static LPCWSTR kArchiveNoNameSwitch = L" -an";
static LPCWSTR kArchiveTypeSwitch = L" -t";
static LPCWSTR kArchiveMapSwitch = L" -ai#";
static LPCWSTR kStopSwitchParsing = L" --";
static LPCWSTR kLargePagesDisable = L" -slp-";

static void AddLagePagesSwitch(UString &params)
{
  if (!ReadLockMemoryEnable())
    params += kLargePagesDisable;
}

HRESULT MyCreateProcess(const UString &params, 
    LPCWSTR curDir, bool waitFinish,
    NWindows::NSynchronization::CBaseEvent *event)
{
  const UString params2 = params;
  PROCESS_INFORMATION processInformation;
  BOOL result;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    STARTUPINFOA startupInfo;
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpReserved = 0;
    startupInfo.lpDesktop = 0;
    startupInfo.lpTitle = 0;
    startupInfo.dwFlags = 0;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = 0;
    
    CSysString curDirA;
    if (curDir != 0)
      curDirA = GetSystemString(curDir);
    result = ::CreateProcessA(NULL, (LPSTR)(LPCSTR)GetSystemString(params), 
      NULL, NULL, FALSE, 0, NULL, 
      ((curDir != 0) ? (LPCSTR)curDirA: 0), 
      &startupInfo, &processInformation);
  }
  else
  #endif
  {
    STARTUPINFOW startupInfo;
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpReserved = 0;
    startupInfo.lpDesktop = 0;
    startupInfo.lpTitle = 0;
    startupInfo.dwFlags = 0;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = 0;
    
    result = ::CreateProcessW(NULL, (LPWSTR)(LPCWSTR)params,  
      NULL, NULL, FALSE, 0, NULL, 
      curDir, 
      &startupInfo, &processInformation);
  }
  if (result == 0)
    return ::GetLastError();
  else
  {
    ::CloseHandle(processInformation.hThread);
    if (waitFinish)
      WaitForSingleObject(processInformation.hProcess, INFINITE);
    else if (event != NULL)
    {
      HANDLE handles[] = {processInformation.hProcess, *event };
      ::WaitForMultipleObjects(sizeof(handles) / sizeof(handles[0]),
        handles, FALSE, INFINITE);
    }
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
  path += L"7zG.exe";
  return GetQuotedString(path);
}

static HRESULT CreateTempEvent(const wchar_t *name, 
    NSynchronization::CManualResetEvent &event, UString &eventName)
{
  CRandom random;
  random.Init(GetTickCount());
  for (;;)
  {
    int number = random.Generate();
    wchar_t temp[32];
    ConvertUInt64ToString((UInt32)number, temp);
    eventName = name;
    eventName += temp;
    RINOK(event.CreateWithName(false, GetSystemString(eventName)));
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
      return S_OK;
    event.Close();
  }
}

static HRESULT CreateMap(const UStringVector &names, 
    const UString &id,   
    CFileMapping &fileMapping, NSynchronization::CManualResetEvent &event,
    UString &params)
{
  UInt32 extraSize = 2;
  UInt32 dataSize = 0;
  for (int i = 0; i < names.Size(); i++)
    dataSize += (names[i].Length() + 1) * sizeof(wchar_t);
  UInt32 totalSize = extraSize + dataSize;
  
  UString mappingName;
  
  CRandom random;
  random.Init(GetTickCount());
  for (;;)
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
  
  UString eventName;
  RINOK(CreateTempEvent(id + L"MappingEndEvent", event, eventName));

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
  return S_OK;
}

HRESULT CompressFiles(
    const UString &curDir,
    const UString &archiveName,
    const UString &archiveType,
    const UStringVector &names, 
    // const UString &outFolder, 
    bool email,
    bool showDialog, 
    bool waitFinish)
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
  
  CFileMapping fileMapping;
  CRandom random;
  random.Init(GetTickCount());
  for (;;)
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
  
  NSynchronization::CManualResetEvent event;
  UString eventName;
  RINOK(CreateTempEvent(L"7zCompressMappingEndEvent", event, eventName));

  params += mappingName;
  params += L":";
  wchar_t string[10];
  ConvertUInt64ToString(totalSize, string);
  params += string;
  
  params += L":";
  params += eventName;

  if (!archiveType.IsEmpty())
  {
    params += kArchiveTypeSwitch;
    params += archiveType;
  }

  if (email)
    params += kEmailSwitch;

  if (showDialog)
    params += kShowDialogSwitch;

  AddLagePagesSwitch(params);

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
    RINOK(MyCreateProcess(params, 
      (curDir.IsEmpty()? 0: (LPCWSTR)curDir), 
      waitFinish, &event));
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
  AddLagePagesSwitch(params2);
  params2 += kArchiveNoNameSwitch;
  params2 += kArchiveMapSwitch;
  CFileMapping fileMapping;
  NSynchronization::CManualResetEvent event;
  RINOK(CreateMap(archivePaths, L"7zExtract", fileMapping, event, params2));
  return MyCreateProcess(params2, 0, false, &event);
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

HRESULT Benchmark()
{
  UString params;
  params = Get7zGuiPath();
  params += L" b";
  return MyCreateProcess(params, 0, false, NULL);
}
