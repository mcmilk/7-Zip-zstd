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

HRESULT ReOpenArchive(IInArchive *archive, 
    const CSysString &fileName)
{
  #ifndef EXCLUDE_COM
  NCOM::CComInitializer comInitializer; // test it
  #endif
  CComObjectNoLock<CInFileStream> *inStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStream(inStreamSpec);
  inStreamSpec->Open(fileName);
  return archive->Open(inStream, &kMaxCheckStartPosition, NULL);
}

HRESULT OpenArchive(const CSysString &fileName, 
    IInArchive **archiveResult, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult,
    IArchiveOpenCallback *openArchiveCallback)
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
  CComPtr<IArchiveHandler200> archive = aHandlerSpec;
  inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
  RETURN_IF_NOT_S_OK(archive->Open(inStream, 
      &kMaxCheckStartPosition, openArchiveCallback));
  *archiveResult = archive.Detach();
  archiverInfoResult.Name = TEXT("7z");
  archiverInfoResult.Extension = TEXT("7z");
  archiverInfoResult.KeepName = false;
  archiverInfoResult.UpdateEnabled = true;
  return S_OK;
  
  #else
 
  #ifndef EXCLUDE_COM
  */
  *archiveResult = NULL;
  CObjectVector<NZipRootRegistry::CArchiverInfo> archiverInfoList;
  NZipRootRegistry::ReadArchiverInfoList(archiverInfoList);
  CSysString extension;
  {
    CSysString aName, aPureName, aDot;
    if(!NFile::NDirectory::GetOnlyName(fileName, aName))
      return E_FAIL;
    NFile::NName::SplitNameToPureNameAndExtension(aName, aPureName, aDot, extension);
  }
  CIntVector orderIndices;
  int firstArchiverIndex;
  for(firstArchiverIndex = 0; 
      firstArchiverIndex < archiverInfoList.Size(); firstArchiverIndex++)
    if(extension.CollateNoCase(archiverInfoList[firstArchiverIndex].Extension) == 0)
      break;
  if(firstArchiverIndex < archiverInfoList.Size())
    orderIndices.Add(firstArchiverIndex);
  for(int j = 0; j < archiverInfoList.Size(); j++)
    if(j != firstArchiverIndex)
      orderIndices.Add(j);
  
  HRESULT badResult = S_OK;
  for(int i = 0; i < orderIndices.Size(); i++)
  {
    inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const NZipRootRegistry::CArchiverInfo &archiverInfo = 
        archiverInfoList[orderIndices[i]];
    CComPtr<IInArchive> archive;

    #ifdef FORMAT_7Z
    if (archiverInfo.Name.CompareNoCase(TEXT("7z")) == 0)
      archive = new CComObjectNoLock<NArchive::N7z::CHandler>;
    #endif

    #ifdef FORMAT_BZIP2
    if (archiverInfo.Name.CompareNoCase(TEXT("BZip2")) == 0)
      archive = new CComObjectNoLock<NArchive::NBZip2::CHandler>;
    #endif

    #ifdef FORMAT_GZIP
    if (archiverInfo.Name.CompareNoCase(TEXT("GZip")) == 0)
      archive = new CComObjectNoLock<NArchive::NGZip::CGZipHandler>;
    #endif

    #ifdef FORMAT_TAR
    if (archiverInfo.Name.CompareNoCase(TEXT("Tar")) == 0)
      archive = new CComObjectNoLock<NArchive::NTar::CTarHandler>;
    #endif

    #ifdef FORMAT_ZIP
    if (archiverInfo.Name.CompareNoCase(TEXT("Zip")) == 0)
      archive = new CComObjectNoLock<NArchive::NZip::CZipHandler>;
    #endif


    #ifndef EXCLUDE_COM
    if (!archive)
    {
      HRESULT result = archive.CoCreateInstance(archiverInfo.ClassID);
      if (result != S_OK)
        continue;
    }
    #endif EXCLUDE_COM
    
    if (!archive)
      return E_FAIL;
    
    HRESULT result = archive->Open(inStream, &kMaxCheckStartPosition, openArchiveCallback);
    if(result == S_FALSE)
      continue;
    if(result != S_OK)
    {
      badResult = result;
      continue;
      // return result;
    }
    *archiveResult = archive.Detach();
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
    IInArchive **archiveResult, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult)
{
  return OpenArchive(fileName, archiveResult, archiverInfoResult, NULL);
}
