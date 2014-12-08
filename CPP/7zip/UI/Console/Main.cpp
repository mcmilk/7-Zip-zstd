// Main.cpp

#include "StdAfx.h"

#include <Psapi.h>

#if defined( _WIN32) && defined( _7ZIP_LARGE_PAGES)
#include "../../../../C/Alloc.h"
#endif

#include "../../../Common/MyInitGuid.h"

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/IntToString.h"
#include "../../../Common/MyException.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"

#include "../../../Windows/ErrorMsg.h"
#ifdef _WIN32
#include "../../../Windows/MemoryLock.h"
#endif

#ifndef _7ZIP_ST
#include "../../../Windows/Synchronization.h"
#endif

#include "../../../Windows/TimeUtils.h"

#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"
#include "../Common/Extract.h"
#ifdef EXTERNAL_CODECS
#include "../Common/LoadCodecs.h"
#endif

#include "BenchCon.h"
#include "ConsoleClose.h"
#include "ExtractCallbackConsole.h"
#include "List.h"
#include "OpenCallbackConsole.h"
#include "UpdateCallbackConsole.h"

#include "HashCon.h"

#ifdef PROG_VARIANT_R
#include "../../../../C/7zVersion.h"
#else
#include "../../MyVersion.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

#ifdef _WIN32
HINSTANCE g_hInstance = 0;
#endif
extern CStdOutStream *g_StdStream;

static const char *kCopyrightString = "\n7-Zip"
#ifndef EXTERNAL_CODECS
#ifdef PROG_VARIANT_R
" (r)"
#else
" (a)"
#endif
#endif

#ifdef _WIN64
" [64]"
#endif

" " MY_VERSION_COPYRIGHT_DATE "\n";

static const char *kHelpString =
    "\nUsage: 7z"
#ifndef EXTERNAL_CODECS
#ifdef PROG_VARIANT_R
    "r"
#else
    "a"
#endif
#endif
    " <command> [<switches>...] <archive_name> [<file_names>...]\n"
    "       [<@listfiles...>]\n"
    "\n"
    "<Commands>\n"
    "  a : Add files to archive\n"
    "  b : Benchmark\n"
    "  d : Delete files from archive\n"
    "  e : Extract files from archive (without using directory names)\n"
    "  h : Calculate hash values for files\n"
    "  l : List contents of archive\n"
//    "  l[a|t][f] : List contents of archive\n"
//    "    a - with Additional fields\n"
//    "    t - with all fields\n"
//    "    f - with Full pathnames\n"
    "  rn : Rename files in archive\n"
    "  t : Test integrity of archive\n"
    "  u : Update files to archive\n"
    "  x : eXtract files with full paths\n"
    "<Switches>\n"
    "  -- : Stop switches parsing\n"
    "  -ai[r[-|0]]{@listfile|!wildcard} : Include archives\n"
    "  -ax[r[-|0]]{@listfile|!wildcard} : eXclude archives\n"
    "  -bd : Disable percentage indicator\n"
    "  -i[r[-|0]]{@listfile|!wildcard} : Include filenames\n"
    "  -m{Parameters} : set compression Method\n"
    "  -o{Directory} : set Output directory\n"
    #ifndef _NO_CRYPTO
    "  -p{Password} : set Password\n"
    #endif
    "  -r[-|0] : Recurse subdirectories\n"
    "  -scs{UTF-8|UTF-16LE|UTF-16BE|WIN|DOS|{id}} : set charset for list files\n"
    "  -sfx[{name}] : Create SFX archive\n"
    "  -si[{name}] : read data from stdin\n"
    "  -slt : show technical information for l (List) command\n"
    "  -so : write data to stdout\n"
    "  -ssc[-] : set sensitive case mode\n"
    "  -ssw : compress shared files\n"
    "  -t{Type} : Set type of archive\n"
    "  -u[-][p#][q#][r#][x#][y#][z#][!newArchiveName] : Update options\n"
    "  -v{Size}[b|k|m|g] : Create volumes\n"
    "  -w[{path}] : assign Work directory. Empty path means a temporary directory\n"
    "  -x[r[-|0]]]{@listfile|!wildcard} : eXclude filenames\n"
    "  -y : assume Yes on all queries\n";

