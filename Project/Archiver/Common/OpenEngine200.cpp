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

HRESULT ReOpenArchive(IArchiveHandler200 *anArchiveHandler, 
    const CSysString &aFileName)
{
  NCOM::CComInitializer aComInitializer; // test it
  CComObjectNoLock<CInFileStream> *anInStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> anInStream(anInStreamSpec);
  anInStreamSpec->Open(aFileName);
  return anArchiveHandler->Open(anInStream, &kMaxCheckStartPosition, NULL);
}

HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler200 **anArchiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult,
    IOpenArchive2CallBack *anOpenArchive2CallBack)
{
  NCOM::CComInitializer aComInitializer;
  CComObjectNoLock<CInFileStream> *anInStreamSpec = new 
    CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> anInStream(anInStreamSpec);
  if (!anInStreamSpec->Open(aFileName))
    return E_FAIL;

  /*
  #ifdef FORMAT_7Z

  CComObjectNoLock<NArchive::N7z::CHandler> *aHandlerSpec = new CComObjectNoLock<NArchive::N7z::CHandler>;
  CComPtr<IArchiveHandler200> anArchiveHandler = aHandlerSpec;
  anInStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
  RETURN_IF_NOT_S_OK(anArchiveHandler->Open(anInStream, 
      &kMaxCheckStartPosition, anOpenArchive2CallBack));
  *anArchiveHandlerResult = anArchiveHandler.Detach();
  anArchiverInfoResult.Name = TEXT("7z");
  anArchiverInfoResult.Extension = TEXT("7z");
  anArchiverInfoResult.KeepName = false;
  anArchiverInfoResult.UpdateEnabled = true;
  return S_OK;
  
  #else
 
  #ifndef EXCLUDE_COM
  */
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
  
  for(int i = 0; i < anOrderIndexes.size(); i++)
  {
    anInStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
        anArchiverInfoList[anOrderIndexes[i]];
    CComPtr<IArchiveHandler200> anArchiveHandler;

    #ifdef FORMAT_7Z
    if (anArchiverInfo.Name.CompareNoCase("7z") == 0)
      anArchiveHandler = new CComObjectNoLock<NArchive::N7z::CHandler>;
    #endif

    #ifdef FORMAT_BZIP2
    if (anArchiverInfo.Name.CompareNoCase("BZip2") == 0)
      anArchiveHandler = new CComObjectNoLock<NArchive::NBZip2::CHandler>;
    #endif

    #ifdef FORMAT_GZIP
    if (anArchiverInfo.Name.CompareNoCase("GZip") == 0)
      anArchiveHandler = new CComObjectNoLock<NArchive::NGZip::CGZipHandler>;
    #endif

    #ifdef FORMAT_TAR
    if (anArchiverInfo.Name.CompareNoCase("Tar") == 0)
      anArchiveHandler = new CComObjectNoLock<NArchive::NTar::CTarHandler>;
    #endif

    #ifdef FORMAT_ZIP
    if (anArchiverInfo.Name.CompareNoCase("Zip") == 0)
      anArchiveHandler = new CComObjectNoLock<NArchive::NZip::CZipHandler>;
    #endif


    #ifndef EXCLUDE_COM
    if (!anArchiveHandler)
    {
      HRESULT aResult = anArchiveHandler.CoCreateInstance(anArchiverInfo.ClassID);
      if (aResult != S_OK)
        continue;
    }
    #endif EXCLUDE_COM
    
    if (!anArchiveHandler)
      return E_FAIL;
    
    HRESULT aResult = anArchiveHandler->Open(anInStream, &kMaxCheckStartPosition, anOpenArchive2CallBack);
    if(aResult == S_FALSE)
      continue;
    if(aResult != S_OK)
    {
      return aResult;
    }
    *anArchiveHandlerResult = anArchiveHandler.Detach();
    anArchiverInfoResult = anArchiverInfo;
    return S_OK;
  }
  return S_FALSE;
  
  /*
  #else
  return S_FALSE;
  #endif
  
  #endif
  */
}

HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler200 **anArchiveHandlerResult, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult)
{
  return OpenArchive(aFileName, anArchiveHandlerResult, anArchiverInfoResult, NULL);
}
