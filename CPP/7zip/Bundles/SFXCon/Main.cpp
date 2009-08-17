// Main.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"

#include "Common/CommandLineParser.h"
#include "Common/MyException.h"

#ifdef _WIN32
#include "Windows/DLL.h"
#include "Windows/FileDir.h"
#endif

#include "../../UI/Common/ExitCode.h"
#include "../../UI/Common/Extract.h"

#include "../../UI/Console/ExtractCallbackConsole.h"
#include "../../UI/Console/List.h"
#include "../../UI/Console/OpenCallbackConsole.h"

#include "../../MyVersion.h"

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

int g_CodePage = -1;
extern CStdOutStream *g_StdStream;

static const char *kCopyrightString =
"\n7-Zip SFX " MY_VERSION_COPYRIGHT_DATE "\n";

static const int kNumSwitches = 6;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kDisablePercents,
  kYes,
  kPassword,
  kOutputDir
};

}

namespace NRecursedType {
enum EEnum
{
  kRecursed,
  kWildCardOnlyRecursed,
  kNonRecursed
};
}
/*
static const char kRecursedIDChar = 'R';
static const wchar_t *kRecursedPostCharSet = L"0-";

namespace NRecursedPostCharIndex {
  enum EEnum
  {
    kWildCardRecursionOnly = 0,
    kNoRecursion = 1
  };
}

static const char kFileListID = '@';
static const char kImmediateNameID = '!';

static const char kSomeCludePostStringMinSize = 2; // at least <@|!><N>ame must be
static const char kSomeCludeAfterRecursedPostStringMinSize = 2; // at least <@|!><N>ame must be
*/
static const CSwitchForm kSwitchForms[kNumSwitches] =
  {
    { L"?",  NSwitchType::kSimple, false },
    { L"H",  NSwitchType::kSimple, false },
    { L"BD", NSwitchType::kSimple, false },
    { L"Y",  NSwitchType::kSimple, false },
    { L"P",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"O",  NSwitchType::kUnLimitedPostString, false, 1 },
  };

static const int kNumCommandForms = 3;

namespace NCommandType {
enum EEnum
{
  kTest = 0,
  // kExtract,
  kFullExtract,
  kList
};

}

static const CCommandForm commandForms[kNumCommandForms] =
{
  { L"T", false },
  // { "E", false },
  { L"X", false },
  { L"L", false }
};

static const NRecursedType::EEnum kCommandRecursedDefault[kNumCommandForms] =
{
  NRecursedType::kRecursed
};

// static const bool kTestExtractRecursedDefault = true;
// static const bool kAddRecursedDefault = false;

static const wchar_t *kUniversalWildcard = L"*";
static const int kCommandIndex = 0;

static const char *kHelpString =
    "\nUsage: 7zSFX [<command>] [<switches>...]\n"
    "\n"
    "<Commands>\n"
    "  l: List contents of archive\n"
    "  t: Test integrity of archive\n"
    "  x: eXtract files with full pathname (default)\n"
    "<Switches>\n"
    // "  -bd Disable percentage indicator\n"
    "  -o{Directory}: set Output directory\n"
    "  -p{Password}: set Password\n"
    "  -y: assume Yes on all queries\n";


// ---------------------------
// exception messages

static const char *kUserErrorMessage  = "Incorrect command line"; // NExitCode::kUserError
// static const char *kIncorrectListFile = "Incorrect wildcard in listfile";
static const char *kIncorrectWildCardInCommandLine  = "Incorrect wildcard in command line";

// static const CSysString kFileIsNotArchiveMessageBefore = "File \"";
// static const CSysString kFileIsNotArchiveMessageAfter = "\" is not archive";

// static const char *kProcessArchiveMessage = " archive: ";

static const char *kCantFindSFX = " cannot find sfx";


struct CArchiveCommand
{
  NCommandType::EEnum CommandType;
  NRecursedType::EEnum DefaultRecursedType() const;
};

NRecursedType::EEnum CArchiveCommand::DefaultRecursedType() const
{
  return kCommandRecursedDefault[CommandType];
}