// ---------------------------
// exception messages

static const char *kEverythingIsOk = "Everything is Ok";
static const char *kUserErrorMessage = "Incorrect command line";
static const char *kNoFormats = "7-Zip cannot find the code that works with archives.";
static const char *kUnsupportedArcTypeMessage = "Unsupported archive type";
// static const char *kUnsupportedUpdateArcType = "Can't create archive for that type";

static CFSTR kDefaultSfxModule = FTEXT("7zCon.sfx");

static void ShowMessageAndThrowException(CStdOutStream &s, LPCSTR message, NExitCode::EEnum code)
{
  s << endl << "Error: " << message << endl;
  throw code;
}

#ifndef _WIN32
static void GetArguments(int numArgs, const char *args[], UStringVector &parts)
{
  parts.Clear();
  for (int i = 0; i < numArgs; i++)
  {
    UString s = MultiByteToUnicodeString(args[i]);
    parts.Add(s);
  }
}
#endif

static void ShowCopyrightAndHelp(CStdOutStream &s, bool needHelp)
{
  s << kCopyrightString;
  // s << "# CPUs: " << (UInt64)NWindows::NSystem::GetNumberOfProcessors() << "\n";
  if (needHelp)
    s << kHelpString;
}

#ifdef EXTERNAL_CODECS

static void PrintString(CStdOutStream &stdStream, const AString &s, int size)
{
  int len = s.Len();
  for (int i = len; i < size; i++)
    stdStream << ' ';
  stdStream << s;
}

static void PrintUInt32(CStdOutStream &stdStream, UInt32 val, int size)
{
  char s[16];
  ConvertUInt32ToString(val, s);
  PrintString(stdStream, s, size);
}

static void PrintLibIndex(CStdOutStream &stdStream, int libIndex)
{
  if (libIndex >= 0)
    PrintUInt32(stdStream, libIndex, 2);
  else
    stdStream << "  ";
  stdStream << ' ';
}

#endif

static void PrintString(CStdOutStream &stdStream, const UString &s, int size)
{
  int len = s.Len();
  stdStream << s;
  for (int i = len; i < size; i++)
    stdStream << ' ';
}

static inline char GetHex(unsigned val)
{
  return (char)((val < 10) ? ('0' + val) : ('A' + (val - 10)));
}

static int WarningsCheck(HRESULT result, const CCallbackConsoleBase &callback,
    const CErrorInfo &errorInfo, CStdOutStream &stdStream)
{
  int exitCode = NExitCode::kSuccess;
  
  if (callback.CantFindFiles.Size() > 0)
  {
    stdStream << endl;
    stdStream << "WARNINGS for files:" << endl << endl;
    unsigned numErrors = callback.CantFindFiles.Size();
    for (unsigned i = 0; i < numErrors; i++)
    {
      stdStream << callback.CantFindFiles[i] << " : ";
      stdStream << NError::MyFormatMessage(callback.CantFindCodes[i]) << endl;
    }
    stdStream << "----------------" << endl;
    stdStream << "WARNING: Cannot find " << numErrors << " file";
    if (numErrors > 1)
      stdStream << "s";
    stdStream << endl;
    exitCode = NExitCode::kWarning;
  }
  
  if (result != S_OK)
  {
    UString message;
    if (!errorInfo.Message.IsEmpty())
    {
      message += errorInfo.Message;
      message += L"\n";
    }
    if (!errorInfo.FileName.IsEmpty())
    {
      message += fs2us(errorInfo.FileName);
      message += L"\n";
    }
    if (!errorInfo.FileName2.IsEmpty())
    {
      message += fs2us(errorInfo.FileName2);
      message += L"\n";
    }
    if (errorInfo.SystemError != 0)
    {
      message += NError::MyFormatMessage(errorInfo.SystemError);
      message += L"\n";
    }
    if (!message.IsEmpty())
      stdStream << L"\nError:\n" << message;

    // we will work with (result) later
    // throw CSystemException(result);
    return NExitCode::kFatalError;
  }

  unsigned numErrors = callback.FailedFiles.Size();
  if (numErrors == 0)
  {
    if (callback.CantFindFiles.Size() == 0)
      stdStream << kEverythingIsOk << endl;
  }
  else
  {
    stdStream << endl;
    stdStream << "WARNINGS for files:" << endl << endl;
    for (unsigned i = 0; i < numErrors; i++)
    {
      stdStream << callback.FailedFiles[i] << " : ";
      stdStream << NError::MyFormatMessage(callback.FailedCodes[i]) << endl;
    }
    stdStream << "----------------" << endl;
    stdStream << "WARNING: Cannot open " << numErrors << " file";
    if (numErrors > 1)
      stdStream << "s";
    stdStream << endl;
    exitCode = NExitCode::kWarning;
  }
  return exitCode;
}

