// PanelSplitFile.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"

#include "Windows/Error.h"
#include "Windows/FileIO.h"
#include "Windows/FileFind.h"

#include "../GUI/ExtractRes.h"

#include "resource.h"

#include "App.h"
#include "CopyDialog.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "SplitDialog.h"
#include "SplitUtils.h"

using namespace NWindows;

static const wchar_t *g_Message_FileWriteError = L"File write error";

struct CVolSeqName
{
  UString UnchangedPart;
  UString ChangedPart;
  CVolSeqName(): ChangedPart(L"000") {};

  void SetNumDigits(UInt64 numVolumes)
  {
    ChangedPart = L"000";
    while (numVolumes > 999)
    {
      numVolumes /= 10;
      ChangedPart += L'0';
    }
  }

  bool ParseName(const UString &name)
  {
    if (name.Right(2) != L"01")
      return false;
    int numLetters = 2;
    while (numLetters < name.Length())
    {
      if (name[name.Length() - numLetters - 1] != '0')
        break;
      numLetters++;
    }
    UnchangedPart = name.Left(name.Length() - numLetters);
    ChangedPart = name.Right(numLetters);
    return true;
  }

  UString GetNextName()
  {
    UString newName;
    int i;
    int numLetters = ChangedPart.Length();
    for (i = numLetters - 1; i >= 0; i--)
    {
      wchar_t c = ChangedPart[i];
      if (c == L'9')
      {
        c = L'0';
        newName = c + newName;
        if (i == 0)
          newName = UString(L'1') + newName;
        continue;
      }
      c++;
      newName = c + newName;
      i--;
      for (; i >= 0; i--)
        newName = ChangedPart[i] + newName;
      break;
    }
    ChangedPart = newName;
    return UnchangedPart + ChangedPart;
  }
};

static const UInt32 kBufSize = (1 << 20);

class CThreadSplit: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  UString FilePath;
  UString VolBasePath;
  UInt64 NumVolumes;
  CRecordVector<UInt64> VolumeSizes;
};

HRESULT CThreadSplit::ProcessVirt()
{
  NFile::NIO::CInFile inFile;
  if (!inFile.Open(FilePath))
    return GetLastError();
  NFile::NIO::COutFile outFile;
  CMyBuffer bufferObject;
  if (!bufferObject.Allocate(kBufSize))
    return E_OUTOFMEMORY;
  Byte *buffer = (Byte *)(void *)bufferObject;
  UInt64 curVolSize = 0;
  CVolSeqName seqName;
  seqName.SetNumDigits(NumVolumes);
  UInt64 length;
  if (!inFile.GetLength(length))
    return GetLastError();
  
  CProgressSync &sync = ProgressDialog.Sync;
  sync.SetProgress(length, 0);
  UInt64 pos = 0;
  
  UInt64 numFiles = 0;
  int volIndex = 0;
  
  for (;;)
  {
    UInt64 volSize;
    if (volIndex < VolumeSizes.Size())
      volSize = VolumeSizes[volIndex];
    else
      volSize = VolumeSizes.Back();
    
    UInt32 needSize = (UInt32)(MyMin((UInt64)kBufSize, volSize - curVolSize));
    UInt32 processedSize;
    if (!inFile.Read(buffer, needSize, processedSize))
      return GetLastError();
    if (processedSize == 0)
      break;
    needSize = processedSize;
    if (curVolSize == 0)
    {
      UString name = VolBasePath;
      name += L'.';
      name += seqName.GetNextName();
      sync.SetCurrentFileName(name);
      sync.SetNumFilesCur(numFiles++);
      if (!outFile.Create(name, false))
      {
        HRESULT res = GetLastError();
        ErrorPath1 = name;
        return res;
      }
    }
    if (!outFile.Write(buffer, needSize, processedSize))
      return GetLastError();
    if (needSize != processedSize)
      throw g_Message_FileWriteError;
    curVolSize += processedSize;
    if (curVolSize == volSize)
    {
      outFile.Close();
      if (volIndex < VolumeSizes.Size())
        volIndex++;
      curVolSize = 0;
    }
    pos += processedSize;
    RINOK(sync.SetPosAndCheckPaused(pos));
  }
  sync.SetNumFilesCur(numFiles);
  return S_OK;
}