void PrintHelp(void)
{
  g_StdOut << kHelpString;
}

static void ShowMessageAndThrowException(const char *message, NExitCode::EEnum code)
{
  g_StdOut << message << endl;
  throw code;
}

static void PrintHelpAndExit() // yyy
{
  PrintHelp();
  ShowMessageAndThrowException(kUserErrorMessage, NExitCode::kUserError);
}

bool ParseArchiveCommand(const UString &commandString, CArchiveCommand &command)
{
  UString commandStringUpper = commandString;
  commandStringUpper.MakeUpper();
  UString postString;
  int commandIndex = ParseCommand(kNumCommandForms, commandForms, commandStringUpper,
      postString) ;
  if (commandIndex < 0)
    return false;
  command.CommandType = (NCommandType::EEnum)commandIndex;
  return true;
}

// ------------------------------------------------------------------
// filenames functions

static bool AddNameToCensor(NWildcard::CCensor &wildcardCensor,
    const UString &name, bool include, NRecursedType::EEnum type)
{
  /*
  if (!IsWildCardFilePathLegal(name))
    return false;
  */
  bool isWildCard = DoesNameContainWildCard(name);
  bool recursed = false;

  switch (type)
  {
    case NRecursedType::kWildCardOnlyRecursed:
      recursed = isWildCard;
      break;
    case NRecursedType::kRecursed:
      recursed = true;
      break;
    case NRecursedType::kNonRecursed:
      recursed = false;
      break;
  }
  wildcardCensor.AddItem(include, name, recursed);
  return true;
}

void AddCommandLineWildCardToCensor(NWildcard::CCensor &wildcardCensor,
    const UString &name, bool include, NRecursedType::EEnum type)
{
  if (!AddNameToCensor(wildcardCensor, name, include, type))
    ShowMessageAndThrowException(kIncorrectWildCardInCommandLine, NExitCode::kUserError);
}

