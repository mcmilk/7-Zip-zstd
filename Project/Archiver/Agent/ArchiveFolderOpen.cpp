// Zip/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/COM.h"

#include "Interface/FileStreams.h"

#include "Common/StringConvert.h"

#include "../Common/DefaultName.h"
#include "../Common/ZipRegistryMain.h"

using namespace NWindows;

static const UINT64 kMaxCheckStartPosition = 1 << 20;

STDMETHODIMP CAgent::FolderOpen(
    const wchar_t *_aFileName, 
    IOpenArchive2CallBack *anOpenArchive2CallBack)
{
  UString aDefaultName;

  CSysString aFileName = GetSystemString(_aFileName, CP_OEMCP);
  CObjectVector<NZipRootRegistry::CArchiverInfo> anArchiverInfoList;
  NZipRootRegistry::ReadArchiverInfoList(anArchiverInfoList);
  CSysString anExtension;
  {
    CSysString aName, aPureName, aDot;
    if(!NFile::NDirectory::GetOnlyName(aFileName, aName))
      return E_FAIL;
    NFile::NName::SplitNameToPureNameAndExtension(aName, aPureName, aDot, anExtension);
  }
  std::vector<int> anOrderIndexes;
  for(int anFirstArchiverIndex = 0; 
      anFirstArchiverIndex < anArchiverInfoList.Size(); anFirstArchiverIndex++)
    if(anExtension.CollateNoCase(anArchiverInfoList[anFirstArchiverIndex].Extension) == 0)
      break;
  if(anFirstArchiverIndex < anArchiverInfoList.Size())
    anOrderIndexes.push_back(anFirstArchiverIndex);
  for(int j = 0; j < anArchiverInfoList.Size(); j++)
    if(j != anFirstArchiverIndex)
      anOrderIndexes.push_back(j);
  
  NCOM::CComInitializer aComInitializer;
  CComObjectNoLock<CInFileStream> *anInStreamSpec = new 
    CComObjectNoLock<CInFileStream>;

  NFile::NFind::CFileInfo aFileInfo;
  if (!NFile::NFind::FindFile(aFileName, aFileInfo))
    return E_FAIL;
  CComPtr<IInStream> anInStream(anInStreamSpec);
  if (!anInStreamSpec->Open(aFileName))
    return E_FAIL;


  for(int i = 0; i < anOrderIndexes.size(); i++)
  {
    anInStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
        anArchiverInfoList[anOrderIndexes[i]];
    
    /*
    */
    aDefaultName = GetDefaultName(aFileName, anArchiverInfo.Extension, 
        GetUnicodeString(anArchiverInfo.AddExtension));

    #ifdef EXCLUDE_COM
    CLSID aClassID;
    aClassID.Data4[5] = 5;
    #endif
    HRESULT aResult = Open(
        anInStream, aDefaultName, 
        &aFileInfo.LastWriteTime, aFileInfo.Attributes, 
        &kMaxCheckStartPosition, 

        #ifdef EXCLUDE_COM
        &aClassID,
        #else
        &anArchiverInfo.ClassID, 
        #endif

        anOpenArchive2CallBack);

    if(aResult == S_FALSE)
      continue;
    if(aResult != S_OK)
      return aResult;
    //anArchiverInfoResult = anArchiverInfo;
    return S_OK;
  }
  // OutputDebugString("Fin=======\n");
  return S_FALSE;
}

