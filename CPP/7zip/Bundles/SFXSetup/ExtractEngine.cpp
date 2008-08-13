// ExtractEngine.cpp

#include "StdAfx.h"

#include "ExtractEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"

#include "../../UI/Common/OpenArchive.h"

#include "../../UI/FileManager/FormatUtils.h"

#include "ExtractCallback.h"

using namespace NWindows;

static LPCWSTR kCantFindArchive = L"Can not find archive file";
static LPCWSTR kCantOpenArchive = L"Can not open the file as archive";

struct CThreadExtracting
{
  #ifndef _NO_PROGRESS
  bool ShowProgress;
  #endif
  CCodecs *Codecs;
  UString FileName;
  UString DestFolder;

  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IArchiveExtractCallback> ExtractCallback;

  CArchiveLink ArchiveLink;
  HRESULT Result;
  UString ErrorMessage;

  void Process()
  {
    NFile::NFind::CFileInfoW fi;
    if (!NFile::NFind::FindFile(FileName, fi))
    {
      ErrorMessage = kCantFindArchive;
      Result = E_FAIL;
      return;
    }
    
    Result = MyOpenArchive(Codecs, CIntVector(), FileName, ArchiveLink, ExtractCallbackSpec);
    if (Result != S_OK)
    {
      if (Result != S_OK)
      ErrorMessage = kCantOpenArchive;
      return;
    }

    UString dirPath = DestFolder;
    NFile::NName::NormalizeDirPathPrefix(dirPath);
    
    if (!NFile::NDirectory::CreateComplexDirectory(dirPath))
    {
      ErrorMessage = MyFormatNew(IDS_CANNOT_CREATE_FOLDER,
        #ifdef LANG
        0x02000603,
        #endif
        dirPath);
      Result = E_FAIL;
      return;
    }

    ExtractCallbackSpec->Init(ArchiveLink.GetArchive(), dirPath, L"Default", fi.MTime, 0);

    #ifndef _NO_PROGRESS
    if (ShowProgress)
      ExtractCallbackSpec->ProgressDialog.WaitCreating();
    #endif
    Result = ArchiveLink.GetArchive()->Extract(0, (UInt32)-1 , BoolToInt(false), ExtractCallback);
    #ifndef _NO_PROGRESS
    if (ShowProgress)
      ExtractCallbackSpec->ProgressDialog.MyClose();
    #endif
  }
  
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    ((CThreadExtracting *)param)->Process();
    return 0;
  }
};

HRESULT ExtractArchive(CCodecs *codecs,const UString &fileName, const UString &destFolder,
    bool showProgress, bool &isCorrupt, UString &errorMessage)
{
  isCorrupt = false;
  CThreadExtracting t;

  t.Codecs = codecs;
  t.FileName = fileName;
  t.DestFolder = destFolder;

  t.ExtractCallbackSpec = new CExtractCallbackImp;
  t.ExtractCallback = t.ExtractCallbackSpec;
  
  #ifndef _NO_PROGRESS

  t.ShowProgress = showProgress;
  if (showProgress)
  {
    NWindows::CThread thread;
    RINOK(thread.Create(CThreadExtracting::MyThreadFunction, &t));
    
    UString title;
    #ifdef LANG
    title = LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890);
    #else
    title = NWindows::MyLoadStringW(IDS_PROGRESS_EXTRACTING);
    #endif
    t.ExtractCallbackSpec->StartProgressDialog(title);
  }
  else

  #endif
  {
    t.Process();
  }

  errorMessage = t.ErrorMessage;
  if (errorMessage.IsEmpty())
    errorMessage = t.ExtractCallbackSpec->_message;
  isCorrupt = t.ExtractCallbackSpec->_isCorrupt;
  return t.Result;
}