void AddToCensorFromNonSwitchesStrings(NWildcard::CCensor &wildcardCensor,
    const UStringVector & /* nonSwitchStrings */, NRecursedType::EEnum type,
    bool /* thereAreSwitchIncludeWildCards */)
{
  AddCommandLineWildCardToCensor(wildcardCensor, kUniversalWildcard, true, type);
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

int Main2(
  #ifndef _WIN32
  int numArgs, const char *args[]
  #endif
)
{
  #if defined(_WIN32) && !defined(UNDER_CE)
  SetFileApisToOEM();
  #endif
  
  g_StdOut << kCopyrightString;

  UStringVector commandStrings;
  #ifdef _WIN32
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);
  #else
  GetArguments(numArgs, args, commandStrings);
  #endif

  #ifdef _WIN32
  
  UString arcPath;
  {
    UString path;
    NDLL::MyGetModuleFileName(NULL, path);
    int fileNamePartStartIndex;
    if (!NDirectory::MyGetFullPathName(path, arcPath, fileNamePartStartIndex))
    {
      g_StdOut << "GetFullPathName Error";
      return NExitCode::kFatalError;
    }
  }

  #else

  UString arcPath = commandStrings.Front();

  #endif

  commandStrings.Delete(0);

  NCommandLineParser::CParser parser(kNumSwitches);
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  catch(...)
  {
    PrintHelpAndExit();
  }

  if(parser[NKey::kHelp1].ThereIs || parser[NKey::kHelp2].ThereIs)
  {
    PrintHelp();
    return 0;
  }
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;

  int numNonSwitchStrings = nonSwitchStrings.Size();

  CArchiveCommand command;
  if (numNonSwitchStrings == 0)
    command.CommandType = NCommandType::kFullExtract;
  else
  {
    if (numNonSwitchStrings > 1)
      PrintHelpAndExit();
    if (!ParseArchiveCommand(nonSwitchStrings[kCommandIndex], command))
      PrintHelpAndExit();
  }


  NRecursedType::EEnum recursedType;
  recursedType = command.DefaultRecursedType();

  NWildcard::CCensor wildcardCensor;
  
  bool thereAreSwitchIncludeWildCards;
  thereAreSwitchIncludeWildCards = false;
  AddToCensorFromNonSwitchesStrings(wildcardCensor, nonSwitchStrings, recursedType,
      thereAreSwitchIncludeWildCards);

  bool yesToAll = parser[NKey::kYes].ThereIs;

  // NExtractMode::EEnum extractMode;
  // bool isExtractGroupCommand = command.IsFromExtractGroup(extractMode);

  bool passwordEnabled = parser[NKey::kPassword].ThereIs;

  UString password;
  if(passwordEnabled)
    password = parser[NKey::kPassword].PostStrings[0];

  if (!NFind::DoesFileExist(arcPath))
    throw kCantFindSFX;
  
  UString outputDir;
  if (parser[NKey::kOutputDir].ThereIs)
  {
    outputDir = parser[NKey::kOutputDir].PostStrings[0];
    NName::NormalizeDirPathPrefix(outputDir);
  }

  {
    UStringVector v1, v2;
    v1.Add(arcPath);
    v2.Add(arcPath);
    const NWildcard::CCensorNode &wildcardCensorHead =
      wildcardCensor.Pairs.Front().Head;

    CCodecs *codecs = new CCodecs;
    CMyComPtr<
      #ifdef EXTERNAL_CODECS
      ICompressCodecsInfo
      #else
      IUnknown
      #endif
      > compressCodecsInfo = codecs;
    HRESULT result = codecs->Load();
    if (result != S_OK)
      throw CSystemException(result);

    if(command.CommandType != NCommandType::kList)
    {
      CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
      CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;
      ecs->OutStream = g_StdStream;

      #ifndef _NO_CRYPTO
      ecs->PasswordIsDefined = passwordEnabled;
      ecs->Password = password;
      #endif

      ecs->Init();

      COpenCallbackConsole openCallback;
      openCallback.OutStream = g_StdStream;

      #ifndef _NO_CRYPTO
      openCallback.PasswordIsDefined = passwordEnabled;
      openCallback.Password = password;
      #endif

      CExtractOptions eo;
      eo.StdOutMode = false;
      eo.PathMode = NExtract::NPathMode::kFullPathnames;
      eo.TestMode = command.CommandType == NCommandType::kTest;
      eo.OverwriteMode = yesToAll ?
          NExtract::NOverwriteMode::kWithoutPrompt :
          NExtract::NOverwriteMode::kAskBefore;
      eo.OutputDir = outputDir;
      eo.YesToAll = yesToAll;

      UString errorMessage;
      CDecompressStat stat;
      HRESULT result = DecompressArchives(
          codecs, CIntVector(),
          v1, v2,
          wildcardCensorHead,
          eo, &openCallback, ecs, errorMessage, stat);
      if (!errorMessage.IsEmpty())
      {
        (*g_StdStream) << endl << "Error: " << errorMessage;;
        if (result == S_OK)
          result = E_FAIL;
      }

      if (ecs->NumArchiveErrors != 0 || ecs->NumFileErrors != 0)
      {
        if (ecs->NumArchiveErrors != 0)
          (*g_StdStream) << endl << "Archive Errors: " << ecs->NumArchiveErrors << endl;
        if (ecs->NumFileErrors != 0)
          (*g_StdStream) << endl << "Sub items Errors: " << ecs->NumFileErrors << endl;
        return NExitCode::kFatalError;
      }
      if (result != S_OK)
        throw CSystemException(result);
    }
    else
    {
      UInt64 numErrors = 0;
      HRESULT result = ListArchives(
          codecs, CIntVector(),
          false,
          v1, v2,
          wildcardCensorHead,
          true, false,
          #ifndef _NO_CRYPTO
          passwordEnabled, password,
          #endif
          numErrors);
      if (numErrors > 0)
      {
        g_StdOut << endl << "Errors: " << numErrors;
        return NExitCode::kFatalError;
      }
      if (result != S_OK)
        throw CSystemException(result);
    }
  }
  return 0;
}
