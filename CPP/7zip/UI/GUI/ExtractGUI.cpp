// ExtractGUI.cpp

#include "StdAfx.h"

#include "ExtractGUI.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/Error.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"

#include "../../FileManager/FormatUtils.h"
#include "../../FileManager/ExtractCallback.h"
#include "../../FileManager/LangUtils.h"

#include "../Common/ArchiveExtractCallback.h"
#include "../Explorer/MyMessages.h"
#include "../Resource/Extract/resource.h"

#include "OpenCallbackGUI.h"
#include "ExtractDialog.h"

using namespace NWindows;

static const wchar_t *kIncorrectOutDir = L"Incorrect output directory path";

struct CThreadExtracting
{
  CExtractCallbackImp *ExtractCallbackSpec;

  UStringVector *ArchivePaths;
  UStringVector *ArchivePathsFull;
  const NWildcard::CCensorNode *WildcardCensor;
  const CExtractOptions *Options;
  COpenCallbackGUI *OpenCallback;
  CMyComPtr<IExtractCallbackUI> ExtractCallback;

  UString ErrorMessage;
  HRESULT Result;
  
  DWORD Process()
  {
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    try
    {
      Result = DecompressArchives(*ArchivePaths, *ArchivePathsFull,
          *WildcardCensor, *Options, OpenCallback, ExtractCallback, ErrorMessage);
    }
    catch(const UString &s)
    {
      ErrorMessage = s;
      Result = E_FAIL;
    } 
    catch(const wchar_t *s)
    {
      ErrorMessage = s;
      Result = E_FAIL;
    } 
    catch(const char *s)
    {
      ErrorMessage = GetUnicodeString(s);
      Result = E_FAIL;
    }
    catch(...)
    {
      Result = E_FAIL;
    }
    ExtractCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadExtracting *)param)->Process();
  }
};

HRESULT ExtractGUI(
    UStringVector &archivePaths, 
    UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    CExtractOptions &options,
    bool showDialog,
    COpenCallbackGUI *openCallback,
    CExtractCallbackImp *extractCallback)
{
  CThreadExtracting extracter;

  if (!options.TestMode)
  {
    UString outputDir = options.OutputDir;
    if (outputDir.IsEmpty())
      NFile::NDirectory::MyGetCurrentDirectory(outputDir);
    if (showDialog)
    {
      CExtractDialog dialog;
      if (!NFile::NDirectory::MyGetFullPathName(outputDir, dialog.DirectoryPath))
      {
        MyMessageBox(kIncorrectOutDir);
        return E_FAIL;
      }
      NFile::NName::NormalizeDirPathPrefix(dialog.DirectoryPath);

      // dialog.OverwriteMode = options.OverwriteMode;
      // dialog.PathMode = options.PathMode;

      if(dialog.Create(0) != IDOK)
        return E_ABORT;
      outputDir = dialog.DirectoryPath;
      options.OverwriteMode = dialog.OverwriteMode;
      options.PathMode = dialog.PathMode;
      #ifndef _SFX
      openCallback->Password = dialog.Password;
      openCallback->PasswordIsDefined = !dialog.Password.IsEmpty();
      #endif
    }
    if (!NFile::NDirectory::MyGetFullPathName(outputDir, options.OutputDir))
    {
      MyMessageBox(kIncorrectOutDir);
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
      MyMessageBox(s2 + UString(L"\n") + s);
      return E_FAIL;
    }
    */
  }
  
  UString title = LangStringSpec(options.TestMode ? IDS_PROGRESS_TESTING : IDS_PROGRESS_EXTRACTING, 
      options.TestMode ? 0x02000F90: 0x02000890);

  extracter.ExtractCallbackSpec = extractCallback;
  extracter.ExtractCallback = extractCallback;
  extracter.ExtractCallbackSpec->Init();

  extracter.ArchivePaths = &archivePaths;
  extracter.ArchivePathsFull = &archivePathsFull;
  extracter.WildcardCensor = &wildcardCensor;
  extracter.Options = &options;
  extracter.OpenCallback = openCallback;

  CThread thread;
  if (!thread.Create(CThreadExtracting::MyThreadFunction, &extracter))
    throw 271824;
  extracter.ExtractCallbackSpec->StartProgressDialog(title);
  if (extracter.Result == S_OK && options.TestMode && 
      extracter.ExtractCallbackSpec->Messages.IsEmpty() &&
      extracter.ExtractCallbackSpec->NumArchiveErrors == 0)
  {
    #ifndef _SFX
    MessageBoxW(0, LangString(IDS_MESSAGE_NO_ERRORS, 0x02000608),
      LangString(IDS_PROGRESS_TESTING, 0x02000F90), 0);
    #endif
  }
  if (extracter.Result != S_OK)
    if (!extracter.ErrorMessage.IsEmpty())
      throw extracter.ErrorMessage;
  return extracter.Result;
}