void CApp::Split()
{
  int srcPanelIndex = GetFocusedPanelIndex();
  CPanel &srcPanel = Panels[srcPanelIndex];
  if (!srcPanel.IsFSFolder())
  {
    srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return;
  }
  CRecordVector<UInt32> indices;
  srcPanel.GetOperatedItemIndices(indices);
  if (indices.IsEmpty())
    return;
  if (indices.Size() != 1)
  {
    srcPanel.MessageBoxErrorLang(IDS_SELECT_ONE_FILE, 0x03020A02);
    return;
  }
  int index = indices[0];
  if (srcPanel.IsItemFolder(index))
  {
    srcPanel.MessageBoxErrorLang(IDS_SELECT_ONE_FILE, 0x03020A02);
    return;
  }
  const UString itemName = srcPanel.GetItemName(index);

  UString srcPath = srcPanel._currentFolderPrefix + srcPanel.GetItemPrefix(index);
  UString path = srcPath;
  int destPanelIndex = (NumPanels <= 1) ? srcPanelIndex : (1 - srcPanelIndex);
  CPanel &destPanel = Panels[destPanelIndex];
  if (NumPanels > 1)
    if (destPanel.IsFSFolder())
      path = destPanel._currentFolderPrefix;
  CSplitDialog splitDialog;
  splitDialog.FilePath = srcPanel.GetItemRelPath(index);
  splitDialog.Path = path;
  if (splitDialog.Create(srcPanel.GetParent()) == IDCANCEL)
    return;

  NFile::NFind::CFileInfoW fileInfo;
  if (!fileInfo.Find(srcPath + itemName))
  {
    srcPanel.MessageBoxMyError(L"Can not find file");
    return;
  }
  if (fileInfo.Size <= splitDialog.VolumeSizes.Front())
  {
    srcPanel.MessageBoxErrorLang(IDS_SPLIT_VOL_MUST_BE_SMALLER, 0x03020522);
    return;
  }
  const UInt64 numVolumes = GetNumberOfVolumes(fileInfo.Size, splitDialog.VolumeSizes);
  if (numVolumes >= 100)
  {
    wchar_t s[32];
    ConvertUInt64ToString(numVolumes, s);
    if (::MessageBoxW(srcPanel, MyFormatNew(IDS_SPLIT_CONFIRM_MESSAGE, 0x03020521, s),
        LangString(IDS_SPLIT_CONFIRM_TITLE, 0x03020520),
        MB_YESNOCANCEL | MB_ICONQUESTION) != IDYES)
      return;
  }

  path = splitDialog.Path;
  NFile::NName::NormalizeDirPathPrefix(path);
  if (!NFile::NDirectory::CreateComplexDirectory(path))
  {
    srcPanel.MessageBoxMyError(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 0x02000603, path));
    return;
  }

  {
  CThreadSplit spliter;
  spliter.NumVolumes = numVolumes;

  CProgressDialog &progressDialog = spliter.ProgressDialog;

  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
  UString title = LangString(IDS_SPLITTING, 0x03020510);

  progressDialog.ShowCompressionInfo = false;

  progressDialog.MainWindow = _window;
  progressDialog.MainTitle = progressWindowTitle;
  progressDialog.MainAddTitle = title + UString(L" ");
  progressDialog.Sync.SetTitleFileName(itemName);


  spliter.FilePath = srcPath + itemName;
  spliter.VolBasePath = path + itemName;
  spliter.VolumeSizes = splitDialog.VolumeSizes;
  
  // if (splitDialog.VolumeSizes.Size() == 0) return;

  // CPanel::CDisableTimerProcessing disableTimerProcessing1(srcPanel);
  // CPanel::CDisableTimerProcessing disableTimerProcessing2(destPanel);

  if (spliter.Create(title, _window) != 0)
    return;
  }
  RefreshTitleAlways();


  // disableTimerProcessing1.Restore();
  // disableTimerProcessing2.Restore();
  // srcPanel.SetFocusToList();
  // srcPanel.RefreshListCtrlSaveFocused();
}


class CThreadCombine: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  UString InputDirPrefix;
  UStringVector Names;
  UString OutputPath;
  UInt64 TotalSize;
};

HRESULT CThreadCombine::ProcessVirt()
{
  NFile::NIO::COutFile outFile;
  if (!outFile.Create(OutputPath, false))
  {
    HRESULT res = GetLastError();
    ErrorPath1 = OutputPath;
    return res;
  }
  
  CProgressSync &sync = ProgressDialog.Sync;
  sync.SetProgress(TotalSize, 0);
  
  CMyBuffer bufferObject;
  if (!bufferObject.Allocate(kBufSize))
    return E_OUTOFMEMORY;
  Byte *buffer = (Byte *)(void *)bufferObject;
  UInt64 pos = 0;
  for (int i = 0; i < Names.Size(); i++)
  {
    NFile::NIO::CInFile inFile;
    const UString nextName = InputDirPrefix + Names[i];
    if (!inFile.Open(nextName))
    {
      HRESULT res = GetLastError();
      ErrorPath1 = nextName;
      return res;
    }
    sync.SetCurrentFileName(nextName);
    for (;;)
    {
      UInt32 processedSize;
      if (!inFile.Read(buffer, kBufSize, processedSize))
      {
        HRESULT res = GetLastError();
        ErrorPath1 = nextName;
        return res;
      }
      if (processedSize == 0)
        break;
      UInt32 needSize = processedSize;
      if (!outFile.Write(buffer, needSize, processedSize))
      {
        HRESULT res = GetLastError();
        ErrorPath1 = OutputPath;
        return res;
      }
      if (needSize != processedSize)
        throw g_Message_FileWriteError;
      pos += processedSize;
      RINOK(sync.SetPosAndCheckPaused(pos));
    }
  }
  return S_OK;
}

extern void AddValuePair2(UINT resourceID, UInt32 langID, UInt64 num, UInt64 size, UString &s);

static void AddInfoFileName(const UString &name, UString &dest)
{
  dest += L"\n  ";
  dest += name;
}

