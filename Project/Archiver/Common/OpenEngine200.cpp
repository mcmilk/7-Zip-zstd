// OpenEngine200.cpp

#include "StdAfx.h"

#include "OpenEngine200.h"

#include "../Common/ZipRegistryMain.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/COM.h"
#include "Windows/Defs.h"

#include "Interface/FileStreams.h"

#include "Common/StringConvert.h"

#ifdef FORMAT_7Z
#include "../Format/7z/Handler.h"
#endif

#ifdef FORMAT_BZIP2
#include "../Format/BZip2/Handler.h"
#endif

#ifdef FORMAT_GZIP
#include "../Format/GZip/Handler.h"
#endif

#ifdef FORMAT_TAR
#include "../Format/Tar/Handler.h"
#endif

#ifdef FORMAT_ZIP
#include "../Format/Zip/Handler.h"
#endif


using namespace NWindows;

const UINT64 kMaxCheckStartPosition = 1 << 20;

HRESULT ReOpenArchive(IArchiveHandler200 *archiveHandler, 
    const CSysString &fileName)
{
  #ifndef EXCLUDE_COM
  NCOM::CComInitializer comInitializer; // test it
  #endif
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStream(inStreamSpec);
  inStreamSpec->Open(fileName);
  return archiveHandler->Open(inStream, &kMaxCheckStartPosition, NULL);
}

HRESULT OpenArchive(const CSysString &fileName, 
    IArchiveHandler200 **archiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult,
    IOpenArchive2CallBack *openArchive2CallBack)
{
  #ifndef EXCLUDE_COM
  NCOM::CComInitializer comInitializer;
  #endif
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(fileName))
    return E_FAIL;

  /*
  #ifdef FORMAT_7Z

  CComObjectNoLock<NArchive::N7z::CHandler> *aHandlerSpec = new CComObjectNoLock<NArchive::N7z::CHandler>;
  CComPtr<IArchiveHandler200> archiveHandler = aHandlerSpec;
  inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
  RETURN_IF_NOT_S_OK(archiveHandler->Open(inStream, 
      &kMaxCheckStartPosition, openArchive2CallBack));
  *archiveHandlerResult = archiveHandler.Detach();
  archiverInfoResult.Name = TEXT("7z");
  archiverInfoResult.Extension = TEXT("7z");
  archiverInfoResult.KeepName = false;
  archiverInfoResult.UpdateEnabled = true;
  return S_OK;
  
  #else
 
  #ifndef EXCLUDE_COM
  */
  *archiveHandlerResult = NULL;
  CObjectVector<NZipRootRegistry::CArchiverInfo> archiverInfoList;
  NZipRootRegistry::ReadArchiverInfoList(archiverInfoList);
  CSysString extension;
  {
    CSysString aName, aPureName, aDot;
    if(!NFile::NDirectory::GetOnlyName(fileName, aName))
      return E_FAIL;
    NFile::NName::SplitNameToPureNameAndExtension(aName, aPureName, aDot, extension);
  }
  std::vector<int> orderIndexes;
  int firstArchiverIndex;
  for(firstArchiverIndex = 0; 
      firstArchiverIndex < archiverInfoList.Size(); firstArchiverIndex++)
    if(extension.CollateNoCase(archiverInfoList[firstArchiverIndex].Extension) == 0)
      break;
  if(firstArchiverIndex < archiverInfoList.Size())
    orderIndexes.push_back(firstArchiverIndex);
  for(int j = 0; j < archiverInfoList.Size(); j++)
    if(j != firstArchiverIndex)
      orderIndexes.push_back(j);
  
  HRESULT badResult = S_OK;
  for(int i = 0; i < orderIndexes.size(); i++)
  {
    inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const NZipRootRegistry::CArchiverInfo &archiverInfo = 
        archiverInfoList[orderIndexes[i]];
    CComPtr<IArchiveHandler200> archiveHandler;

    #ifdef FORMAT_7Z
    if (archiverInfo.Name.CompareNoCase(TEXT("7z")) == 0)
      archiveHandler = new CComObjectNoLock<NArchive::N7z::CHandler>;
    #endif

    #ifdef FORMAT_BZIP2
    if (archiverInfo.Name.CompareNoCase(TEXT("BZip2")) == 0)
      archiveHandler = new CComObjectNoLock<NArchive::NBZip2::CHandler>;
    #endif

    #ifdef FORMAT_GZIP
    if (archiverInfo.Name.CompareNoCase(TEXT("GZip")) == 0)
      archiveHandler = new CComObjectNoLock<NArchive::NGZip::CGZipHandler>;
    #endif

    #ifdef FORMAT_TAR
    if (archiverInfo.Name.CompareNoCase(TEXT("Tar")) == 0)
      archiveHandler = new CComObjectNoLock<NArchive::NTar::CTarHandler>;
    #endif

    #ifdef FORMAT_ZIP
    if (archiverInfo.Name.CompareNoCase(TEXT("Zip")) == 0)
      archiveHandler = new CComObjectNoLock<NArchive::NZip::CZipHandler>;
    #endif


    #ifndef EXCLUDE_COM
    if (!archiveHandler)
    {
      HRESULT result = archiveHandler.CoCreateInstance(archiverInfo.ClassID);
      if (result != S_OK)
        continue;
    }
    #endif EXCLUDE_COM
    
    if (!archiveHandler)
      return E_FAIL;
    
    HRESULT result = archiveHandler->Open(inStream, &kMaxCheckStartPosition, openArchive2CallBack);
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
  return S_FALSE;
  
  /*
  #else
  return S_FALSE;
  #endif
  
  #endif
  */
}

HRESULT OpenArchive(const CSysString &fileName, 
    IArchiveHandler200 **archiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult)
{
  return OpenArchive(fileName, archiveHandlerResult, archiverInfoResult, NULL);
}
