// OpenEngine.h

#include "StdAfx.h"

#include "OpenEngine2.h"

#include "ZipRegistryMain.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/COM.h"

#include "Interface/FileStreams.h"

#include "Common/StringConvert.h"

#include "DefaultName.h"

#include "../Agent/Handler.h"

using namespace NWindows;

static const UINT64 kMaxCheckStartPosition = 1 << 20;

HRESULT ReOpenArchive(IArchiveHandler100 *anArchiveHandler,
    const UString &aDefaultName,
    const CSysString &aFileName)
{
  NFile::NFind::CFileInfo aFileInfo;
  if (!NFile::NFind::FindFile(aFileName, aFileInfo))
    return E_FAIL;
  NCOM::CComInitializer aComInitializer; // test it
  CComObjectNoLock<CInFileStream> *anInStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> anInStream(anInStreamSpec);
  anInStreamSpec->Open(aFileName);
  return anArchiveHandler->ReOpen(anInStream, aDefaultName, 
      &aFileInfo.LastWriteTime, aFileInfo.Attributes, 
      &kMaxCheckStartPosition, NULL);
}

/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler100 **anArchiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult,
    UString &aDefaultName,
    IOpenArchive2CallBack *anOpenArchive2CallBack)
{
  *anArchiveHandlerResult = NULL;
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


  CComPtr<IArchiveHandler100> anArchiveHandler;

  CComObjectNoLock<CAgent> *anAgentSpec = new CComObjectNoLock<CAgent>;
  anArchiveHandler = anAgentSpec;
  
  /*
  HRESULT aResult = anArchiveHandler.CoCreateInstance(CLSID_CAgentArchiveHandler);
  if (aResult != S_OK)
    return aResult;
  */

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
    HRESULT aResult = anAgentSpec->Open(
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
    *anArchiveHandlerResult = anArchiveHandler.Detach();
    anArchiverInfoResult = anArchiverInfo;
    return S_OK;
  }
  // OutputDebugString("Fin=======\n");
  return S_FALSE;
}

/*
  HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler100 **anArchiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult)
{
  return OpenArchive(aFileName, anArchiveHandlerResult, anArchiverInfoResult, NULL);
}
*/
