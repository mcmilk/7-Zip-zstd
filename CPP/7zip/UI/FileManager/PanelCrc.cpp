// PanelSplitFile.cpp

#include "StdAfx.h"

#include "../../../../C/7zCrc.h"
#include "../../../../C/Sha256.h"

#include "Common/IntToString.h"

#include "Windows/FileFind.h"
#include "Windows/FileIO.h"
#include "Windows/FileName.h"

#include "OverwriteDialogRes.h"

#include "App.h"
#include "FormatUtils.h"
#include "LangUtils.h"

#include "../Common/PropIDUtils.h"

#include "resource.h"

using namespace NWindows;
using namespace NFile;

static const UInt32 kBufSize = (1 << 15);

struct CDirEnumerator
{
  bool FlatMode;
  UString BasePrefix;
  UStringVector FileNames;

  CObjectVector<NFind::CEnumeratorW> Enumerators;
  UStringVector Prefixes;
  int Index;
  HRESULT GetNextFile(NFind::CFileInfoW &fileInfo, bool &filled, UString &fullPath);
  void Init();
  
  CDirEnumerator(): FlatMode(false) {};
};

void CDirEnumerator::Init()
{
  Enumerators.Clear();
  Prefixes.Clear();
  Index = 0;
}

static HRESULT GetNormalizedError()
{
  HRESULT errorCode = GetLastError();
  return (errorCode == 0) ? E_FAIL : errorCode;
}

