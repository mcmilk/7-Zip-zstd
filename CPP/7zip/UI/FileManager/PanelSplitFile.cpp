// PanelSplitFile.cpp

#include "StdAfx.h"

#include "resource.h"

extern "C" 
{ 
  #include "../../../../C/Alloc.h"
}

#include "Common/Types.h"
#include "Common/IntToString.h"

// #include "Windows/COM.h"
#include "Windows/FileIO.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"
#include "ProgressDialog2.h"
#include "SplitDialog.h"
#include "CopyDialog.h"

#include "../GUI/ExtractRes.h"

#include "SplitUtils.h"
#include "App.h"
#include "FormatUtils.h"
#include "LangUtils.h"

using namespace NWindows;

class CMyBuffer
{
  void *_data;
public:
  CMyBuffer(): _data(0) {}
  operator void *() { return _data; }
  bool Allocate(size_t size)
  {
    if (_data != 0)
      return false;
    _data = ::MidAlloc(size);
    return _data != 0;
  }
  ~CMyBuffer() { ::MidFree(_data); }
};

struct CVolSeqName
{
  UString UnchangedPart;
  UString ChangedPart;    
  CVolSeqName(): ChangedPart(L"000") {};

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

struct CThreadSplit
{
  // HRESULT Result;
  // CPanel *Panel;
  CProgressDialog *ProgressDialog;
  UString FilePath;
  UString VolBasePath;
  CRecordVector<UInt64> VolumeSizes;
  UString Error;
  
  void Process2()
  {
    // NCOM::CComInitializer comInitializer;
    ProgressDialog->WaitCreating();
    NFile::NIO::CInFile inFile;
    if (!inFile.Open(FilePath))
      throw L"Can not open file";
    NFile::NIO::COutFile outFile;
    CMyBuffer bufferObject;
    if (!bufferObject.Allocate(kBufSize))
      throw L"Can not allocate buffer";
    Byte *buffer = (Byte *)(void *)bufferObject;
    UInt64 curVolSize = 0;
    CVolSeqName seqName;
    UInt64 length;
    if (!inFile.GetLength(length))
      throw "error";

    ProgressDialog->ProgressSynch.SetProgress(length, 0);
    UInt64 pos = 0;

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
        throw L"Can not read input file";
      if (processedSize == 0)
        break;
      needSize = processedSize;
      if (curVolSize == 0)
      {
        UString name = VolBasePath;
        name += L".";
        name += seqName.GetNextName();
        if (!outFile.Create(name, false))
          throw L"Can not create output file";
        ProgressDialog->ProgressSynch.SetCurrentFileName(name);
      }
      if (!outFile.Write(buffer, needSize, processedSize))
        throw L"Can not write output file";
      if (needSize != processedSize)
        throw L"Can not write output file";
      curVolSize += processedSize;
      if (curVolSize == volSize)
      {
        outFile.Close();
        if (volIndex < VolumeSizes.Size())
          volIndex++;
        curVolSize = 0;
      }
      pos += processedSize;
      HRESULT res = ProgressDialog->ProgressSynch.SetPosAndCheckPaused(pos);
      if (res != S_OK)
        return;
    }
  }
  DWORD Process()
  {
    try { Process2(); }
    catch(const wchar_t *s) { Error = s; }
    catch(...) { Error = L"Error"; }
    ProgressDialog->MyClose();
    return 0;
  }
  
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    return ((CThreadSplit *)param)->Process();
  }
};

