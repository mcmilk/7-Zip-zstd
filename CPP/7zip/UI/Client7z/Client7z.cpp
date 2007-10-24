// Client7z.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"

#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/DLL.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"

#include "../../Common/FileStreams.h"
#include "../../Archive/IArchive.h"
#include "../../IPassword.h"
#include "../../MyVersion.h"


// {23170F69-40C1-278A-1000-000110070000}
DEFINE_GUID(CLSID_CFormat7z, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

using namespace NWindows;

#define kDllName "7z.dll"

static const char *kCopyrightString = MY_7ZIP_VERSION
" ("  kDllName " client) "  
MY_COPYRIGHT " " MY_DATE;

static const char *kHelpString = 
"Usage: Client7z.exe [a | l | x ] archive.7z [fileName ...]\n"
"Examples:\n"
"  Client7z.exe a archive.7z f1.txt f2.txt  : compress two files to archive.7z\n"
"  Client7z.exe l archive.7z   : List contents of archive.7z\n"
"  Client7z.exe x archive.7z   : eXtract files from archive.7z\n";


typedef UINT32 (WINAPI * CreateObjectFunc)(
    const GUID *clsID, 
    const GUID *interfaceID, 
    void **outObject);

#ifdef _WIN32
#ifndef _UNICODE
bool g_IsNT = false;
static inline bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif
#endif

void PrintString(const UString &s)
{
  printf("%s", (LPCSTR)GetOemString(s));
}

void PrintString(const AString &s)
{
  printf("%s", (LPCSTR)s);
}

void PrintNewLine()
{
  PrintString("\n");
}

void PrintStringLn(const AString &s)
{
  PrintString(s);
  PrintNewLine();
}

void PrintError(const AString &s)
{
  PrintNewLine();
  PrintString(s);
  PrintNewLine();
}

static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop));
  if(prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt == VT_EMPTY)
    result = false;
  else
    return E_FAIL;
  return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsFolder, result);
}


static const wchar_t *kEmptyFileAlias = L"[Content]";


//////////////////////////////////////////////////////////////
// Archive Open callback class


class CArchiveOpenCallback: 
  public IArchiveOpenCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  bool PasswordIsDefined;
  UString Password;

  CArchiveOpenCallback() : PasswordIsDefined(false) {}
};

STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
  return S_OK;
}
  
STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    // You can ask real password here from user
    // Password = GetPassword(OutStream); 
    // PasswordIsDefined = true;
    PrintError("Password is not defined");
    return E_ABORT;
  }
  CMyComBSTR tempName(Password);
  *password = tempName.Detach();
  return S_OK;
}


//////////////////////////////////////////////////////////////
// Archive Extracting callback class

static const wchar_t *kCantDeleteOutputFile = L"ERROR: Can not delete output file ";

static const char *kTestingString    =  "Testing     ";
static const char *kExtractingString =  "Extracting  ";
static const char *kSkippingString   =  "Skipping    ";

static const char *kUnsupportedMethod = "Unsupported Method";
static const char *kCRCFailed = "CRC Failed";
static const char *kDataError = "Data Error";
static const char *kUnknownError = "Unknown Error";

class CArchiveExtractCallback: 
  public IArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IArchiveExtractCallback
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
  STDMETHOD(PrepareOperation)(Int32 askExtractMode);
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CMyComPtr<IInArchive> _archiveHandler;
  UString _directoryPath;  // Output directory
  UString _filePath;       // name inside arcvhive
  UString _diskFilePath;   // full path to file on disk
  bool _extractMode;
  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    UInt32 Attributes;
    bool IsDirectory;
    bool AttributesAreDefined;
    bool UTCLastWriteTimeIsDefined;
  } _processedFileInfo;

  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;