static void ThrowException_if_Error(HRESULT res)
{
  if (res != S_OK)
    throw CSystemException(res);
}


static void PrintNum(UInt64 val, unsigned numDigits, char c = ' ')
{
  char temp[64];
  char *p = temp + 32;
  ConvertUInt64ToString(val, p);
  unsigned len = MyStringLen(p);
  for (; len < numDigits; len++)
    *--p = c;
  *g_StdStream << p;
}

static void PrintTime(const char *s, UInt64 val, UInt64 total)
{
  *g_StdStream << endl << s << " Time =";
  const UInt32 kFreq = 10000000;
  UInt64 sec = val / kFreq;
  PrintNum(sec, 6);
  *g_StdStream << '.';
  UInt32 ms = (UInt32)(val - (sec * kFreq)) / (kFreq / 1000);
  PrintNum(ms, 3, '0');
  
  while (val > ((UInt64)1 << 56))
  {
    val >>= 1;
    total >>= 1;
  }

  UInt64 percent = 0;
  if (total != 0)
    percent = val * 100 / total;
  *g_StdStream << " =";
  PrintNum(percent, 5);
  *g_StdStream << '%';
}

#ifndef UNDER_CE

#define SHIFT_SIZE_VALUE(x, num) (((x) + (1 << (num)) - 1) >> (num))

static void PrintMemUsage(const char *s, UInt64 val)
{
  *g_StdStream << "    " << s << " Memory =";
  PrintNum(SHIFT_SIZE_VALUE(val, 20), 7);
  *g_StdStream << " MB";
}

EXTERN_C_BEGIN
typedef BOOL (WINAPI *Func_GetProcessMemoryInfo)(HANDLE Process,
    PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);
EXTERN_C_END

#endif

static inline UInt64 GetTime64(const FILETIME &t) { return ((UInt64)t.dwHighDateTime << 32) | t.dwLowDateTime; }

static void PrintStat()
{
  FILETIME creationTimeFT, exitTimeFT, kernelTimeFT, userTimeFT;
  if (!
      #ifdef UNDER_CE
        ::GetThreadTimes(::GetCurrentThread()
      #else
        // NT 3.5
        ::GetProcessTimes(::GetCurrentProcess()
      #endif
      , &creationTimeFT, &exitTimeFT, &kernelTimeFT, &userTimeFT))
    return;
  FILETIME curTimeFT;
  NTime::GetCurUtcFileTime(curTimeFT);

  #ifndef UNDER_CE
  
  PROCESS_MEMORY_COUNTERS m;
  memset(&m, 0, sizeof(m));
  BOOL memDefined = FALSE;
  {
    /* NT 4.0: GetProcessMemoryInfo() in Psapi.dll
       Win7: new function K32GetProcessMemoryInfo() in kernel32.dll
       It's faster to call kernel32.dll code than Psapi.dll code
       GetProcessMemoryInfo() requires Psapi.lib
       Psapi.lib in SDK7+ can link to K32GetProcessMemoryInfo in kernel32.dll
       The program with K32GetProcessMemoryInfo will not work on systems before Win7
       // memDefined = GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof(m));
    */

    Func_GetProcessMemoryInfo my_GetProcessMemoryInfo = (Func_GetProcessMemoryInfo)
        ::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "K32GetProcessMemoryInfo");
    if (!my_GetProcessMemoryInfo)
    {
      HMODULE lib = LoadLibraryW(L"Psapi.dll");
      if (lib)
        my_GetProcessMemoryInfo = (Func_GetProcessMemoryInfo)::GetProcAddress(lib, "GetProcessMemoryInfo");
    }
    if (my_GetProcessMemoryInfo)
      memDefined = my_GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof(m));
    // FreeLibrary(lib);
  }

  #endif

  UInt64 curTime = GetTime64(curTimeFT);
  UInt64 creationTime = GetTime64(creationTimeFT);
  UInt64 kernelTime = GetTime64(kernelTimeFT);
  UInt64 userTime = GetTime64(userTimeFT);

  UInt64 totalTime = curTime - creationTime;
  
  PrintTime("Kernel ", kernelTime, totalTime);
  PrintTime("User   ", userTime, totalTime);
  
  PrintTime("Process", kernelTime + userTime, totalTime);
  #ifndef UNDER_CE
  if (memDefined) PrintMemUsage("Virtual ", m.PeakPagefileUsage);
  #endif
  
  PrintTime("Global ", totalTime, totalTime);
  #ifndef UNDER_CE
  if (memDefined) PrintMemUsage("Physical", m.PeakWorkingSetSize);
  #endif
  
  *g_StdStream << endl;
}

