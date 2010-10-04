// ExtractGUI.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"

#include "../FileManager/ExtractCallback.h"
#include "../FileManager/FormatUtils.h"
#include "../FileManager/LangUtils.h"
#include "../FileManager/resourceGui.h"

#include "../Common/ArchiveExtractCallback.h"
#include "../Common/PropIDUtils.h"

#include "../Explorer/MyMessages.h"

#include "resource2.h"
#include "ExtractRes.h"

#include "ExtractDialog.h"
#include "ExtractGUI.h"

using namespace NWindows;

static const wchar_t *kIncorrectOutDir = L"Incorrect output directory path";

#ifndef _SFX

static void AddValuePair(UINT resourceID, UInt32 langID, UInt64 value, UString &s)
{
  wchar_t sz[32];
  s += LangString(resourceID, langID);
  s += L' ';
  ConvertUInt64ToString(value, sz);
  s += sz;
  s += L'\n';
}

static void AddSizePair(UINT resourceID, UInt32 langID, UInt64 value, UString &s)
{
  wchar_t sz[32];
  s += LangString(resourceID, langID);
  s += L' ';
  ConvertUInt64ToString(value, sz);
  s += sz;
  ConvertUInt64ToString(value >> 20, sz);
  s += L" (";
  s += sz;
  s += L" MB)";
  s += L'\n';
}

#endif

class CThreadExtracting: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  CCodecs *codecs;
  CExtractCallbackImp *ExtractCallbackSpec;
  CIntVector FormatIndices;

  UStringVector *ArchivePaths;
  UStringVector *ArchivePathsFull;
  const NWildcard::CCensorNode *WildcardCensor;
  const CExtractOptions *Options;
  CMyComPtr<IExtractCallbackUI> ExtractCallback;
  UString Title;
};

HRESULT CThreadExtracting::ProcessVirt()
{
  CDecompressStat Stat;
  HRESULT res = DecompressArchives(codecs, FormatIndices, *ArchivePaths, *ArchivePathsFull,
      *WildcardCensor, *Options, ExtractCallbackSpec, ExtractCallback, ErrorMessage, Stat);
  #ifndef _SFX
  if (Options->TestMode && ExtractCallbackSpec->IsOK())
  {
    UString s;
    AddValuePair(IDS_ARCHIVES_COLON, 0x02000324, Stat.NumArchives, s);
    AddValuePair(IDS_FOLDERS_COLON, 0x02000321, Stat.NumFolders, s);
    AddValuePair(IDS_FILES_COLON, 0x02000320, Stat.NumFiles, s);
    AddSizePair(IDS_SIZE_COLON, 0x02000322, Stat.UnpackSize, s);
    AddSizePair(IDS_COMPRESSED_COLON, 0x02000323, Stat.PackSize, s);
    
    if (Options->CalcCrc)
    {
      wchar_t temp[16];
      ConvertUInt32ToHex(Stat.CrcSum, temp);
      s += L"CRC: ";
      s += temp;
      s += L'\n';
    }
    
    s += L'\n';
    s += LangString(IDS_MESSAGE_NO_ERRORS, 0x02000608);
    
    OkMessageTitle = Title;
    OkMessage = s;
  }
  #endif
  return res;
}

HRESULT ExtractGUI(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    UStringVector &archivePaths,
    UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    CExtractOptions &options,
    bool showDialog,
    bool &messageWasDisplayed,
    CExtractCallbackImp *extractCallback,
    HWND hwndParent)
{
  messageWasDisplayed = false;

  CThreadExtracting extracter;
  extracter.codecs = codecs;
  extracter.FormatIndices = formatIndices;

  if (!options.TestMode)
  {
    UString outputDir = options.OutputDir;
    #ifndef UNDER_CE
    if (outputDir.IsEmpty())
      NFile::NDirectory::MyGetCurrentDirectory(outputDir);
    #endif
    if (showDialog)
    {
      CExtractDialog dialog;
      if (!NFile::NDirectory::MyGetFullPathName(outputDir, dialog.DirectoryPath))
      {
        ShowErrorMessage(kIncorrectOutDir);
        messageWasDisplayed = true;
        return E_FAIL;
      }
      NFile::NName::NormalizeDirPathPrefix(dialog.DirectoryPath);

      // dialog.OverwriteMode = options.OverwriteMode;
      // dialog.PathMode = options.PathMode;

      if (dialog.Create(hwndParent) != IDOK)
        return E_ABORT;
      outputDir = dialog.DirectoryPath;
      options.OverwriteMode = dialog.OverwriteMode;
      options.PathMode = dialog.PathMode;
      #ifndef _SFX
      extractCallback->Password = dialog.Password;
      extractCallback->PasswordIsDefined = !dialog.Password.IsEmpty();
      #endif
    }
    if (!NFile::NDirectory::MyGetFullPathName(outputDir, options.OutputDir))
    {
      ShowErrorMessage(kIncorrectOutDir);
      messageWasDisplayed = true;
      return E_FAIL;
    }
    NFile::NName::NormalizeDirPathPrefix(options.OutputDir);
    
    /*
    if(!NFile::NDirectory::CreateComplexDirectory(options.OutputDir))
    {
      UString s = GetUnicodeString(NError::MyFormatMessage(GetLastError()));
      UString s2 = MyFormatNew(IDS_CANNOT_CREATE_FOLDER,
      #ifdef LANG
      0x02000603,
      #endif
      options.OutputDir);
      MyMessageBox(s2 + UString(L'\n') + s);
      return E_FAIL;
    }
    */
  }
  
  UString title = LangStringSpec(options.TestMode ? IDS_PROGRESS_TESTING : IDS_PROGRESS_EXTRACTING,
      options.TestMode ? 0x02000F90: 0x02000890);

  extracter.Title = title;
  extracter.ExtractCallbackSpec = extractCallback;
  extracter.ExtractCallbackSpec->ProgressDialog = &extracter.ProgressDialog;
  extracter.ExtractCallback = extractCallback;
  extracter.ExtractCallbackSpec->Init();

  extracter.ProgressDialog.CompressingMode = false;

  extracter.ArchivePaths = &archivePaths;
  extracter.ArchivePathsFull = &archivePathsFull;
  extracter.WildcardCensor = &wildcardCensor;
  extracter.Options = &options;

  extracter.ProgressDialog.IconID = IDI_ICON;

  RINOK(extracter.Create(title, hwndParent));
  messageWasDisplayed = extracter.ThreadFinishedOK &
      extracter.ProgressDialog.MessagesDisplayed;
  return extracter.Result;
}
