// OpenArchive.cpp

#include "StdAfx.h"

#include "OpenArchive.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"

#include "../../Common/FileStreams.h"

#include "Common/StringConvert.h"

#ifdef FORMAT_7Z
#include "../../Archive/7z/7zHandler.h"
#endif

#ifdef FORMAT_BZIP2
#include "../../Archive/BZip2/BZip2Handler.h"
#endif

#ifdef FORMAT_GZIP
#include "../../Archive/GZip/GZipHandler.h"
#endif

#ifdef FORMAT_TAR
#include "../../Archive/Tar/TarHandler.h"
#endif

#ifdef FORMAT_ZIP
#include "../../Archive/Zip/ZipHandler.h"
#endif

#ifndef EXCLUDE_COM
#include "HandlerLoader.h"
#endif


using namespace NWindows;

const UINT64 kMaxCheckStartPosition = 1 << 20;

HRESULT ReOpenArchive(IInArchive *archive, 
    const UString &fileName)
{
  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream(inStreamSpec);
  inStreamSpec->Open(fileName);
  return archive->Open(inStream, &kMaxCheckStartPosition, NULL);
}

HRESULT OpenArchive(const UString &fileName, 
    #ifndef EXCLUDE_COM
    HMODULE *module,
    #endif
    IInArchive **archiveResult, 
    CArchiverInfo &archiverInfoResult,
    int &subExtIndex,
    IArchiveOpenCallback *openArchiveCallback)
{
  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(fileName))
    return GetLastError();

  *archiveResult = NULL;
  CObjectVector<CArchiverInfo> archiverInfoList;
  ReadArchiverInfoList(archiverInfoList);
  UString extension;
  {
    UString name, pureName, dot;
    if(!NFile::NDirectory::GetOnlyName(fileName, name))
      return E_FAIL;
    NFile::NName::SplitNameToPureNameAndExtension(name, pureName, dot, extension);
  }
  CIntVector orderIndices;
  int firstArchiverIndex;
  for(firstArchiverIndex = 0; 
      firstArchiverIndex < archiverInfoList.Size(); firstArchiverIndex++)
  {
    int subIndex = archiverInfoList[firstArchiverIndex].FindExtension(extension);
    if (subIndex >= 0)
      break;
  }
  if(firstArchiverIndex < archiverInfoList.Size())
    orderIndices.Add(firstArchiverIndex);
  for(int j = 0; j < archiverInfoList.Size(); j++)
    if(j != firstArchiverIndex)
      orderIndices.Add(j);
  
  HRESULT badResult = S_OK;
  for(int i = 0; i < orderIndices.Size(); i++)
  {
    inStreamSpec->Seek(0, STREAM_SEEK_SET, NULL);
    const CArchiverInfo &archiverInfo = archiverInfoList[orderIndices[i]];
    #ifndef EXCLUDE_COM
    CHandlerLoader loader;
    #endif
    CMyComPtr<IInArchive> archive;

    #ifdef FORMAT_7Z
    if (archiverInfo.Name.CompareNoCase(L"7z") == 0)
      archive = new NArchive::N7z::CHandler;
    #endif

    #ifdef FORMAT_BZIP2
    if (archiverInfo.Name.CompareNoCase(L"BZip2") == 0)
      archive = new NArchive::NBZip2::CHandler;
    #endif

    #ifdef FORMAT_GZIP
    if (archiverInfo.Name.CompareNoCase(L"GZip") == 0)
      archive = new NArchive::NGZip::CHandler;
    #endif

    #ifdef FORMAT_TAR
    if (archiverInfo.Name.CompareNoCase(L"Tar") == 0)
      archive = new NArchive::NTar::CHandler;
    #endif

    #ifdef FORMAT_ZIP
    if (archiverInfo.Name.CompareNoCase(L"Zip") == 0)
      archive = new NArchive::NZip::CHandler;
    #endif


    #ifndef EXCLUDE_COM
    if (!archive)
    {
      HRESULT result = loader.CreateHandler(archiverInfo.FilePath, 
          archiverInfo.ClassID, (void **)&archive, false);
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
    #ifndef EXCLUDE_COM
    *module = loader.Detach();
    #endif
    archiverInfoResult = archiverInfo;
    subExtIndex = archiverInfo.FindExtension(extension);
    if (subExtIndex < 0)
      subExtIndex = 0;
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
