// PanelCrc.cpp

#include "StdAfx.h"

#include "../../../Common/MyException.h"

#include "../../../Windows/FileFind.h"
#include "../../../Windows/FileIO.h"

#include "../Common/LoadCodecs.h"

#include "../GUI/HashGUI.h"

#include "App.h"
#include "LangUtils.h"

#include "resource.h"

using namespace NWindows;
using namespace NFile;

static const UInt32 kBufSize = (1 << 15);

struct CDirEnumerator
{
  bool EnterToDirs;
  FString BasePrefix;
  FStringVector FilePaths;

  CObjectVector<NFind::CEnumerator> Enumerators;
  FStringVector Prefixes;
  unsigned Index;

  CDirEnumerator(): EnterToDirs(false), Index(0) {};

  void Init();
  DWORD GetNextFile(NFind::CFileInfo &fi, bool &filled, FString &resPath);
};

void CDirEnumerator::Init()
{
  Enumerators.Clear();
  Prefixes.Clear();
  Index = 0;
}

static DWORD GetNormalizedError()
{
  DWORD error = GetLastError();
  return (error == 0) ? E_FAIL : error;
}

DWORD CDirEnumerator::GetNextFile(NFind::CFileInfo &fi, bool &filled, FString &resPath)
{
  filled = false;
  resPath.Empty();
  for (;;)
  {
    if (Enumerators.IsEmpty())
    {
      if (Index >= FilePaths.Size())
        return S_OK;
      const FString &path = FilePaths[Index++];
      int pos = path.ReverseFind(FCHAR_PATH_SEPARATOR);
      if (pos >= 0)
        resPath.SetFrom(path, pos + 1);

      #ifdef _WIN32
      if (BasePrefix.IsEmpty() && path.Len() == 2 && path[1] == ':')
      {
        // we use "c:" item as directory item
        fi.Clear();
        fi.Name = path;
        fi.SetAsDir();
        fi.Size = 0;
      }
      else
      #endif
      if (!fi.Find(BasePrefix + path))
      {
        DWORD error = GetNormalizedError();
        resPath = path;
        return error;
      }
      break;
    }
    bool found;
    if (Enumerators.Back().Next(fi, found))
    {
      if (found)
      {
        resPath = Prefixes.Back();
        break;
      }
    }
    else
    {
      DWORD error = GetNormalizedError();
      resPath = Prefixes.Back();
      Enumerators.DeleteBack();
      Prefixes.DeleteBack();
      return error;
    }
    Enumerators.DeleteBack();
    Prefixes.DeleteBack();
  }
  resPath += fi.Name;
  if (EnterToDirs && fi.IsDir())
  {
    FString s = resPath;
    s += FCHAR_PATH_SEPARATOR;
    Prefixes.Add(s);
    s += FCHAR_ANY_MASK;
    Enumerators.Add(NFind::CEnumerator(BasePrefix + s));
  }
  filled = true;
  return S_OK;
}

class CThreadCrc: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  CDirEnumerator Enumerator;
  CHashBundle Hash;

  void SetStatus(const UString &s);
  void AddErrorMessage(DWORD systemError, const FChar *name);
};

void CThreadCrc::AddErrorMessage(DWORD systemError, const FChar *name)
{
  ProgressDialog.Sync.AddError_Code_Name(systemError, fs2us(Enumerator.BasePrefix + name));
  Hash.NumErrors++;
}

void CThreadCrc::SetStatus(const UString &s2)
{
  UString s = s2;
  if (Enumerator.BasePrefix)
  {
    if (!s.IsEmpty())
      s += L' ';
    s += fs2us(Enumerator.BasePrefix);
  }
  ProgressDialog.Sync.Set_Status(s);
}