public:
  void Init(IInArchive *archiveHandler, const UString &directoryPath);

  UInt64 NumErrors;
  bool PasswordIsDefined;
  UString Password;

  CArchiveExtractCallback() : PasswordIsDefined(false) {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const UString &directoryPath)
{
  NumErrors = 0;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 /* size */)
{
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */)
{
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, 
    ISequentialOutStream **outStream, Int32 askExtractMode)
{
  *outStream = 0;
  _outFileStream.Release();

  {
    // Get Name
    NCOM::CPropVariant propVariant;
    RINOK(_archiveHandler->GetProperty(index, kpidPath, &propVariant));
    
    UString fullPath;
    if(propVariant.vt == VT_EMPTY)
      fullPath = kEmptyFileAlias;
    else 
    {
      if(propVariant.vt != VT_BSTR)
        return E_FAIL;
      fullPath = propVariant.bstrVal;
    }
    _filePath = fullPath;
  }

  if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
    return S_OK;

  {
    // Get Attributes
    NCOM::CPropVariant propVariant;
    RINOK(_archiveHandler->GetProperty(index, kpidAttributes, &propVariant));
    if (propVariant.vt == VT_EMPTY)
    {
      _processedFileInfo.Attributes = 0;
      _processedFileInfo.AttributesAreDefined = false;
    }
    else
    {
      if (propVariant.vt != VT_UI4)
        throw "incorrect item";
      _processedFileInfo.Attributes = propVariant.ulVal;
      _processedFileInfo.AttributesAreDefined = true;
    }
  }

  RINOK(IsArchiveItemFolder(_archiveHandler, index, _processedFileInfo.IsDirectory));

  {
    // Get Modified Time
    NCOM::CPropVariant propVariant;
    RINOK(_archiveHandler->GetProperty(index, kpidLastWriteTime, &propVariant));
    _processedFileInfo.UTCLastWriteTimeIsDefined = false;
    switch(propVariant.vt)
    {
      case VT_EMPTY:
        // _processedFileInfo.UTCLastWriteTime = _utcLastWriteTimeDefault;
        break;
      case VT_FILETIME:
        _processedFileInfo.UTCLastWriteTime = propVariant.filetime;
        _processedFileInfo.UTCLastWriteTimeIsDefined = true;
        break;
      default:
        return E_FAIL;
    }

  }
  {
    // Get Size
    NCOM::CPropVariant propVariant;
    RINOK(_archiveHandler->GetProperty(index, kpidSize, &propVariant));
    bool newFileSizeDefined = (propVariant.vt != VT_EMPTY);
    UInt64 newFileSize;
    if (newFileSizeDefined)
      newFileSize = ConvertPropVariantToUInt64(propVariant);
  }

  
  {
    // Create folders for file
    int slashPos = _filePath.ReverseFind(WCHAR_PATH_SEPARATOR);
    if (slashPos >= 0)
      NFile::NDirectory::CreateComplexDirectory(_directoryPath + _filePath.Left(slashPos));
  }

  UString fullProcessedPath = _directoryPath + _filePath;
  _diskFilePath = fullProcessedPath;

  if (_processedFileInfo.IsDirectory)
  {
    NFile::NDirectory::CreateComplexDirectory(fullProcessedPath);
  }
  else
  {
    NFile::NFind::CFileInfoW fileInfo;
    if(NFile::NFind::FindFile(fullProcessedPath, fileInfo))
    {
      if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
      {
        PrintString(UString(kCantDeleteOutputFile) + fullProcessedPath);
        return E_ABORT;
      }
    }
    
    _outFileStreamSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
    if (!_outFileStreamSpec->Open(fullProcessedPath, CREATE_ALWAYS))
    {
      PrintString((UString)L"can not open output file " + fullProcessedPath);
      return E_ABORT;
    }
    _outFileStream = outStreamLoc;
    *outStream = outStreamLoc.Detach();
  }
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      _extractMode = true;
  };
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      PrintString(kExtractingString);
      break;
    case NArchive::NExtract::NAskMode::kTest:
      PrintString(kTestingString);
      break;
    case NArchive::NExtract::NAskMode::kSkip:
      PrintString(kSkippingString);
      break;
  };
  PrintString(_filePath);
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      NumErrors++;
      PrintString("     ");
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          PrintString(kUnsupportedMethod);
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          PrintString(kCRCFailed);
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          PrintString(kDataError);
          break;
        default:
          PrintString(kUnknownError);
      }
    }
  }

  if (_outFileStream != NULL)
  {
    if (_processedFileInfo.UTCLastWriteTimeIsDefined)
      _outFileStreamSpec->SetLastWriteTime(&_processedFileInfo.UTCLastWriteTime);
    RINOK(_outFileStreamSpec->Close());
  }
  _outFileStream.Release();
  if (_extractMode && _processedFileInfo.AttributesAreDefined)
    NFile::NDirectory::MySetFileAttributes(_diskFilePath, _processedFileInfo.Attributes);
  PrintNewLine();
  return S_OK;
}


STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    // You can ask real password here from user
    // Password = GetPassword(OutStream); 
    // PasswordIsDefined = true;
    PrintError("Password is not defined");
    return E_ABORT;
  }
  CMyComBSTR tempName(Password);
  *password = tempName.Detach();
  return S_OK;
}



//////////////////////////////////////////////////////////////
// Archive Creating callback class

struct CDirItem
{ 
  UInt32 Attributes;
  FILETIME CreationTime;
  FILETIME LastAccessTime;
  FILETIME LastWriteTime;
  UInt64 Size;
  UString Name;
  UString FullPath;
  bool IsDirectory() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ; }
};

class CArchiveUpdateCallback: 
  public IArchiveUpdateCallback2,
  public ICryptoGetTextPassword2,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(IArchiveUpdateCallback2, ICryptoGetTextPassword2)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IUpdateCallback2
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetUpdateItemInfo)(UInt32 index, 
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive);
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **inStream);
  STDMETHOD(SetOperationResult)(Int32 operationResult);
  STDMETHOD(GetVolumeSize)(UInt32 index, UInt64 *size);
  STDMETHOD(GetVolumeStream)(UInt32 index, ISequentialOutStream **volumeStream);

  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

public:
  CRecordVector<UInt64> VolumesSizes;
  UString VolName;
  UString VolExt;

  UString DirPrefix;
  const CObjectVector<CDirItem> *DirItems;

  bool PasswordIsDefined;
  UString Password;
  bool AskPassword;

  bool m_NeedBeClosed;

  UStringVector FailedFiles;
  CRecordVector<HRESULT> FailedCodes;

  CArchiveUpdateCallback(): PasswordIsDefined(false), AskPassword(false), DirItems(0) {};

  ~CArchiveUpdateCallback() { Finilize(); }
  HRESULT Finilize();

  void Init(const CObjectVector<CDirItem> *dirItems)
  {
    DirItems = dirItems;
    m_NeedBeClosed = false;
    FailedFiles.Clear();
    FailedCodes.Clear();
  }
};

STDMETHODIMP CArchiveUpdateCallback::SetTotal(UInt64 /* size */)
{
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const UInt64 * /* completeValue */)
{
  return S_OK;
}