int Main2(
  #ifndef _WIN32
  int numArgs, const char *args[]
  #endif
)
{
  #if defined(_WIN32) && !defined(UNDER_CE)
  SetFileApisToOEM();
  #endif

  UStringVector commandStrings;
  #ifdef _WIN32
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);
  #else
  GetArguments(numArgs, args, commandStrings);
  #endif

  if (commandStrings.Size() == 1)
  {
    ShowCopyrightAndHelp(g_StdOut, true);
    return 0;
  }

  commandStrings.Delete(0);

  CArcCmdLineOptions options;

  CArcCmdLineParser parser;

  parser.Parse1(commandStrings, options);

  if (options.HelpMode)
  {
    ShowCopyrightAndHelp(g_StdOut, true);
    return 0;
  }

  #if defined(_WIN32) && !defined(UNDER_CE)
  NSecurity::EnablePrivilege_SymLink();
  #endif
  #ifdef _7ZIP_LARGE_PAGES
  if (options.LargePages)
  {
    SetLargePageSize();
    #if defined(_WIN32) && !defined(UNDER_CE)
    NSecurity::EnablePrivilege_LockMemory();
    #endif
  }
  #endif

  CStdOutStream &stdStream = options.StdOutMode ? g_StdErr : g_StdOut;
  g_StdStream = &stdStream;

  if (options.EnableHeaders)
    ShowCopyrightAndHelp(stdStream, false);

  parser.Parse2(options);

  CCodecs *codecs = new CCodecs;
  #ifdef EXTERNAL_CODECS
  CExternalCodecs __externalCodecs;
  __externalCodecs.GetCodecs = codecs;
  __externalCodecs.GetHashers = codecs;
  #else
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  #endif
  codecs->CaseSensitiveChange = options.CaseSensitiveChange;
  codecs->CaseSensitive = options.CaseSensitive;
  ThrowException_if_Error(codecs->Load());

  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();

  if (codecs->Formats.Size() == 0 &&
        (isExtractGroupCommand
        || options.Command.CommandType == NCommandType::kList
        || options.Command.IsFromUpdateGroup()))
    throw kNoFormats;

  CObjectVector<COpenType> types;
  if (!ParseOpenTypes(*codecs, options.ArcType, types))
    throw kUnsupportedArcTypeMessage;

  CIntVector excludedFormats;
  FOR_VECTOR (k, options.ExcludedArcTypes)
  {
    CIntVector tempIndices;
    if (!codecs->FindFormatForArchiveType(options.ExcludedArcTypes[k], tempIndices)
        || tempIndices.Size() != 1)
      throw kUnsupportedArcTypeMessage;
    excludedFormats.AddToUniqueSorted(tempIndices[0]);
    // excludedFormats.Sort();
  }

  
  #ifdef EXTERNAL_CODECS
  if (isExtractGroupCommand
      || options.Command.CommandType == NCommandType::kHash
      || options.Command.CommandType == NCommandType::kBenchmark)
    ThrowException_if_Error(__externalCodecs.LoadCodecs());
  #endif

  int retCode = NExitCode::kSuccess;
  HRESULT hresultMain = S_OK;

  bool showStat = true;
  if (!options.EnableHeaders ||
      options.TechMode)
    showStat = false;
  

  if (options.Command.CommandType == NCommandType::kInfo)
  {
    unsigned i;

    #ifdef EXTERNAL_CODECS
    stdStream << endl << "Libs:" << endl;
    for (i = 0; i < codecs->Libs.Size(); i++)
    {
      PrintLibIndex(stdStream, i);
      stdStream << ' ' << codecs->Libs[i].Path << endl;
    }
    #endif

    stdStream << endl << "Formats:" << endl;
    
    const char *kArcFlags = "KSNFMGOPBELH";
    const unsigned kNumArcFlags = (unsigned)strlen(kArcFlags);
    
    for (i = 0; i < codecs->Formats.Size(); i++)
    {
      const CArcInfoEx &arc = codecs->Formats[i];
      #ifdef EXTERNAL_CODECS
      PrintLibIndex(stdStream, arc.LibIndex);
      #else
      stdStream << "  ";
      #endif
      stdStream << (char)(arc.UpdateEnabled ? 'C' : ' ');
      for (unsigned b = 0; b < kNumArcFlags; b++)
      {
        stdStream << (char)
          ((arc.Flags & ((UInt32)1 << b)) != 0 ? kArcFlags[b] : ' ');
      }
        
      stdStream << ' ';
      PrintString(stdStream, arc.Name, 8);
      stdStream << ' ';
      UString s;
      FOR_VECTOR (t, arc.Exts)
      {
        if (t != 0)
          s += L' ';
        const CArcExtInfo &ext = arc.Exts[t];
        s += ext.Ext;
        if (!ext.AddExt.IsEmpty())
        {
          s += L" (";
          s += ext.AddExt;
          s += L')';
        }
      }
      PrintString(stdStream, s, 13);
      stdStream << ' ';
      if (arc.SignatureOffset != 0)
        stdStream << "offset=" << arc.SignatureOffset << ' ';

      FOR_VECTOR(si, arc.Signatures)
      {
        if (si != 0)
          stdStream << "  ||  ";

        const CByteBuffer &sig = arc.Signatures[si];
        
        for (size_t j = 0; j < sig.Size(); j++)
        {
          if (j != 0)
            stdStream << ' ';
          Byte b = sig[j];
          if (b > 0x20 && b < 0x80)
          {
            stdStream << (char)b;
          }
          else
          {
            stdStream << GetHex((b >> 4) & 0xF);
            stdStream << GetHex(b & 0xF);
          }
        }
      }
      stdStream << endl;
    }

    #ifdef EXTERNAL_CODECS

    stdStream << endl << "Codecs:" << endl << "Lib         ID  Name" << endl;
    UInt32 numMethods;
    if (codecs->GetNumberOfMethods(&numMethods) == S_OK)
    for (UInt32 j = 0; j < numMethods; j++)
    {
      PrintLibIndex(stdStream, codecs->GetCodecLibIndex(j));
      stdStream << (char)(codecs->GetCodecEncoderIsAssigned(j) ? 'C' : ' ');
      UInt64 id;
      stdStream << "  ";
      HRESULT res = codecs->GetCodecId(j, id);
      if (res != S_OK)
        id = (UInt64)(Int64)-1;
      char s[32];
      ConvertUInt64ToHex(id, s);
      PrintString(stdStream, s, 8);
      stdStream << "  " << codecs->GetCodecName(j) << endl;
    }
    
    stdStream << endl << "Hashers:" << endl << " L Size     ID  Name" << endl;
    numMethods = codecs->GetNumHashers();
    for (UInt32 j = 0; j < numMethods; j++)
    {
      PrintLibIndex(stdStream, codecs->GetHasherLibIndex(j));
      PrintUInt32(stdStream, codecs->GetHasherDigestSize(j), 4);
      stdStream << ' ';
      char s[32];
      ConvertUInt64ToHex(codecs->GetHasherId(j), s);
      PrintString(stdStream, s, 6);
      stdStream << "  " << codecs->GetHasherName(j) << endl;
    }

    #endif
    
  }
  else if (options.Command.CommandType == NCommandType::kBenchmark)
  {
    hresultMain = BenchCon(EXTERNAL_CODECS_VARS
        options.Properties, options.NumIterations, (FILE *)stdStream);
    if (hresultMain == S_FALSE)
    {
      stdStream << "\nDecoding Error\n";
      retCode = NExitCode::kFatalError;
      hresultMain = S_OK;
    }
  }
  else if (isExtractGroupCommand || options.Command.CommandType == NCommandType::kList)
  {
    if (isExtractGroupCommand)
    {
      CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
      CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

      ecs->OutStream = &stdStream;

      #ifndef _NO_CRYPTO
      ecs->PasswordIsDefined = options.PasswordEnabled;
      ecs->Password = options.Password;
      #endif

      ecs->Init();

      COpenCallbackConsole openCallback;
      openCallback.OutStream = &stdStream;

      #ifndef _NO_CRYPTO
      openCallback.PasswordIsDefined = options.PasswordEnabled;
      openCallback.Password = options.Password;
      #endif

      CExtractOptions eo;
      (CExtractOptionsBase &)eo = options.ExtractOptions;
      eo.StdInMode = options.StdInMode;
      eo.StdOutMode = options.StdOutMode;
      eo.YesToAll = options.YesToAll;
      eo.TestMode = options.Command.IsTestCommand();
      
      #ifndef _SFX
      eo.Properties = options.Properties;
      #endif

      UString errorMessage;
      CDecompressStat stat;
      CHashBundle hb;
      IHashCalc *hashCalc = NULL;

      if (!options.HashMethods.IsEmpty())
      {
        hashCalc = &hb;
        ThrowException_if_Error(hb.SetMethods(EXTERNAL_CODECS_VARS options.HashMethods));
        hb.Init();
      }
      hresultMain = Extract(
          codecs,
          types,
          excludedFormats,
          options.ArchivePathsSorted,
          options.ArchivePathsFullSorted,
          options.Censor.Pairs.Front().Head,
          eo, &openCallback, ecs, hashCalc, errorMessage, stat);
      if (!errorMessage.IsEmpty())
      {
        stdStream << endl << "Error: " << errorMessage;
        if (hresultMain == S_OK)
          hresultMain = E_FAIL;
      }

      stdStream << endl;

      if (ecs->NumTryArcs > 1)
      {
        stdStream << "Archives: " << ecs->NumTryArcs << endl;
        stdStream << "OK archives: " << ecs->NumOkArcs << endl;
      }
      bool isError = false;
      if (ecs->NumCantOpenArcs != 0)
      {
        isError = true;
        stdStream << "Can't open as archive: " << ecs->NumCantOpenArcs << endl;
      }
      if (ecs->NumArcsWithError != 0)
      {
        isError = true;
        stdStream << "Archives with Errors: " << ecs->NumArcsWithError << endl;
      }
      if (ecs->NumArcsWithWarnings != 0)
        stdStream << "Archives with Warnings: " << ecs->NumArcsWithWarnings << endl;
      
      if (ecs->NumOpenArcWarnings != 0)
      {
        stdStream << endl;
        if (ecs->NumOpenArcWarnings != 0)
          stdStream << "Warnings: " << ecs->NumOpenArcWarnings << endl;
      }
      
      if (ecs->NumOpenArcErrors != 0)
      {
        isError = true;
        stdStream << endl;
        if (ecs->NumOpenArcErrors != 0)
          stdStream << "Open Errors: " << ecs->NumOpenArcErrors << endl;
      }

      if (isError)
        retCode = NExitCode::kFatalError;
      
      if (ecs->NumArcsWithError != 0 || ecs->NumFileErrors != 0)
      {
        // if (ecs->NumArchives > 1)
        {
          stdStream << endl;
          if (ecs->NumFileErrors != 0)
            stdStream << "Sub items Errors: " << ecs->NumFileErrors << endl;
        }
      }
      else if (hresultMain == S_OK)
      {
     
      if (stat.NumFolders != 0)
        stdStream << "Folders: " << stat.NumFolders << endl;
      if (stat.NumFiles != 1 || stat.NumFolders != 0 || stat.NumAltStreams != 0)
        stdStream << "Files: " << stat.NumFiles << endl;
      if (stat.NumAltStreams != 0)
      {
        stdStream << "Alternate Streams: " << stat.NumAltStreams << endl;
        stdStream << "Alternate Streams Size: " << stat.AltStreams_UnpackSize << endl;
      }

      stdStream
           << "Size:       " << stat.UnpackSize << endl
           << "Compressed: " << stat.PackSize << endl;
      if (hashCalc)
        PrintHashStat(stdStream, hb);
      }
    }
    else
    {
      UInt64 numErrors = 0;
      UInt64 numWarnings = 0;
      
      // options.ExtractNtOptions.StoreAltStreams = true, if -sns[-] is not definmed

      hresultMain = ListArchives(
          codecs,
          types,
          excludedFormats,
          options.StdInMode,
          options.ArchivePathsSorted,
          options.ArchivePathsFullSorted,
          options.ExtractOptions.NtOptions.AltStreams.Val,
          options.AltStreams.Val, // we don't want to show AltStreams by default
          options.Censor.Pairs.Front().Head,
          options.EnableHeaders,
          options.TechMode,
          #ifndef _NO_CRYPTO
          options.PasswordEnabled,
          options.Password,
          #endif
          &options.Properties,
          numErrors, numWarnings);

        if (options.EnableHeaders)
          if (numWarnings > 0)
            g_StdOut << endl << "Warnings: " << numWarnings << endl;
      if (numErrors > 0)
      {
        if (options.EnableHeaders)
          g_StdOut << endl << "Errors: " << numErrors << endl;
        retCode = NExitCode::kFatalError;
      }
    }
  }
  else if (options.Command.IsFromUpdateGroup())
  {
    CUpdateOptions &uo = options.UpdateOptions;
    if (uo.SfxMode && uo.SfxModule.IsEmpty())
      uo.SfxModule = kDefaultSfxModule;

    COpenCallbackConsole openCallback;
    openCallback.OutStream = &stdStream;

    #ifndef _NO_CRYPTO
    bool passwordIsDefined =
        options.PasswordEnabled && !options.Password.IsEmpty();
    openCallback.PasswordIsDefined = passwordIsDefined;
    openCallback.Password = options.Password;
    #endif

    CUpdateCallbackConsole callback;
    callback.EnablePercents = options.EnablePercents;

    #ifndef _NO_CRYPTO
    callback.PasswordIsDefined = passwordIsDefined;
    callback.AskPassword = options.PasswordEnabled && options.Password.IsEmpty();
    callback.Password = options.Password;
    #endif
    callback.StdOutMode = uo.StdOutMode;
    callback.Init(&stdStream);

    CUpdateErrorInfo errorInfo;

    /*
    if (!uo.Init(codecs, types, options.ArchiveName))
      throw kUnsupportedUpdateArcType;
    */
    hresultMain = UpdateArchive(codecs,
        types,
        options.ArchiveName,
        options.Censor,
        uo,
        errorInfo, &openCallback, &callback, true);
    retCode = WarningsCheck(hresultMain, callback, errorInfo, stdStream);
  }
  else if (options.Command.CommandType == NCommandType::kHash)
  {
    const CHashOptions &uo = options.HashOptions;

    CHashCallbackConsole callback;
    callback.EnablePercents = options.EnablePercents;

    callback.Init(&stdStream);

    UString errorInfoString;
    hresultMain = HashCalc(EXTERNAL_CODECS_VARS
        options.Censor, uo,
        errorInfoString, &callback);
    CErrorInfo errorInfo;
    errorInfo.Message = errorInfoString;
    retCode = WarningsCheck(hresultMain, callback, errorInfo, stdStream);
  }
  else
    ShowMessageAndThrowException(stdStream, kUserErrorMessage, NExitCode::kUserError);

  if (showStat)
    PrintStat();

  ThrowException_if_Error(hresultMain);

  return retCode;
}
