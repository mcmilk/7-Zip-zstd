// Zip/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/COM.h"
#include "Windows/Registry.h"

#include "Interface/FileStreams.h"

#include "Common/StringConvert.h"

#include "../Common/DefaultName.h"

using namespace NWindows;
using namespace NRegistry;
using namespace NCOM;

static const UINT64 kMaxCheckStartPosition = 1 << 20;

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CAgent::LoadFormats()
{
  if (!_formatsLoaded)
    NZipRootRegistry::ReadArchiverInfoList(_formats);
}

int CAgent::FindFormat(const UString &type)
{
  // LoadFormats();
  for (int i = 0; i < _formats.Size(); i++)
    if (type.CompareNoCase(GetUnicodeString(_formats[i].Name)) == 0)
      return i;
  return -1;
}

STDMETHODIMP CAgent::OpenFolderFile(const wchar_t *filePath, 
    IFolderFolder **resultFolder, IProgress *progress)
{
  LoadFormats();
  CComPtr<IArchiveOpenCallback> openArchiveCallback;
  if (progress != 0)
  {
    CComPtr<IProgress> progressWrapper = progress;
    // RETURN_IF_NOT_S_OK(progressWrapper.QueryInterface(&openArchiveCallback));
    progressWrapper.QueryInterface(&openArchiveCallback);
  }
  UString defaultName;
  UINT codePage = GetCurrentFileCodePage();
  CSysString filePath2 = GetSystemString(filePath, codePage);
  _archiveFilePath = filePath2;

  /*
  int slashPos = _archiveFilePath.ReverseFind('\\');
  if (slashPos >= 0)
  {
    _srcDirectoryPrefix = _archiveFilePath.Left(slashPos + 1);
    _fileName = _archiveFilePath.Mid(slashPos + 1);
  }
  else
  {
    _srcDirectoryPrefix.Empty();
    fileName = _archiveFilePath;
  }
  */

  
  

  CSysString extension;
  {
    CSysString name, pureName, dot;
    if(!NFile::NDirectory::GetOnlyName(filePath2, name))
      return E_FAIL;
    NFile::NName::SplitNameToPureNameAndExtension(name, pureName, dot, extension);
  }
  CIntVector orderIndices;
  for(int firstArchiverIndex = 0; 
      firstArchiverIndex < _formats.Size(); firstArchiverIndex++)
    if(extension.CollateNoCase(_formats[firstArchiverIndex].Extension) == 0)
      break;
  if(firstArchiverIndex < _formats.Size())
    orderIndices.Add(firstArchiverIndex);
  for(int j = 0; j < _formats.Size(); j++)
    if(j != firstArchiverIndex)
      orderIndices.Add(j);
  
  NCOM::CComInitializer comInitializer;
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;

  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(filePath2, fileInfo))
    return E_FAIL;
  CComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(filePath2))
    return E_FAIL;

  HRESULT badResult = S_OK;
  for(int i = 0; i < orderIndices.Size(); i++)
  {
    inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const NZipRootRegistry::CArchiverInfo &archiverInfo = 
        _formats[orderIndices[i]];
    
    defaultName = GetDefaultName(filePath2, archiverInfo.Extension, 
        GetUnicodeString(archiverInfo.AddExtension));

    #ifdef EXCLUDE_COM
    CLSID classID;
    classID.Data4[5] = 5;
    #endif
    HRESULT result = Open(
        inStream, defaultName, 
        &fileInfo.LastWriteTime, fileInfo.Attributes, 
        &kMaxCheckStartPosition, 

        #ifdef EXCLUDE_COM
        &classID,
        #else
        &archiverInfo.ClassID, 
        #endif

        openArchiveCallback);

    if(result == S_FALSE)
      continue;
    if(result != S_OK)
    {
      badResult = result;
      continue;
      // return result;
    }
    // CoFreeUnusedLibraries();
    return BindToRootFolder(resultFolder);
  }
  if (badResult != S_OK)
    return badResult;
  // OutputDebugString("Fin=======\n");
  // CoFreeUnusedLibraries();
  return S_FALSE;
}


HRESULT CAgent::FolderReOpen(
    IArchiveOpenCallback *openArchiveCallback)
{
  CSysString fileName = _archiveFilePath;
  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(fileName, fileInfo))
    return E_FAIL;
  NCOM::CComInitializer comInitializer;
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(fileName))
    return E_FAIL;

  inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
  UString defaultName = _defaultName;
  return ReOpen(inStream, defaultName, 
        &fileInfo.LastWriteTime, fileInfo.Attributes, 
        &kMaxCheckStartPosition, openArchiveCallback);
}

STDMETHODIMP CAgent::GetTypes(BSTR *types)
{
  LoadFormats();
  UString typesStrings;
  for(int i = 0; i < _formats.Size(); i++)
  {
    if (i != 0)
      typesStrings += L' ';
    typesStrings += GetUnicodeString(_formats[i].Name);
  }
  CComBSTR valueTemp = typesStrings;
  *types = valueTemp.Detach();
  return S_OK;
}

STDMETHODIMP CAgent::GetExtension(const wchar_t *type, BSTR *extension)
{
  *extension = 0;
  int formatIndex = FindFormat(type);
  if (formatIndex <  0)
    return E_INVALIDARG;
  CComBSTR valueTemp = GetUnicodeString(_formats[formatIndex].Extension);
  *extension = valueTemp.Detach();
  return S_OK;
}

static const TCHAR *kCLSIDKeyName = _T("CLSID");
static const TCHAR *kInprocServer32KeyName = _T("InprocServer32");

STDMETHODIMP CAgent::GetIconPath(const wchar_t *type, BSTR *iconPath)
{
  *iconPath = 0;
  int formatIndex = FindFormat(type);
  if (formatIndex <  0)
    return E_INVALIDARG;
  NRegistry::CKey key;
  CSysString keyPath = kCLSIDKeyName;
  keyPath += kKeyNameDelimiter;
  keyPath += GUIDToString(_formats[formatIndex].ClassID);
  keyPath += kKeyNameDelimiter;
  keyPath += kInprocServer32KeyName;
  CSysString tempPath;
  if (key.Open(HKEY_CLASSES_ROOT, keyPath, KEY_READ) == ERROR_SUCCESS)
    key.QueryValue(NULL, tempPath);
  CComBSTR iconPathTemp = GetUnicodeString(tempPath);
  *iconPath = iconPathTemp.Detach();
  return S_OK;
}

STDMETHODIMP CAgent::CreateFolderFile(const wchar_t *type, const wchar_t *filePath, IProgress *progress)
{
  return E_NOTIMPL;
}