STDMETHODIMP CArchiveUpdateCallback::EnumProperties(IEnumSTATPROPSTG ** /* enumerator */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(UInt32 /* index */, 
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive)
{
  if(newData != NULL)
    *newData = BoolToInt(true);
  if(newProperties != NULL)
    *newProperties = BoolToInt(true);
  if(indexInArchive != NULL)
    *indexInArchive = UInt32(-1);
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
  
  if (propID == kpidIsAnti)
  {
    propVariant = false;
    propVariant.Detach(value);
    return S_OK;
  }

  {
    const CDirItem &dirItem = (*DirItems)[index];
    switch(propID)
    {
      case kpidPath:
        propVariant = dirItem.Name;
        break;
      case kpidIsFolder:
        propVariant = dirItem.IsDirectory();
        break;
      case kpidSize:
        propVariant = dirItem.Size;
        break;
      case kpidAttributes:
        propVariant = dirItem.Attributes;
        break;
      case kpidLastAccessTime:
        propVariant = dirItem.LastAccessTime;
        break;
      case kpidCreationTime:
        propVariant = dirItem.CreationTime;
        break;
      case kpidLastWriteTime:
        propVariant = dirItem.LastWriteTime;
        break;
    }
  }
  propVariant.Detach(value);
  return S_OK;
}

HRESULT CArchiveUpdateCallback::Finilize()
{
  if (m_NeedBeClosed)
  {
    PrintNewLine();
    m_NeedBeClosed = false;
  }
  return S_OK;
}

static void GetStream2(const wchar_t *name)
{
  PrintString("Compressing  ");
  if (name[0] == 0)
    name = kEmptyFileAlias;
  PrintString(name);
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream)
{
  RINOK(Finilize());

  const CDirItem &dirItem = (*DirItems)[index];
  GetStream2(dirItem.Name);
 
  if(dirItem.IsDirectory())
    return S_OK;

  {
    CInFileStream *inStreamSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
    UString path = DirPrefix + dirItem.FullPath;
    if(!inStreamSpec->Open(path))
    {
      DWORD sysError = ::GetLastError();
      FailedCodes.Add(sysError);
      FailedFiles.Add(path);
      // if (systemError == ERROR_SHARING_VIOLATION)
      {
        PrintNewLine();
        PrintError("WARNING: can't open file");
        // PrintString(NError::MyFormatMessageW(systemError));
        return S_FALSE;
      }
      // return sysError;
    }
    *inStream = inStreamLoc.Detach();
  }
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetOperationResult(Int32 /* operationResult */)
{
  m_NeedBeClosed = true;
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeSize(UInt32 index, UInt64 *size)
{
  if (VolumesSizes.Size() == 0)
    return S_FALSE;
  if (index >= (UInt32)VolumesSizes.Size())
    index = VolumesSizes.Size() - 1;
  *size = VolumesSizes[index];
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeStream(UInt32 index, ISequentialOutStream **volumeStream)
{
  wchar_t temp[32];
  ConvertUInt64ToString(index + 1, temp);
  UString res = temp;
  while (res.Length() < 2)
    res = UString(L'0') + res;
  UString fileName = VolName;
  fileName += L'.';
  fileName += res;
  fileName += VolExt;
  COutFileStream *streamSpec = new COutFileStream;
  CMyComPtr<ISequentialOutStream> streamLoc(streamSpec);
  if(!streamSpec->Create(fileName, false))
    return ::GetLastError();
  *volumeStream = streamLoc.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  if (!PasswordIsDefined) 
  {
    if (AskPassword)
    {
      // You can ask real password here from user
      // Password = GetPassword(OutStream); 
      // PasswordIsDefined = true;
      PrintError("Password is not defined");
      return E_ABORT;
    }
  }
  *passwordIsDefined = BoolToInt(PasswordIsDefined);
  CMyComBSTR tempName(Password);
  *password = tempName.Detach();
  return S_OK;
}



//////////////////////////////////////////////////////////////////////////
// Main function

int 
#ifdef _MSC_VER
__cdecl 
#endif
main(int argc, char* argv[])
{
  #ifdef _WIN32
  #ifndef _UNICODE
  g_IsNT = IsItWindowsNT();
  #endif
  #endif

  PrintStringLn(kCopyrightString);

  if (argc < 3)
  {
    PrintStringLn(kHelpString);
    return 1;
  }
  NWindows::NDLL::CLibrary library;
  if (!library.Load(TEXT(kDllName)))
  {
    PrintError("Can not load library");
    return 1;
  }
  CreateObjectFunc createObjectFunc = (CreateObjectFunc)library.GetProcAddress("CreateObject");
  if (createObjectFunc == 0)
  {
    PrintError("Can not get CreateObject");
    return 1;
  }

  AString command = argv[1];
  UString archiveName = GetUnicodeString(argv[2], CP_OEMCP);
  if (command.CompareNoCase("a") == 0)
  {
    // create archive command
    if (argc < 4)
    {
      PrintStringLn(kHelpString);
      return 1;
    }
    CObjectVector<CDirItem> dirItems;
    int i;
    for (i = 3; i < argc; i++)
    {
      CDirItem item;
      UString name = GetUnicodeString(argv[i], CP_OEMCP);
      
      NFile::NFind::CFileInfoW fileInfo;
      if (!NFile::NFind::FindFile(name, fileInfo))
      {
        PrintString(UString(L"Can't find file") + name);
        return 1;
      }

      item.Attributes = fileInfo.Attributes;
      item.Size = fileInfo.Size;
      item.CreationTime = fileInfo.CreationTime;
      item.LastAccessTime = fileInfo.LastAccessTime;
      item.LastWriteTime = fileInfo.LastWriteTime;
      item.Name = name;
      item.FullPath = name;
      dirItems.Add(item);
    }
    COutFileStream *outFileStreamSpec = new COutFileStream;
    CMyComPtr<IOutStream> outFileStream = outFileStreamSpec;
    if (!outFileStreamSpec->Create(archiveName, false))
    {
      PrintError("can't create archive file");
      return 1;
    }

    CMyComPtr<IOutArchive> outArchive;
    if (createObjectFunc(&CLSID_CFormat7z, &IID_IOutArchive, (void **)&outArchive) != S_OK)
    {
      PrintError("Can not get class object");
      return 1;
    }

    CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
    CMyComPtr<IArchiveUpdateCallback2> updateCallback(updateCallbackSpec);
    updateCallbackSpec->Init(&dirItems);
    // updateCallbackSpec->PasswordIsDefined = true;
    // updateCallbackSpec->Password = L"1";

    HRESULT result = outArchive->UpdateItems(outFileStream, dirItems.Size(), updateCallback);
    updateCallbackSpec->Finilize();
    if (result != S_OK)
    {
      PrintError("Update Error");
      return 1;
    }
    for (i = 0; i < updateCallbackSpec->FailedFiles.Size(); i++)
    {
      PrintNewLine();
      PrintString((UString)L"Error for file: " + updateCallbackSpec->FailedFiles[i]);
    }
    if (updateCallbackSpec->FailedFiles.Size() != 0)
      return 1;
  }
  else
  {
    if (argc != 3)
    {
      PrintStringLn(kHelpString);
      return 1;
    }

    bool listCommand;
    if (command.CompareNoCase("l") == 0)
      listCommand = true;
    else if (command.CompareNoCase("x") == 0)
      listCommand = false;
    else
    {
      PrintError("incorrect command");
      return 1;
    }
  
    CMyComPtr<IInArchive> archive;
    if (createObjectFunc(&CLSID_CFormat7z, &IID_IInArchive, (void **)&archive) != S_OK)
    {
      PrintError("Can not get class object");
      return 1;
    }
    
    CInFileStream *fileSpec = new CInFileStream;
    CMyComPtr<IInStream> file = fileSpec;
    
    if (!fileSpec->Open(archiveName))
    {
      PrintError("Can not open archive file");
      return 1;
    }

    {
      CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
      CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
      openCallbackSpec->PasswordIsDefined = false;
      // openCallbackSpec->PasswordIsDefined = true;
      // openCallbackSpec->Password = L"1";
      
      if (archive->Open(file, 0, openCallback) != S_OK)
      {
        PrintError("Can not open archive");
        return 1;
      }
    }
    
    if (listCommand)
    {
      // List command
      UInt32 numItems = 0;
      archive->GetNumberOfItems(&numItems);  
      for (UInt32 i = 0; i < numItems; i++)
      {
        {
          // Get uncompressed size of file
          NWindows::NCOM::CPropVariant propVariant;
          archive->GetProperty(i, kpidSize, &propVariant);
          UString s = ConvertPropVariantToString(propVariant);
          PrintString(s);
          PrintString("  ");
        }
        {
          // Get name of file
          NWindows::NCOM::CPropVariant propVariant;
          archive->GetProperty(i, kpidPath, &propVariant);
          UString s = ConvertPropVariantToString(propVariant);
          PrintString(s);
        }
        PrintString("\n");
      }
    }
    else
    {
      // Extract command
      CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
      CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
      extractCallbackSpec->Init(archive, L""); // second parameter is output folder path
      extractCallbackSpec->PasswordIsDefined = false;
      // extractCallbackSpec->PasswordIsDefined = true;
      // extractCallbackSpec->Password = L"1";
      HRESULT result = archive->Extract(NULL, (UInt32)(Int32)(-1), false, extractCallback);
      if (result != S_OK)
      {
        PrintError("Extract Error");
        return 1;
      }
    }
  }
  return 0;
}
