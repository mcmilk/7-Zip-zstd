// PanelSplitFile.cpp

#include "StdAfx.h"

#include "resource.h"

extern "C" 
{ 
  #include "../../../../C/Alloc.h"
  #include "../../../../C/7zCrc.h"
}

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/FileIO.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/Thread.h"
#include "Windows/Error.h"

#include "ProgressDialog2.h"
#include "OverwriteDialogRes.h"

#include "App.h"
#include "FormatUtils.h"
#include "LangUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

static const UInt32 kBufSize = (1 << 15);

struct CDirEnumerator
{
  bool FlatMode;
  UString BasePrefix;
  UStringVector FileNames;

  CObjectVector<NFind::CEnumeratorW> Enumerators;
  UStringVector Prefixes;
  int Index;
  bool GetNextFile(NFind::CFileInfoW &fileInfo, bool &filled, UString &fullPath, DWORD &errorCode);
  void Init();
  
  CDirEnumerator(): FlatMode(false) {};
};

void CDirEnumerator::Init()
{
  Enumerators.Clear();
  Prefixes.Clear();
  Index = 0;
}

bool CDirEnumerator::GetNextFile(NFind::CFileInfoW &fileInfo, bool &filled, UString &resPath, DWORD &errorCode)
{
  filled = false;
  for (;;)
  {
    if (Enumerators.IsEmpty())
    {
      if (Index >= FileNames.Size())
        return true;
      const UString &path = FileNames[Index];
      int pos = path.ReverseFind('\\');
      resPath.Empty();
      if (pos >= 0)
        resPath = path.Left(pos + 1);
      if (!NFind::FindFile(BasePrefix + path, fileInfo))
      {
        errorCode = ::GetLastError();
        resPath = path;
        return false;
      }
      Index++;
      break;
    }
    bool found;
    if (!Enumerators.Back().Next(fileInfo, found))
    {
      errorCode = ::GetLastError();
      resPath = Prefixes.Back();
      return false;
    }
    if (found)
    {
      resPath = Prefixes.Back();
      break;
    }
    Enumerators.DeleteBack();
    Prefixes.DeleteBack();
  }
  resPath += fileInfo.Name;
  if (!FlatMode && fileInfo.IsDirectory())
  {
    UString prefix = resPath + (UString)(wchar_t)kDirDelimiter;
    Enumerators.Add(NFind::CEnumeratorW(BasePrefix + prefix + (UString)(wchar_t)kAnyStringWildcard));
    Prefixes.Add(prefix);
  }
  filled = true;
  return true;
}

struct CThreadCrc
{
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

  CProgressDialog *ProgressDialog;

  CDirEnumerator DirEnumerator;
  
  UInt64 NumFiles;
  UInt64 NumFolders;
  UInt64 DataSize;
  UInt32 DataCrcSum;
  UInt32 DataNameCrcSum;

  HRESULT Result;
  DWORD ErrorCode;
  UString ErrorPath;
  UString Error;
  bool ThereIsError;
  
  void Process2()
  {
    DataSize = NumFolders = NumFiles = DataCrcSum = DataNameCrcSum = 0;
    ProgressDialog->WaitCreating();

    CMyBuffer bufferObject;
    if (!bufferObject.Allocate(kBufSize))
    {
      Error = L"Can not allocate memory";
      ThereIsError = true;
      return;
    }
    Byte *buffer = (Byte *)(void *)bufferObject;

    UInt64 totalSize = 0;

    DirEnumerator.Init();

    UString scanningStr = LangString(IDS_SCANNING, 0x03020800);
    scanningStr += L" ";

    for (;;)
    {
      NFile::NFind::CFileInfoW fileInfo;
      bool filled;
      UString resPath;
      if (!DirEnumerator.GetNextFile(fileInfo, filled, resPath, ErrorCode))
      {
        ThereIsError = true;
        ErrorPath = resPath;
        return;
      }
      if (!filled)
        break;
      if (!fileInfo.IsDirectory())
        totalSize += fileInfo.Size;
      ProgressDialog->ProgressSynch.SetCurrentFileName(scanningStr + resPath);
      ProgressDialog->ProgressSynch.SetProgress(totalSize, 0);
      Result = ProgressDialog->ProgressSynch.SetPosAndCheckPaused(0);
      if (Result != S_OK)
        return;
    }

    ProgressDialog->ProgressSynch.SetProgress(totalSize, 0);

    DirEnumerator.Init();

    for (;;)
    {
      NFile::NFind::CFileInfoW fileInfo;
      bool filled;
      UString resPath;
      if (!DirEnumerator.GetNextFile(fileInfo, filled, resPath, ErrorCode))
      {
        ThereIsError = true;
        ErrorPath = resPath;
        return;
      }
      if (!filled)
        break;

      UInt32 crc = CRC_INIT_VAL;
      if (fileInfo.IsDirectory())
        NumFolders++;
      else
      {
        NFile::NIO::CInFile inFile;
        if (!inFile.Open(DirEnumerator.BasePrefix + resPath))
        {
          ErrorCode = ::GetLastError();
          ThereIsError = true;
          ErrorPath = resPath;
          return;
        }
        NumFiles++;
        ProgressDialog->ProgressSynch.SetCurrentFileName(resPath);
        for (;;)
        {
          UInt32 processedSize;
          if (!inFile.Read(buffer, kBufSize, processedSize))
          {
            ErrorCode = ::GetLastError();
            ThereIsError = true;
            ErrorPath = resPath;
            return;
          }
          if (processedSize == 0)
            break;
          crc = CrcUpdate(crc, buffer, processedSize);
          DataSize += processedSize;
          Result = ProgressDialog->ProgressSynch.SetPosAndCheckPaused(DataSize);
          if (Result != S_OK)
            return;
        }
        DataCrcSum += CRC_GET_DIGEST(crc);
      }
      for (int i = 0; i < resPath.Length(); i++)
      {
        wchar_t c = resPath[i];
        crc = CRC_UPDATE_BYTE(crc, ((Byte)(c & 0xFF)));
        crc = CRC_UPDATE_BYTE(crc, ((Byte)((c >> 8) & 0xFF)));
      }
      DataNameCrcSum += CRC_GET_DIGEST(crc);
      Result = ProgressDialog->ProgressSynch.SetPosAndCheckPaused(DataSize);
      if (Result != S_OK)
        return;
    }
  }
  DWORD Process()
  {
    try { Process2(); }
    catch(...) { Error = L"Error"; ThereIsError = true;}
    ProgressDialog->MyClose();
    return 0;
  }
  
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    return ((CThreadCrc *)param)->Process();
  }
};