void CApp::Split()
{
  int srcPanelIndex = GetFocusedPanelIndex();
  CPanel &srcPanel = Panels[srcPanelIndex];
  if (!srcPanel.IsFSFolder())
  {
    srcPanel.MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }
  CRecordVector<UInt32> indices;
  srcPanel.GetOperatedItemIndices(indices);
  if (indices.IsEmpty())
    return;
  if (indices.Size() != 1)
  {
    srcPanel.MessageBox(L"Select one file");
    return;
  }
  int index = indices[0];
  if (srcPanel.IsItemFolder(index))
  {
    srcPanel.MessageBox(L"Select one file");
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
  if (!NFile::NFind::FindFile(srcPath + itemName, fileInfo))
  {
    srcPanel.MessageBoxMyError(L"Can not find file");
    return;
  }
  if (fileInfo.Size <= splitDialog.VolumeSizes.Front())
  {
    srcPanel.MessageBoxMyError(LangString(IDS_SPLIT_VOL_MUST_BE_SMALLER, 0x03020522));
    return;
  }
  const UInt64 numVolumes = GetNumberOfVolumes(fileInfo.Size, splitDialog.VolumeSizes);
  if (numVolumes >= 100)
  {
    wchar_t s[32];
    ConvertUInt64ToString(numVolumes, s);
    if (::MessageBoxW(srcPanel, MyFormatNew(IDS_SPLIT_CONFIRM_MESSAGE, 0x03020521, s), 
        LangString(IDS_SPLIT_CONFIRM_TITLE, 0x03020520), 
        MB_YESNOCANCEL | MB_ICONQUESTION | MB_TASKMODAL) != IDYES)
      return;
  }

  path = splitDialog.Path;
  NFile::NName::NormalizeDirPathPrefix(path);
  if (!NFile::NDirectory::CreateComplexDirectory(path))
  {
    srcPanel.MessageBoxMyError(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 0x02000603, path));
    return;
  }

  CThreadSplit spliter;
  // spliter.Panel = this;

  {
  CProgressDialog progressDialog;
  spliter.ProgressDialog = &progressDialog;

  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
  UString title = LangString(IDS_SPLITTING, 0x03020510);

  progressDialog.MainWindow = _window;
  progressDialog.MainTitle = progressWindowTitle;
  progressDialog.MainAddTitle = title + UString(L" ");
  progressDialog.ProgressSynch.SetTitleFileName(itemName);


  spliter.FilePath = srcPath + itemName;
  spliter.VolBasePath = path  + itemName;
  spliter.VolumeSizes = splitDialog.VolumeSizes;
  
  // if (splitDialog.VolumeSizes.Size() == 0) return;

  // CPanel::CDisableTimerProcessing disableTimerProcessing1(srcPanel);
  // CPanel::CDisableTimerProcessing disableTimerProcessing2(destPanel);

  NWindows::CThread thread;
  if (thread.Create(CThreadSplit::MyThreadFunction, &spliter) != S_OK)
    throw 271824;
  progressDialog.Create(title, _window);
  }
  RefreshTitleAlways();


  if (!spliter.Error.IsEmpty())
    srcPanel.MessageBoxMyError(spliter.Error);
  // disableTimerProcessing1.Restore();
  // disableTimerProcessing2.Restore();
  // srcPanel.SetFocusToList();
  // srcPanel.RefreshListCtrlSaveFocused();
}


struct CThreadCombine
{
  CProgressDialog *ProgressDialog;
  UString InputDirPrefix;
  UString FirstVolumeName;
  UString OutputDirPrefix;
  UString Error;
  
