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

HRESULT ReOpenArchive(IInFolderArchive *archiveHandler,
    const UString &defaultName,
    const CSysString &fileName)
{
  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(fileName, fileInfo))
    return E_FAIL;
  NCOM::CComInitializer comInitializer; // test it
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStream(inStreamSpec);
  inStreamSpec->Open(fileName);
  return archiveHandler->ReOpen(inStream, defaultName, 
      &fileInfo.LastWriteTime, fileInfo.Attributes, 
      &kMaxCheckStartPosition, NULL);
}

/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

HRESULT OpenArchive(const CSysString &fileName, 
    IInFolderArchive **archiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult,
    UString &defaultName,
    IArchiveOpenCallback *openArchiveCallback)
{
  *archiveHandlerResult = NULL;
  CObjectVector<NZipRootRegistry::CArchiverInfo> archiverInfoList;
  NZipRootRegistry::ReadArchiverInfoList(archiverInfoList);
  CSysString extension;
  {
    CSysString name, pureName, dot;
    if(!NFile::NDirectory::GetOnlyName(fileName, name))
      return E_FAIL;
    NFile::NName::SplitNameToPureNameAndExtension(name, pureName, dot, extension);
  }
  CIntVector orderIndices;
  for(int firstArchiverIndex = 0; 
      firstArchiverIndex < archiverInfoList.Size(); firstArchiverIndex++)
    if(extension.CollateNoCase(archiverInfoList[firstArchiverIndex].Extension) == 0)
      break;
  if(firstArchiverIndex < archiverInfoList.Size())
    orderIndices.Add(firstArchiverIndex);
  for(int j = 0; j < archiverInfoList.Size(); j++)
    if(j != firstArchiverIndex)
      orderIndices.Add(j);
  
  NCOM::CComInitializer comInitializer;
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;

  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(fileName, fileInfo))
    return E_FAIL;
  CComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(fileName))
    return E_FAIL;


  CComPtr<IInFolderArchive> archiveHandler;

  CComObjectNoLock<CAgent> *agentSpec = new CComObjectNoLock<CAgent>;
  archiveHandler = agentSpec;
  
  /*
  HRESULT result = archiveHandler.CoCreateInstance(CLSID_CAgentArchiveHandler);
  if (result != S_OK)
    return result;
  */

  HRESULT badResult = S_OK;
  for(int i = 0; i < orderIndices.Size(); i++)
  {
    inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const NZipRootRegistry::CArchiverInfo &archiverInfo = 
        archiverInfoList[orderIndices[i]];
    
    /*
    */
    defaultName = GetDefaultName(fileName, archiverInfo.Extension, 
        GetUnicodeString(archiverInfo.AddExtension));

    #ifdef EXCLUDE_COM
    CLSID classID;
    classID.Data4[5] = 5;
    #endif
    HRESULT result = agentSpec->Open(
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
    *archiveHandlerResult = archiveHandler.Detach();
    archiverInfoResult = archiverInfo;
    return S_OK;
  }
  if (badResult != S_OK)
    return badResult;
  // OutputDebugString("Fin=======\n");
  return S_FALSE;
}

/*
  HRESULT OpenArchive(const CSysString &fileName, 
    IInFolderArchive **archiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult)
{
  return OpenArchive(fileName, archiveHandlerResult, archiverInfoResult, NULL);
}
*/