void CApp::Combine()
{
  int srcPanelIndex = GetFocusedPanelIndex();
  CPanel &srcPanel = Panels[srcPanelIndex];
  if (!srcPanel.IsFSFolder())
  {
    srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return;
  }
  CRecordVector<UInt32> indices;
  srcPanel.GetOperatedItemIndices(indices);
  if (indices.IsEmpty())
    return;
  int index = indices[0];
  if (indices.Size() != 1 || srcPanel.IsItemFolder(index))
  {
    srcPanel.MessageBoxErrorLang(IDS_COMBINE_SELECT_ONE_FILE, 0x03020620);
    return;
  }
  const UString itemName = srcPanel.GetItemName(index);

  UString srcPath = srcPanel._currentFolderPrefix + srcPanel.GetItemPrefix(index);
  UString path = srcPath;
  int destPanelIndex = (NumPanels <= 1) ? srcPanelIndex : (1 - srcPanelIndex);
  CPanel &destPanel = Panels[destPanelIndex];
  if (NumPanels > 1)
    if (destPanel.IsFSFolder())
      path = destPanel._currentFolderPrefix;

  CVolSeqName volSeqName;
  if (!volSeqName.ParseName(itemName))
  {
    srcPanel.MessageBoxErrorLang(IDS_COMBINE_CANT_DETECT_SPLIT_FILE, 0x03020621);
    return;
  }
  
  {
  CThreadCombine combiner;
  
  UString nextName = itemName;
  combiner.TotalSize = 0;
  for (;;)
  {
    NFile::NFind::CFileInfoW fileInfo;
    if (!fileInfo.Find(srcPath + nextName) || fileInfo.IsDir())
      break;
    combiner.Names.Add(nextName);
    combiner.TotalSize += fileInfo.Size;
    nextName = volSeqName.GetNextName();
  }
  if (combiner.Names.Size() == 1)
  {
    srcPanel.MessageBoxErrorLang(IDS_COMBINE_CANT_FIND_MORE_THAN_ONE_PART, 0x03020622);
    return;
  }
  
  if (combiner.TotalSize == 0)
  {
    srcPanel.MessageBoxMyError(L"No data");
    return;
  }
  
  UString info;
  AddValuePair2(IDS_FILES_COLON, 0x02000320, combiner.Names.Size(), combiner.TotalSize, info);
  
  info += L"\n";
  info += srcPath;
  
  int i;
  for (i = 0; i < combiner.Names.Size() && i < 2; i++)
    AddInfoFileName(combiner.Names[i], info);
  if (i != combiner.Names.Size())
  {
    if (i + 1 != combiner.Names.Size())
      AddInfoFileName(L"...", info);
    AddInfoFileName(combiner.Names.Back(), info);
  }
  
  {
    CCopyDialog copyDialog;
    copyDialog.Value = path;
    copyDialog.Title = LangString(IDS_COMBINE, 0x03020600);
    copyDialog.Title += ' ';
    copyDialog.Title += srcPanel.GetItemRelPath(index);
    copyDialog.Static = LangString(IDS_COMBINE_TO, 0x03020601);
    copyDialog.Info = info;
    if (copyDialog.Create(srcPanel.GetParent()) == IDCANCEL)
      return;
    path = copyDialog.Value;
  }

  NFile::NName::NormalizeDirPathPrefix(path);
  if (!NFile::NDirectory::CreateComplexDirectory(path))
  {
    srcPanel.MessageBoxMyError(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 0x02000603, path));
    return;
  }
  
  UString outName = volSeqName.UnchangedPart;
  while (!outName.IsEmpty())
  {
    int lastIndex = outName.Length() - 1;
    if (outName[lastIndex] != L'.')
      break;
    outName.Delete(lastIndex);
  }
  if (outName.IsEmpty())
    outName = L"file";
  
  NFile::NFind::CFileInfoW fileInfo;
  UString destFilePath = path + outName;
  combiner.OutputPath = destFilePath;
  if (fileInfo.Find(destFilePath))
  {
    srcPanel.MessageBoxMyError(MyFormatNew(IDS_FILE_EXIST, 0x03020A04, destFilePath));
    return;
  }
  
    CProgressDialog &progressDialog = combiner.ProgressDialog;
    progressDialog.ShowCompressionInfo = false;
  
    UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
    UString title = LangString(IDS_COMBINING, 0x03020610);
    
    progressDialog.MainWindow = _window;
    progressDialog.MainTitle = progressWindowTitle;
    progressDialog.MainAddTitle = title + UString(L" ");
    
    combiner.InputDirPrefix = srcPath;
    
    // CPanel::CDisableTimerProcessing disableTimerProcessing1(srcPanel);
    // CPanel::CDisableTimerProcessing disableTimerProcessing2(destPanel);
    
    if (combiner.Create(title, _window) != 0)
      return;
  }
  RefreshTitleAlways();

  // disableTimerProcessing1.Restore();
  // disableTimerProcessing2.Restore();
  // srcPanel.SetFocusToList();
  // srcPanel.RefreshListCtrlSaveFocused();
}