  void Process2()
  {
    // NCOM::CComInitializer comInitializer;
    ProgressDialog->WaitCreating();

    CVolSeqName volSeqName;
    if (!volSeqName.ParseName(FirstVolumeName))
      throw L"Can not detect file as splitted file";

    UString nextName = InputDirPrefix + FirstVolumeName;
    UInt64 totalSize = 0;
    for (;;)
    {
      NFile::NFind::CFileInfoW fileInfo;
      if (!NFile::NFind::FindFile(nextName, fileInfo))
        break;
      if (fileInfo.IsDirectory())
        break;
      totalSize += fileInfo.Size;
      nextName = InputDirPrefix + volSeqName.GetNextName();
    }
    if (totalSize == 0)
      throw L"no data";
    ProgressDialog->ProgressSynch.SetProgress(totalSize, 0);

    if (!volSeqName.ParseName(FirstVolumeName))
      throw L"Can not detect file as splitted file";

    UString outName = volSeqName.UnchangedPart;
    while(!outName.IsEmpty())
    {
      int lastIndex = outName.Length() - 1;
      if (outName[lastIndex] != L'.')
        break;
      outName.Delete(lastIndex);
    }
    if (outName.IsEmpty())
      outName = L"file";
    NFile::NIO::COutFile outFile;
    if (!outFile.Create(OutputDirPrefix + outName, false))
      throw L"Can create open output file";

    NFile::NIO::CInFile inFile;
    CMyBuffer bufferObject;
    if (!bufferObject.Allocate(kBufSize))
      throw L"Can not allocate buffer";
    Byte *buffer = (Byte *)(void *)bufferObject;
    UInt64 pos = 0;
    nextName = InputDirPrefix + FirstVolumeName;
    bool needOpen = true;
    for (;;)
    {
      if (needOpen)
      {
        NFile::NFind::CFileInfoW fileInfo;
        if (!NFile::NFind::FindFile(nextName, fileInfo))
          break;
        if (fileInfo.IsDirectory())
          break;
        if (!inFile.Open(nextName))
          throw L"Can not open file";
        ProgressDialog->ProgressSynch.SetCurrentFileName(fileInfo.Name);
        nextName = InputDirPrefix + volSeqName.GetNextName();
        needOpen = false;
      }
      UInt32 processedSize;
      if (!inFile.Read(buffer, kBufSize, processedSize))
        throw L"Can not read input file";
      if (processedSize == 0)
      {
        needOpen = true;
        continue;
      }
      UInt32 needSize = processedSize;
      if (!outFile.Write(buffer, needSize, processedSize))
        throw L"Can not write output file";
      if (needSize != processedSize)
        throw L"Can not write output file";
      pos += processedSize;
      HRESULT res = ProgressDialog->ProgressSynch.SetPosAndCheckPaused(pos);
      if (res != S_OK)
        return;
    }
  }
  DWORD Process()
  {
    try { Process2(); }
    catch(const wchar_t *s) { Error = s; }
    catch(...) { Error = L"Error";}
    ProgressDialog->MyClose();
    return 0;
  }
  
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    return ((CThreadCombine *)param)->Process();
  }
};

void CApp::Combine()
{
  int srcPanelIndex = GetFocusedPanelIndex();
  CPanel &srcPanel = Panels[srcPanelIndex];
  if (!srcPanel.IsFSFolder())
  {
    srcPanel.MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }
  CRecordVector<UInt32> indices;
  srcPanel.GetOperatedItemIndices(indices);
  if (indices.IsEmpty())
    return;
  int index = indices[0];
  if (indices.Size() != 1 || srcPanel.IsItemFolder(index))
  {
    srcPanel.MessageBox(LangString(IDS_COMBINE_SELECT_ONE_FILE, 0x03020620));
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
  CCopyDialog copyDialog;
  copyDialog.Value = path;
  copyDialog.Title = LangString(IDS_COMBINE, 0x03020600);
  copyDialog.Title += ' ';
  copyDialog.Title += srcPanel.GetItemRelPath(index);

  copyDialog.Static = LangString(IDS_COMBINE_TO, 0x03020601);;
  if (copyDialog.Create(srcPanel.GetParent()) == IDCANCEL)
    return;

  CThreadCombine combiner;
  // combiner.Panel = this;

  {
  CProgressDialog progressDialog;
  combiner.ProgressDialog = &progressDialog;

  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
  UString title = LangString(IDS_COMBINING, 0x03020610);

  progressDialog.MainWindow = _window;
  progressDialog.MainTitle = progressWindowTitle;
  progressDialog.MainAddTitle = title + UString(L" ");

  path = copyDialog.Value;
  NFile::NName::NormalizeDirPathPrefix(path);
  if (!NFile::NDirectory::CreateComplexDirectory(path))
  {
    srcPanel.MessageBoxMyError(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 0x02000603, path));
    return;
  }

  combiner.InputDirPrefix = srcPath;
  combiner.FirstVolumeName = itemName;
  combiner.OutputDirPrefix = path;

  // CPanel::CDisableTimerProcessing disableTimerProcessing1(srcPanel);
  // CPanel::CDisableTimerProcessing disableTimerProcessing2(destPanel);

  NWindows::CThread thread;
  if (thread.Create(CThreadCombine::MyThreadFunction, &combiner) != S_OK)
    throw 271824;
  progressDialog.Create(title, _window);
  }
  RefreshTitleAlways();

  if (!combiner.Error.IsEmpty())
    srcPanel.MessageBoxMyError(combiner.Error);
  // disableTimerProcessing1.Restore();
  // disableTimerProcessing2.Restore();
  // srcPanel.SetFocusToList();
  // srcPanel.RefreshListCtrlSaveFocused();
}
