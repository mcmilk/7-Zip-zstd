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
  CComPtr<IOpenArchive2CallBack> openArchive2CallBack;
  if (progress != 0)
  {
    CComPtr<IProgress> progressWrapper = progress;
    RETURN_IF_NOT_S_OK(progressWrapper.QueryInterface(&openArchive2CallBack));
  }
  UString defaultName;
  UINT codePage = GetCurrentFileCodePage();
  CSysString filePath2 = GetSystemString(filePath, codePage);
  _archiveFilePath = filePath2;
  
  // CObjectVector<NZipRootRegistry::CArchiverInfo> anArchiverInfoList;
  // NZipRootRegistry::ReadArchiverInfoList(anArchiverInfoList);

  CSysString anExtension;
  {
    CSysString name, pureName, dot;
    if(!NFile::NDirectory::GetOnlyName(filePath2, name))
      return E_FAIL;
    NFile::NName::SplitNameToPureNameAndExtension(name, pureName, dot, anExtension);
  }
  std::vector<int> anOrderIndexes;
  for(int anFirstArchiverIndex = 0; 
      anFirstArchiverIndex < _formats.Size(); anFirstArchiverIndex++)
    if(anExtension.CollateNoCase(_formats[anFirstArchiverIndex].Extension) == 0)
      break;
  if(anFirstArchiverIndex < _formats.Size())
    anOrderIndexes.push_back(anFirstArchiverIndex);
  for(int j = 0; j < _formats.Size(); j++)
    if(j != anFirstArchiverIndex)
      anOrderIndexes.push_back(j);
  
  NCOM::CComInitializer aComInitializer;
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;

  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(filePath2, fileInfo))
    return E_FAIL;
  CComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(filePath2))
    return E_FAIL;


  for(int i = 0; i < anOrderIndexes.size(); i++)
  {
    inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
        _formats[anOrderIndexes[i]];
    
    /*
    */
    defaultName = GetDefaultName(filePath2, anArchiverInfo.Extension, 
        GetUnicodeString(anArchiverInfo.AddExtension));

    #ifdef EXCLUDE_COM
    CLSID aClassID;
    aClassID.Data4[5] = 5;
    #endif
    HRESULT aResult = Open(
        inStream, defaultName, 
        &fileInfo.LastWriteTime, fileInfo.Attributes, 
        &kMaxCheckStartPosition, 

        #ifdef EXCLUDE_COM
        &aClassID,
        #else
        &anArchiverInfo.ClassID, 
        #endif

        openArchive2CallBack);

    if(aResult == S_FALSE)
      continue;
    if(aResult != S_OK)
      return aResult;
    //anArchiverInfoResult = anArchiverInfo;
    // CoFreeUnusedLibraries();
    return BindToRootFolder(resultFolder);
  }
  // OutputDebugString("Fin=======\n");
  // CoFreeUnusedLibraries();
  return S_FALSE;
}


HRESULT CAgent::FolderReOpen(
    IOpenArchive2CallBack *openArchive2CallBack)
{
  CSysString fileName = _archiveFilePath;
  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(fileName, fileInfo))
    return E_FAIL;
  NCOM::CComInitializer aComInitializer;
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(fileName))
    return E_FAIL;

  inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
  UString defaultName = _defaultName;
  return ReOpen(inStream, defaultName, 
        &fileInfo.LastWriteTime, fileInfo.Attributes, 
        &kMaxCheckStartPosition, openArchive2CallBack);
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
  NRegistry::CKey aKey;
  CSysString aKeyPath = kCLSIDKeyName;
  aKeyPath += kKeyNameDelimiter;
  aKeyPath += GUIDToString(_formats[formatIndex].ClassID);
  aKeyPath += kKeyNameDelimiter;
  aKeyPath += kInprocServer32KeyName;
  CSysString tempPath;
  if (aKey.Open(HKEY_CLASSES_ROOT, aKeyPath, KEY_READ) == ERROR_SUCCESS)
    aKey.QueryValue(NULL, tempPath);
  CComBSTR iconPathTemp = GetUnicodeString(tempPath);
  *iconPath = iconPathTemp.Detach();
  return S_OK;
}

STDMETHODIMP CAgent::CreateFolderFile(const wchar_t *type, const wchar_t *filePath, IProgress *progress)
{
  return E_NOTIMPL;
}