HRESULT CThreadCrc::ProcessVirt()
{
  Hash.Init();
  
  CMyBuffer buf;
  if (!buf.Allocate(kBufSize))
    return E_OUTOFMEMORY;

  CProgressSync &sync = ProgressDialog.Sync;
  
  SetStatus(LangString(IDS_SCANNING));

  Enumerator.Init();

  FString path;
  NFind::CFileInfo fi;
  UInt64 numFiles = 0;
  UInt64 numItems = 0, numItems_Prev = 0;
  UInt64 totalSize = 0;

  for (;;)
  {
    bool filled;
    DWORD error = Enumerator.GetNextFile(fi, filled, path);
    if (error != 0)
    {
      AddErrorMessage(error, path);
      continue;
    }
    if (!filled)
      break;
    if (!fi.IsDir())
    {
      totalSize += fi.Size;
      numFiles++;
    }
    numItems++;
    bool needPrint = false;
    // if (fi.IsDir())
    {
      if (numItems - numItems_Prev >= 100)
      {
        needPrint = true;
        numItems_Prev = numItems;
      }
    }
    /*
    else if (numFiles - numFiles_Prev >= 200)
    {
      needPrint = true;
      numFiles_Prev = numFiles;
    }
    */
    if (needPrint)
    {
      RINOK(sync.ScanProgress(numFiles, totalSize, fs2us(path), fi.IsDir()));
    }
  }
  RINOK(sync.ScanProgress(numFiles, totalSize, L"", false));
  // sync.SetNumFilesTotal(numFiles);
  // sync.SetProgress(totalSize, 0);
  // SetStatus(LangString(IDS_CHECKSUM_CALCULATING));
  // sync.SetCurFilePath(L"");
  SetStatus(L"");
 
  Enumerator.Init();

  FString tempPath;
  FString firstFilePath;
  bool isFirstFile = true;
  UInt64 errorsFilesSize = 0;

  for (;;)
  {
    bool filled;
    DWORD error = Enumerator.GetNextFile(fi, filled, path);
    if (error != 0)
    {
      AddErrorMessage(error, path);
      continue;
    }
    if (!filled)
      break;
    
    error = 0;
    Hash.InitForNewFile();
    if (!fi.IsDir())
    {
      NIO::CInFile inFile;
      tempPath = Enumerator.BasePrefix;
      tempPath += path;
      if (!inFile.Open(tempPath))
      {
        error = GetNormalizedError();
        AddErrorMessage(error, path);
        continue;
      }
      if (isFirstFile)
      {
        firstFilePath = path;
        isFirstFile = false;
      }
      sync.Set_FilePath(fs2us(path));
      sync.Set_NumFilesCur(Hash.NumFiles);
      UInt64 progress_Prev = 0;
      for (;;)
      {
        UInt32 size;
        if (!inFile.Read(buf, kBufSize, size))
        {
          error = GetNormalizedError();
          AddErrorMessage(error, path);
          UInt64 errorSize = 0;
          if (inFile.GetLength(errorSize))
            errorsFilesSize += errorSize;
          break;
        }
        if (size == 0)
          break;
        Hash.Update(buf, size);
        if (Hash.CurSize - progress_Prev >= ((UInt32)1 << 21))
        {
          RINOK(sync.Set_NumBytesCur(errorsFilesSize + Hash.FilesSize + Hash.CurSize));
          progress_Prev = Hash.CurSize;
        }
      }
    }
    if (error == 0)
      Hash.Final(fi.IsDir(), false, fs2us(path));
    RINOK(sync.Set_NumBytesCur(errorsFilesSize + Hash.FilesSize));
  }
  RINOK(sync.Set_NumBytesCur(errorsFilesSize + Hash.FilesSize));
  sync.Set_NumFilesCur(Hash.NumFiles);
  if (Hash.NumFiles != 1)
    sync.Set_FilePath(L"");
  SetStatus(L"");

  CProgressMessageBoxPair &pair = GetMessagePair(Hash.NumErrors != 0);
  AddHashBundleRes(pair.Message, Hash, fs2us(firstFilePath));
  LangString(IDS_CHECKSUM_INFORMATION, pair.Title);
  return S_OK;
}

static void ThrowException_if_Error(HRESULT res)
{
  if (res != S_OK)
    throw CSystemException(res);
}

void CApp::CalculateCrc(const UString &methodName)
{
  int srcPanelIndex = GetFocusedPanelIndex();
  CPanel &srcPanel = Panels[srcPanelIndex];

  CRecordVector<UInt32> indices;
  srcPanel.GetOperatedIndicesSmart(indices);
  if (indices.IsEmpty())
    return;

  if (!srcPanel.IsFsOrDrivesFolder())
  {
    CCopyToOptions options;
    options.streamMode = true;
    options.showErrorMessages = true;
    options.hashMethods.Add(methodName);

    UStringVector messages;
    HRESULT res = srcPanel.CopyTo(options, indices, &messages);
    if (res != S_OK)
    {
      if (res != E_ABORT)
        srcPanel.MessageBoxError(res);
    }
    return;
  }

  CCodecs *codecs = new CCodecs;
  #ifdef EXTERNAL_CODECS
  CExternalCodecs __externalCodecs;
  __externalCodecs.GetCodecs = codecs;
  __externalCodecs.GetHashers = codecs;
  #else
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  #endif
  ThrowException_if_Error(codecs->Load());

  #ifdef EXTERNAL_CODECS
  ThrowException_if_Error(__externalCodecs.LoadCodecs());
  #endif

  {
    CThreadCrc t;
    {
      UStringVector methods;
      methods.Add(methodName);
      t.Hash.SetMethods(EXTERNAL_CODECS_VARS methods);
    }
    FOR_VECTOR (i, indices)
      t.Enumerator.FilePaths.Add(us2fs(srcPanel.GetItemRelPath(indices[i])));
    t.Enumerator.BasePrefix = us2fs(srcPanel.GetFsPath());
    t.Enumerator.EnterToDirs = !GetFlatMode();
    
    t.ProgressDialog.ShowCompressionInfo = false;
    
    UString title = LangString(IDS_CHECKSUM_CALCULATING);
    
    t.ProgressDialog.MainWindow = _window;
    t.ProgressDialog.MainTitle = L"7-Zip"; // LangString(IDS_APP_TITLE);
    t.ProgressDialog.MainAddTitle = title + UString(L' ');
    
    if (t.Create(title, _window) != S_OK)
      return;
  }
  RefreshTitleAlways();
}
