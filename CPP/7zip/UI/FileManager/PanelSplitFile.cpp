// PanelSplitFile.cpp

#include "StdAfx.h"

#include "../../../Common/IntToString.h"

#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/FileName.h"

#include "../../Archive/Common/MultiStream.h"

#include "../GUI/ExtractRes.h"

#include "resource.h"

#include "App.h"
#include "CopyDialog.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "SplitDialog.h"
#include "SplitUtils.h"

#include "PropertyNameRes.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

static const char * const g_Message_FileWriteError = "File write error";

struct CVolSeqName
{
  FString Extension;
  
  CVolSeqName(UInt64 numVolumes)
  {
    Extension = "00";
    do
    {
      Extension.Add_Char('0');
      numVolumes /= 10;
    }
    while (numVolumes > 99);
  }
  
  void PrepareNextName()
  {
    unsigned i = Extension.Len();
    do
    {
      const FChar c = Extension[--i];
      if (c != '9')
      {
        Extension.ReplaceOneCharAtPos(i, (FChar)(c + 1));
        return;
      }
      Extension.ReplaceOneCharAtPos(i, '0');
    }
    while (i);
    Extension.InsertAtFront((FChar)'1');
  }
};


class CThreadSplit: public CProgressThreadVirt
{
  HRESULT ProcessVirt() Z7_override;
public:
  FString FilePath;
  FString VolBasePath;
  UInt64 NumVolumes;
  CRecordVector<UInt64> VolumeSizes; // VolumeSizes.Size() > 0
};


class CPreAllocOutFile
{
  UInt64 _preAllocSize;
public:
  UInt64 Written;
  NIO::COutFile File;
  
  CPreAllocOutFile(): _preAllocSize(0), Written(0) {}
  
  ~CPreAllocOutFile()
  {
    SetCorrectFileLength();
  }

  void PreAlloc(UInt64 preAllocSize)
  {
    _preAllocSize = 0;
    if (File.SetLength(preAllocSize))
      _preAllocSize = preAllocSize;
    File.SeekToBegin();
  }

  bool Write(const void *data, UInt32 size, UInt32 &processedSize) throw()
  {
    bool res = File.Write(data, size, processedSize);
    Written += processedSize;
    return res;
  }

  void Close()
  {
    SetCorrectFileLength();
    _preAllocSize = 0;
    Written = 0;
    File.Close();
  }

  void SetCorrectFileLength()
  {
    if (Written < _preAllocSize)
    {
      File.SetLength(Written);
      _preAllocSize = 0;
    }
  }
};


static const UInt32 kBufSize = (1 << 20);

HRESULT CThreadSplit::ProcessVirt()
{
  NIO::CInFile inFile;
  if (!inFile.Open(FilePath))
    return GetLastError_noZero_HRESULT();

  CPreAllocOutFile outFile;
  
  CMyBuffer buffer;
  if (!buffer.Allocate(kBufSize))
    return E_OUTOFMEMORY;
  
  CVolSeqName seq(NumVolumes);
  
  UInt64 length;
  if (!inFile.GetLength(length))
    return GetLastError_noZero_HRESULT();
  
  CProgressSync &sync = Sync;
  sync.Set_NumFilesTotal(NumVolumes);
  sync.Set_NumBytesTotal(length);
  
  UInt64 pos = 0, prev = 0, numFiles = 0;
  unsigned volIndex = 0;

  for (;;)
  {
    const UInt64 volSize = VolumeSizes[volIndex];
    UInt32 needSize = kBufSize;
    {
      const UInt64 rem = volSize - outFile.Written;
      if (needSize > rem)
        needSize = (UInt32)rem;
    }
    {
      UInt32 processedSize = 0;
      if (!inFile.Read(buffer, needSize, processedSize))
        return GetLastError_noZero_HRESULT();
      if (processedSize == 0)
        break;
      needSize = processedSize;
    }
  
    if (outFile.Written == 0)
    {
      FString name = VolBasePath;
      name.Add_Dot();
      seq.PrepareNextName();
      name += seq.Extension;
      sync.Set_FilePath(fs2us(name));
      if (!outFile.File.Create_NEW(name))
      {
        const HRESULT res = GetLastError_noZero_HRESULT();
        AddErrorPath(name);
        return res;
      }
      UInt64 expectSize = volSize;
      if (pos < length)
      {
        const UInt64 rem = length - pos;
        if (expectSize > rem)
          expectSize = rem;
      }
      outFile.PreAlloc(expectSize);
    }

    {
      UInt32 processedSize = 0;
      if (!outFile.Write(buffer, needSize, processedSize))
        return GetLastError_noZero_HRESULT();
      if (needSize != processedSize)
        throw g_Message_FileWriteError;
      pos += processedSize;
    }
    
    if (outFile.Written == volSize)
    {
      outFile.Close();
      sync.Set_NumFilesCur(++numFiles);
      if (volIndex + 1 < VolumeSizes.Size())
        volIndex++;
    }

    if (pos - prev >= ((UInt32)1 << 22) || outFile.Written == 0)
    {
      prev = pos;
      RINOK(sync.Set_NumBytesCur(pos))
    }
  }

  return S_OK;
}