HRESULT CDirEnumerator::GetNextFile(NFind::CFileInfoW &fileInfo, bool &filled, UString &resPath)
{
  filled = false;
  for (;;)
  {
    if (Enumerators.IsEmpty())
    {
      if (Index >= FileNames.Size())
        return S_OK;
      const UString &path = FileNames[Index];
      int pos = path.ReverseFind(WCHAR_PATH_SEPARATOR);
      resPath.Empty();
      if (pos >= 0)
        resPath = path.Left(pos + 1);

      #ifdef _WIN32
      // it's for "c:" paths/
      if (BasePrefix.IsEmpty() && path.Length() == 2 && path[1] == ':')
      {
        fileInfo.Name = path;
        fileInfo.Attrib = FILE_ATTRIBUTE_DIRECTORY;
        fileInfo.Size = 0;
      }
      else
      #endif
      if (!fileInfo.Find(BasePrefix + path))
      {
        WRes errorCode = GetNormalizedError();
        resPath = path;
        return errorCode;
      }
      Index++;
      break;
    }
    bool found;
    if (!Enumerators.Back().Next(fileInfo, found))
    {
      HRESULT errorCode = GetNormalizedError();
      resPath = Prefixes.Back();
      return errorCode;
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
  if (!FlatMode && fileInfo.IsDir())
  {
    UString prefix = resPath + WCHAR_PATH_SEPARATOR;
    Enumerators.Add(NFind::CEnumeratorW(BasePrefix + prefix + (UString)(wchar_t)NName::kAnyStringWildcard));
    Prefixes.Add(prefix);
  }
  filled = true;
  return S_OK;
}

static void ConvertByteToHex(unsigned value, wchar_t *s)
{
  for (int i = 0; i < 2; i++)
  {
    unsigned t = value & 0xF;
    value >>= 4;
    s[1 - i] = (wchar_t)((t < 10) ? (L'0' + t) : (L'A' + (t - 10)));
  }
}

class CThreadCrc: public CProgressThreadVirt
{
  UInt64 NumFilesScan;
  UInt64 NumFiles;
  UInt64 NumFolders;
  UInt64 DataSize;
  UInt32 DataCrcSum;
  Byte Sha256Sum[SHA256_DIGEST_SIZE];
  UInt32 DataNameCrcSum;

  UString GetResultMessage() const;
  HRESULT ProcessVirt();
public:
  CDirEnumerator Enumerator;
 
};

UString CThreadCrc::GetResultMessage() const
{
  UString s;
  wchar_t sz[32];
  
  s += LangString(IDS_FILES_COLON, 0x02000320);
  s += L' ';
  ConvertUInt64ToString(NumFiles, sz);
  s += sz;
  s += L'\n';
  
  s += LangString(IDS_FOLDERS_COLON, 0x02000321);
  s += L' ';
  ConvertUInt64ToString(NumFolders, sz);
  s += sz;
  s += L'\n';
  
  s += LangString(IDS_SIZE_COLON, 0x02000322);
  s += L' ';
  ConvertUInt64ToString(DataSize, sz);
  s += MyFormatNew(IDS_FILE_SIZE, 0x02000982, sz);
  s += L'\n';
  
  s += LangString(IDS_CHECKSUM_CRC_DATA, 0x03020721);
  s += L' ';
  ConvertUInt32ToHex(DataCrcSum, sz);
  s += sz;
  s += L'\n';
  
  s += LangString(IDS_CHECKSUM_CRC_DATA_NAMES, 0x03020722);
  s += L' ';
  ConvertUInt32ToHex(DataNameCrcSum, sz);
  s += sz;
  s += L'\n';
  
  if (NumFiles == 1 && NumFilesScan == 1)
  {
    s += L"SHA-256: ";
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++)
    {
      wchar_t s2[4];
      ConvertByteToHex(Sha256Sum[i], s2);
      s2[2] = 0;
      s += s2;
    }
  }
  return s;
}

HRESULT CThreadCrc::ProcessVirt()
{
  DataSize = NumFolders = NumFiles = NumFilesScan = DataCrcSum = DataNameCrcSum = 0;
  memset(Sha256Sum, 0, SHA256_DIGEST_SIZE);
  // ProgressDialog.WaitCreating();
  
  CMyBuffer bufferObject;
  if (!bufferObject.Allocate(kBufSize))
    return E_OUTOFMEMORY;
  Byte *buffer = (Byte *)(void *)bufferObject;
  
  UInt64 totalSize = 0;
  
  Enumerator.Init();
  
  UString scanningStr = LangString(IDS_SCANNING, 0x03020800);
  scanningStr += L' ';
  
  CProgressSync &sync = ProgressDialog.Sync;

  for (;;)
  {
    NFind::CFileInfoW fileInfo;
    bool filled;
    UString resPath;
    HRESULT errorCode = Enumerator.GetNextFile(fileInfo, filled, resPath);
    if (errorCode != 0)
    {
      ErrorPath1 = resPath;
      return errorCode;
    }
    if (!filled)
      break;
    if (!fileInfo.IsDir())
    {
      totalSize += fileInfo.Size;
      NumFilesScan++;
    }
    sync.SetCurrentFileName(scanningStr + resPath);
    sync.SetProgress(totalSize, 0);
    RINOK(sync.SetPosAndCheckPaused(0));
  }
  sync.SetNumFilesTotal(NumFilesScan);
  sync.SetProgress(totalSize, 0);
  
  Enumerator.Init();
  
  for (;;)
  {
    NFind::CFileInfoW fileInfo;
    bool filled;
    UString resPath;
    HRESULT errorCode = Enumerator.GetNextFile(fileInfo, filled, resPath);
    if (errorCode != 0)
    {
      ErrorPath1 = resPath;
      return errorCode;
    }
    if (!filled)
      break;
    
    UInt32 crc = CRC_INIT_VAL;
    CSha256 sha256;
    Sha256_Init(&sha256);
    
    if (fileInfo.IsDir())
      NumFolders++;
    else
    {
      NIO::CInFile inFile;
      if (!inFile.Open(Enumerator.BasePrefix + resPath))
      {
        errorCode = GetNormalizedError();
        ErrorPath1 = resPath;
        return errorCode;
      }
      sync.SetCurrentFileName(resPath);
      sync.SetNumFilesCur(NumFiles);
      NumFiles++;
      for (;;)
      {
        UInt32 processedSize;
        if (!inFile.Read(buffer, kBufSize, processedSize))
        {
          errorCode = GetNormalizedError();
          ErrorPath1 = resPath;
          return errorCode;
        }
        if (processedSize == 0)
          break;
        crc = CrcUpdate(crc, buffer, processedSize);
        if (NumFilesScan == 1)
          Sha256_Update(&sha256, buffer, processedSize);
        
        DataSize += processedSize;
        RINOK(sync.SetPosAndCheckPaused(DataSize));
      }
      DataCrcSum += CRC_GET_DIGEST(crc);
      if (NumFilesScan == 1)
        Sha256_Final(&sha256, Sha256Sum);
    }
    for (int i = 0; i < resPath.Length(); i++)
    {
      wchar_t c = resPath[i];
      crc = CRC_UPDATE_BYTE(crc, ((Byte)(c & 0xFF)));
      crc = CRC_UPDATE_BYTE(crc, ((Byte)((c >> 8) & 0xFF)));
    }
    DataNameCrcSum += CRC_GET_DIGEST(crc);
    RINOK(sync.SetPosAndCheckPaused(DataSize));
  }
  sync.SetNumFilesCur(NumFiles);
  OkMessage = GetResultMessage();
  OkMessageTitle = LangString(IDS_CHECKSUM_INFORMATION, 0x03020720);
  return S_OK;
}

void CApp::CalculateCrc()
{
  int srcPanelIndex = GetFocusedPanelIndex();
  CPanel &srcPanel = Panels[srcPanelIndex];
  if (!srcPanel.IsFsOrDrivesFolder())
  {
    srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return;
  }
  CRecordVector<UInt32> indices;
  srcPanel.GetOperatedItemIndices(indices);
  if (indices.IsEmpty())
    return;

  {
  CThreadCrc t;
  for (int i = 0; i < indices.Size(); i++)
    t.Enumerator.FileNames.Add(srcPanel.GetItemRelPath(indices[i]));
  t.Enumerator.BasePrefix = srcPanel.GetFsPath();
  t.Enumerator.FlatMode = GetFlatMode();

  t.ProgressDialog.ShowCompressionInfo = false;

  UString title = LangString(IDS_CHECKSUM_CALCULATING, 0x03020710);

  t.ProgressDialog.MainWindow = _window;
  t.ProgressDialog.MainTitle = LangString(IDS_APP_TITLE, 0x03000000);
  t.ProgressDialog.MainAddTitle = title + UString(L' ');

  if (t.Create(title, _window) != S_OK)
    return;
  }
  RefreshTitleAlways();
}