static void ConvertUInt32ToHex(UInt32 value, wchar_t *s)
{
  for (int i = 0; i < 8; i++)
  {
    int t = value & 0xF;
    value >>= 4;
    s[7 - i] = (wchar_t)((t < 10) ? (L'0' + t) : (L'A' + (t - 10)));
  }
  s[8] = L'\0';
}

void CApp::CalculateCrc()
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

  CThreadCrc combiner;
  for (int i = 0; i < indices.Size(); i++)
    combiner.DirEnumerator.FileNames.Add(srcPanel.GetItemRelPath(indices[i]));
  combiner.DirEnumerator.BasePrefix = srcPanel._currentFolderPrefix;
  combiner.DirEnumerator.FlatMode = GetFlatMode();

  {
  CProgressDialog progressDialog;
  combiner.ProgressDialog = &progressDialog;
  combiner.ErrorCode = 0;
  combiner.Result = S_OK;
  combiner.ThereIsError = false;

  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
  UString title = LangString(IDS_CHECKSUM_CALCULATING, 0x03020710);

  progressDialog.MainWindow = _window;
  progressDialog.MainTitle = progressWindowTitle;
  progressDialog.MainAddTitle = title + UString(L" ");

  NWindows::CThread thread;
  if (thread.Create(CThreadCrc::MyThreadFunction, &combiner) != S_OK)
    return;
  progressDialog.Create(title, _window);

  if (combiner.Result != S_OK)
  {
    if (combiner.Result != E_ABORT)
      srcPanel.MessageBoxError(combiner.Result);
  }
  else if (combiner.ThereIsError)
  {
    if (combiner.Error.IsEmpty())
    {
      UString message = combiner.DirEnumerator.BasePrefix + combiner.ErrorPath;
      message += L"\n";
      message += NError::MyFormatMessageW(combiner.ErrorCode);
      srcPanel.MessageBoxMyError(message);
    }
    else
      srcPanel.MessageBoxMyError(combiner.Error);
  }
  else
  {
    UString s;
    {
      wchar_t sz[32];

      s += LangString(IDS_FILES_COLON, 0x02000320);
      s += L" ";
      ConvertUInt64ToString(combiner.NumFiles, sz);
      s += sz;
      s += L"\n";
      
      s += LangString(IDS_FOLDERS_COLON, 0x02000321);
      s += L" ";
      ConvertUInt64ToString(combiner.NumFolders, sz);
      s += sz;
      s += L"\n";

      s += LangString(IDS_SIZE_COLON, 0x02000322);
      s += L" ";
      ConvertUInt64ToString(combiner.DataSize, sz);
      s += MyFormatNew(IDS_FILE_SIZE, 0x02000982, sz);;
      s += L"\n";

      s += LangString(IDS_CHECKSUM_CRC_DATA, 0x03020721);
      s += L" ";
      ConvertUInt32ToHex(combiner.DataCrcSum, sz);
      s += sz;
      s += L"\n";

      s += LangString(IDS_CHECKSUM_CRC_DATA_NAMES, 0x03020722);
      s += L" ";
      ConvertUInt32ToHex(combiner.DataNameCrcSum, sz);
      s += sz;
    }
    srcPanel.MessageBoxInfo(s, LangString(IDS_CHECKSUM_INFORMATION, 0x03020720));
  }
  }
  RefreshTitleAlways();
}