// volSizes.Size() > 0
inline UInt64 GetNumberOfVolumes(UInt64 size, const CRecordVector<UInt64> &volSizes)
{
  const unsigned numVols = volSizes.Size();
  if (numVols == 0) // optional check
    return 1;
  const UInt64 *volSizesPtr = volSizes.ConstData();
  UInt64 volSize;
  unsigned i = 0;
  do
  {
    volSize = volSizesPtr[i++];
    if (volSize >= size)
      return i;
    size -= volSize;
  }
  while (i < numVols);
  if (volSize == 0)
    return (UInt64)(Int64)-1;
  return numVols + (size - 1) / volSize + 1;
}

void CApp::Split()
{
  const unsigned srcPanelIndex = GetFocusedPanelIndex();
  CPanel &srcPanel = Panels[srcPanelIndex];
  if (!srcPanel.Is_IO_FS_Folder())
  {
    srcPanel.MessageBox_Error_UnsupportOperation();
    return;
  }
  CRecordVector<UInt32> indices;
  srcPanel.Get_ItemIndices_Operated(indices);
  if (indices.IsEmpty())
    return;
  if (indices.Size() != 1)
  {
    srcPanel.MessageBox_Error_LangID(IDS_SELECT_ONE_FILE);
    return;
  }
  const unsigned index = indices[0];
  if (srcPanel.IsItem_Folder(index))
  {
    srcPanel.MessageBox_Error_LangID(IDS_SELECT_ONE_FILE);
    return;
  }
  const UString itemName = srcPanel.GetItemName(index);

  const UString srcPath = srcPanel.GetFsPath() + srcPanel.GetItemPrefix(index);
  UString path = srcPath;
  unsigned destPanelIndex = (NumPanels <= 1) ? srcPanelIndex : (1 - srcPanelIndex);
  CPanel &destPanel = Panels[destPanelIndex];
  if (NumPanels > 1)
    if (destPanel.IsFSFolder())
      path = destPanel.GetFsPath();
  CSplitDialog splitDialog;
  splitDialog.FilePath = srcPanel.GetItemRelPath(index);
  splitDialog.Path = path;
  if (splitDialog.Create(srcPanel.GetParent()) != IDOK)
    return;

  NFind::CFileInfo fileInfo;
  if (!fileInfo.Find(us2fs(srcPath + itemName)))
  {
    srcPanel.MessageBox_Error(L"Cannot find file");
    return;
  }
  const UInt64 numVolumes = GetNumberOfVolumes(fileInfo.Size, splitDialog.VolumeSizes);
  if (numVolumes <= 1)
  {
    srcPanel.MessageBox_Error_LangID(IDS_SPLIT_VOL_MUST_BE_SMALLER);
    return;
  }
  if (numVolumes >= 100)
  {
    wchar_t s[32];
    ConvertUInt64ToString(numVolumes, s);
    if (::MessageBoxW(srcPanel, MyFormatNew(IDS_SPLIT_CONFIRM_MESSAGE, s),
        LangString(IDS_SPLIT_CONFIRM_TITLE),
        MB_YESNOCANCEL | MB_ICONQUESTION) != IDYES)
      return;
    if (numVolumes >= 1u << 29)
    {
      srcPanel.MessageBox_Error(L"too big number of parts");
      return;
    }
  }

  path = splitDialog.Path;
  NName::NormalizeDirPathPrefix(path);
  if (!CreateComplexDir(us2fs(path)))
  {
    const HRESULT lastError = GetLastError_noZero_HRESULT();
    srcPanel.MessageBox_Error_2Lines_Message_HRESULT(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, path), lastError);
    return;
  }

  {
  CThreadSplit spliter;
  spliter.NumVolumes = numVolumes;

  CProgressDialog &progressDialog = spliter;

  const UString progressWindowTitle ("7-Zip"); // LangString(IDS_APP_TITLE, 0x03000000);
  const UString title = LangString(IDS_SPLITTING);

  progressDialog.ShowCompressionInfo = false;

  progressDialog.MainWindow = _window;
  progressDialog.MainTitle = progressWindowTitle;
  progressDialog.MainAddTitle = title;
  progressDialog.MainAddTitle.Add_Space();
  progressDialog.Sync.Set_TitleFileName(itemName);


  spliter.FilePath = us2fs(srcPath + itemName);
  spliter.VolBasePath = us2fs(path + srcPanel.GetItemName_for_Copy(index));
  spliter.VolumeSizes = splitDialog.VolumeSizes;
  
  // if (splitDialog.VolumeSizes.Size() == 0) return;

  // CPanel::CDisableTimerProcessing disableTimerProcessing1(srcPanel);
  // CPanel::CDisableTimerProcessing disableTimerProcessing2(destPanel);

  if (spliter.Create(title, _window) != 0)
    return;
  }
  RefreshTitleAlways();


  // disableNotify.Restore();
  // disableNotify.Restore();
  // srcPanel.SetFocusToList();
  // srcPanel.RefreshListCtrlSaveFocused();
}


