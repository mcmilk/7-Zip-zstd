// CompressCall.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/MyCom.h"
#include "Common/Random.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileMapping.h"
#include "Windows/Process.h"
#include "Windows/Synchronization.h"

#include "../FileManager/ProgramLocation.h"
#include "../FileManager/RegistryUtils.h"

#include "CompressCall.h"

using namespace NWindows;

#define MY_TRY_BEGIN try {
#define MY_TRY_FINISH } \
  catch(...) { ErrorMessageHRESULT(E_FAIL); return E_FAIL; }

static LPCWSTR kShowDialogSwitch = L" -ad";
static LPCWSTR kEmailSwitch = L" -seml.";
static LPCWSTR kIncludeSwitch = L" -i";
static LPCWSTR kArchiveTypeSwitch = L" -t";
static LPCWSTR kArcIncludeSwitches = L" -an -ai";
static LPCWSTR kStopSwitchParsing = L" --";
static LPCWSTR kLargePagesDisable = L" -slp-";

UString GetQuotedString(const UString &s)
{
  return UString(L'\"') + s + UString(L'\"');
}
static void ErrorMessage(LPCWSTR message)
{
  MessageBoxW(g_HWND, message, L"7-Zip", MB_ICONERROR | MB_OK);
}

static void ErrorMessageHRESULT(HRESULT res, LPCWSTR s = NULL)
{
  UString s2 = HResultToMessage(res);
  if (s)
  {
    s2 += L'\n';
    s2 += s;
  }
  ErrorMessage(s2);
}

static HRESULT MyCreateProcess(LPCWSTR imageName, const UString &params,
    LPCWSTR curDir, bool waitFinish,
    NSynchronization::CBaseEvent *event)
{
  CProcess process;
  WRes res = process.Create(imageName, params, curDir);
  if (res != 0)
  {
    ErrorMessageHRESULT(res, imageName);
    return res;
  }
  if (waitFinish)
    process.Wait();
  else if (event != NULL)
  {
    HANDLE handles[] = { process, *event };
    ::WaitForMultipleObjects(sizeof(handles) / sizeof(handles[0]), handles, FALSE, INFINITE);
  }
  return S_OK;
}

static void AddLagePagesSwitch(UString &params)
{
  if (!ReadLockMemoryEnable())
    params += kLargePagesDisable;
}

static UString Get7zGuiPath()
{
  UString path;
  GetProgramFolderPath(path);
  return path + L"7zG.exe";
}

class CRandNameGenerator
{
  CRandom _random;
public:
  CRandNameGenerator() { _random.Init(); }
  UString GenerateName()
  {
    wchar_t temp[16];
    ConvertUInt32ToString((UInt32)_random.Generate(), temp);
    return temp;
  }
};

static HRESULT CreateMap(const UStringVector &names,
    CFileMapping &fileMapping, NSynchronization::CManualResetEvent &event,
    UString &params)
{
  UInt32 totalSize = 1;
  for (int i = 0; i < names.Size(); i++)
    totalSize += (names[i].Length() + 1);
  totalSize *= sizeof(wchar_t);
  
  CRandNameGenerator random;

  UString mappingName;
  for (;;)
  {
    mappingName = L"7zMap" + random.GenerateName();

    WRes res = fileMapping.Create(PAGE_READWRITE, totalSize, GetSystemString(mappingName));
    if (fileMapping.IsCreated() && res == 0)
      break;
    if (res != ERROR_ALREADY_EXISTS)
      return res;
    fileMapping.Close();
  }
  
  UString eventName;
  for (;;)
  {
    eventName = L"7zEvent" + random.GenerateName();
    WRes res = event.CreateWithName(false, GetSystemString(eventName));
    if (event.IsCreated() && res == 0)
      break;
    if (res != ERROR_ALREADY_EXISTS)
      return res;
    event.Close();
  }

  params += L'#';
  params += mappingName;
  params += L':';
  wchar_t temp[16];
  ConvertUInt32ToString(totalSize, temp);
  params += temp;
  
  params += L':';
  params += eventName;

  LPVOID data = fileMapping.Map(FILE_MAP_WRITE, 0, totalSize);
  if (data == NULL)
    return E_FAIL;
  CFileUnmapper unmapper(data);
  {
    wchar_t *cur = (wchar_t *)data;
    *cur++ = 0;
    for (int i = 0; i < names.Size(); i++)
    {
      const UString &s = names[i];
      int len = s.Length() + 1;
      memcpy(cur, (const wchar_t *)s, len * sizeof(wchar_t));
      cur += len;
    }
  }
  return S_OK;
}

HRESULT CompressFiles(
    const UString &arcPathPrefix,
    const UString &arcName,
    const UString &arcType,
    const UStringVector &names,
    bool email, bool showDialog, bool waitFinish)
{
  MY_TRY_BEGIN
  UString params = L'a';
  
  CFileMapping fileMapping;
  NSynchronization::CManualResetEvent event;
  params += kIncludeSwitch;
  RINOK(CreateMap(names, fileMapping, event, params));

  if (!arcType.IsEmpty())
  {
    params += kArchiveTypeSwitch;
    params += arcType;
  }

  if (email)
    params += kEmailSwitch;

  if (showDialog)
    params += kShowDialogSwitch;

  AddLagePagesSwitch(params);

  params += kStopSwitchParsing;
  params += L' ';
  
  params += GetQuotedString(
    #ifdef UNDER_CE
    arcPathPrefix +
    #endif
    arcName);
  
  return MyCreateProcess(Get7zGuiPath(), params,
      (arcPathPrefix.IsEmpty()? 0: (LPCWSTR)arcPathPrefix), waitFinish, &event);
  MY_TRY_FINISH
}

static HRESULT ExtractGroupCommand(const UStringVector &arcPaths, UString &params)
{
  AddLagePagesSwitch(params);
  params += kArcIncludeSwitches;
  CFileMapping fileMapping;
  NSynchronization::CManualResetEvent event;
  RINOK(CreateMap(arcPaths, fileMapping, event, params));
  return MyCreateProcess(Get7zGuiPath(), params, 0, false, &event);
}

HRESULT ExtractArchives(const UStringVector &arcPaths, const UString &outFolder, bool showDialog)
{
  MY_TRY_BEGIN
  UString params = L'x';
  if (!outFolder.IsEmpty())
  {
    params += L" -o";
    params += GetQuotedString(outFolder);
  }
  if (showDialog)
    params += kShowDialogSwitch;
  return ExtractGroupCommand(arcPaths, params);
  MY_TRY_FINISH
}

HRESULT TestArchives(const UStringVector &arcPaths)
{
  MY_TRY_BEGIN
  UString params = L't';
  return ExtractGroupCommand(arcPaths, params);
  MY_TRY_FINISH
}

HRESULT Benchmark()
{
  MY_TRY_BEGIN
  return MyCreateProcess(Get7zGuiPath(), L'b', 0, false, NULL);
  MY_TRY_FINISH
}