class CThreadCombine: public CProgressThreadVirt
{
  HRESULT ProcessVirt() Z7_override;
public:
  FString InputDirPrefix;
  FStringVector Names;
  FString OutputPath;
  UInt64 TotalSize;
};

HRESULT CThreadCombine::ProcessVirt()
{
  NIO::COutFile outFile;
  if (!outFile.Create_NEW(OutputPath))
  {
    const HRESULT res = GetLastError_noZero_HRESULT();
    AddErrorPath(OutputPath);
    return res;
  }
  
  CProgressSync &sync = Sync;
  sync.Set_NumBytesTotal(TotalSize);
  sync.Set_NumFilesTotal(Names.Size());
  
  CMyBuffer bufferObject;
  if (!bufferObject.Allocate(kBufSize))
    return E_OUTOFMEMORY;
  Byte *buffer = (Byte *)(void *)bufferObject;
  UInt64 pos = 0;
  FOR_VECTOR (i, Names)
  {
    NIO::CInFile inFile;
    const FString nextName = InputDirPrefix + Names[i];
    sync.Set_NumFilesCur(i);
    sync.Set_FilePath(fs2us(nextName));
    if (!inFile.Open(nextName))
    {
      const HRESULT res = GetLastError_noZero_HRESULT();
      AddErrorPath(nextName);
      return res;
    }
    for (;;)
    {
      UInt32 processedSize;
      if (!inFile.Read(buffer, kBufSize, processedSize))
      {
        const HRESULT res = GetLastError_noZero_HRESULT();
        AddErrorPath(nextName);
        return res;
      }
      if (processedSize == 0)
        break;
      const UInt32 needSize = processedSize;
      if (!outFile.Write(buffer, needSize, processedSize))
      {
        const HRESULT res = GetLastError_noZero_HRESULT();
        AddErrorPath(OutputPath);
        return res;
      }
      if (needSize != processedSize)
        throw g_Message_FileWriteError;
      pos += processedSize;
      RINOK(sync.Set_NumBytesCur(pos))
    }
  }
  return S_OK;
}

extern void AddValuePair2(UString &s, UINT resourceID, UInt64 num, UInt64 size);

static void AddInfoFileName(UString &dest, const UString &name)
{
  dest += "\n  ";
  dest += name;
}

void CApp::Combine()
{
  const unsigned srcPanelIndex = GetFocusedPanelIndex();
  const CPanel &srcPanel = Panels[srcPanelIndex];
  if (!srcPanel.IsFSFolder())
  {
    srcPanel.MessageBox_Error_LangID(IDS_OPERATION_IS_NOT_SUPPORTED);
    return;
  }
  CRecordVector<UInt32> indices;
  srcPanel.Get_ItemIndices_Operated(indices);
  if (indices.IsEmpty())
    return;
  const unsigned index = indices[0];
  if (indices.Size() != 1 || srcPanel.IsItem_Folder(index))
  {
    srcPanel.MessageBox_Error_LangID(IDS_COMBINE_SELECT_ONE_FILE);
    return;
  }

  const UString srcPath = srcPanel.GetFsPath() + srcPanel.GetItemPrefix(index);
  UString path = srcPath;
  const unsigned destPanelIndex = (NumPanels <= 1) ? srcPanelIndex : (1 - srcPanelIndex);
  const CPanel &destPanel = Panels[destPanelIndex];
  if (NumPanels > 1 && destPanel.IsFSFolder())
    path = destPanel.GetFsPath();

  CSplitVolName seq;
  seq.Name = srcPanel.GetItemName(index);
  if (!seq.ParseName())
  {
    srcPanel.MessageBox_Error_LangID(IDS_COMBINE_CANT_DETECT_SPLIT_FILE);
    return;
  }
  UString subName;
  seq.Generate_SubName(subName);
  
  {
  CThreadCombine combiner;
  
  combiner.TotalSize = 0;

  UString curPath = srcPath;
  for (;;)
  {
    seq.GenerateName();
    NFind::CFileInfo fileInfo;
    curPath.DeleteFrom(srcPath.Len());
    curPath += seq.Name;
    if (!fileInfo.Find(us2fs(curPath)) || fileInfo.IsDir())
    {
      if (!seq.IsComplicated)
        break;
      seq.AddPostfix_2Chars();
      seq.GenerateName();
      curPath.DeleteFrom(srcPath.Len());
      curPath += seq.Name;
      if (!fileInfo.Find(us2fs(curPath)) || fileInfo.IsDir())
        break;
    }
    combiner.Names.Add(us2fs(seq.Name));
    combiner.TotalSize += fileInfo.Size;
    if (!seq.GetNextName())
      break;
  }
  if (combiner.Names.Size() == 1)
  {
    srcPanel.MessageBox_Error_LangID(IDS_COMBINE_CANT_FIND_MORE_THAN_ONE_PART);
    return;
  }
  
  if (combiner.TotalSize == 0)
  {
    srcPanel.MessageBox_Error(L"No data");
    return;
  }
  
  UString info;
  info += subName;
  info.Add_LF();
  info.Add_LF();
  AddValuePair2(info, IDS_PROP_FILES, combiner.Names.Size(), combiner.TotalSize);
  info.Add_LF();
  info += srcPath;
  
  unsigned i;
  for (i = 0; i < combiner.Names.Size() && i < 2; i++)
    AddInfoFileName(info, fs2us(combiner.Names[i]));
  if (i != combiner.Names.Size())
  {
    if (i + 1 != combiner.Names.Size())
      AddInfoFileName(info, L"...");
    AddInfoFileName(info, fs2us(combiner.Names.Back()));
  }
  
  {
    CCopyDialog copyDialog;
    copyDialog.Value = path;
    LangString(IDS_COMBINE, copyDialog.Title);
    copyDialog.Title.Add_Space();
    copyDialog.Title += srcPanel.GetItemRelPath(index);
    LangString(IDS_COMBINE_TO, copyDialog.Static);
    copyDialog.Info = info;
    if (copyDialog.Create(srcPanel.GetParent()) != IDOK)
      return;
    path = copyDialog.Value;
  }

  NName::NormalizeDirPathPrefix(path);
  if (!CreateComplexDir(us2fs(path)))
  {
    const HRESULT lastError = GetLastError_noZero_HRESULT();
    srcPanel.MessageBox_Error_2Lines_Message_HRESULT(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, path), lastError);
    return;
  }
  
  NFind::CFileInfo fileInfo;
  const UString destFilePath = path + subName;
  combiner.OutputPath = us2fs(destFilePath);
  if (fileInfo.Find(combiner.OutputPath))
  {
    srcPanel.MessageBox_Error(MyFormatNew(IDS_FILE_EXIST, destFilePath));
    return;
  }
  
    CProgressDialog &progressDialog = combiner;
    progressDialog.ShowCompressionInfo = false;
  
    const UString progressWindowTitle ("7-Zip"); // LangString(IDS_APP_TITLE, 0x03000000);
    const UString title = LangString(IDS_COMBINING);
    
    progressDialog.MainWindow = _window;
    progressDialog.MainTitle = progressWindowTitle;
    progressDialog.MainAddTitle = title;
    progressDialog.MainAddTitle.Add_Space();
    
    combiner.InputDirPrefix = us2fs(srcPath);
    
    // CPanel::CDisableTimerProcessing disableTimerProcessing1(srcPanel);
    // CPanel::CDisableTimerProcessing disableTimerProcessing2(destPanel);
    
    if (combiner.Create(title, _window) != 0)
      return;
  }
  RefreshTitleAlways();

  // disableNotify.Restore();
  // disableNotify.Restore();
  // srcPanel.SetFocusToList();
  // srcPanel.RefreshListCtrlSaveFocused();
}
