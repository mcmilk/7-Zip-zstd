// CompressEngine.h

#include "StdAfx.h"

#include "Common/StdOutStream.h"

#include "Compression/CopyCoder.h"

#include "CompressEngine.h"
#include "UpdateCallback.h"

#include "../Common/CompressEngineCommon.h"
#include "../Common/UpdateProducer.h"
#include "../Common/UpdateUtils.h"

#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"
#include "Windows/Error.h"
#include "Windows/Defs.h"

#include "Windows/PropVariantConversions.h"

#include "../../Compress/Interface/CompressInterface.h"

#include "Common/StringConvert.h"

#include "Interface/FileStreams.h"

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
using namespace NFile;
using namespace NName;
using namespace NCOM;

using namespace NUpdateArchive;

static LPCTSTR kTempArcivePrefix = _T("7zi");

static bool ParseNumberString(const UString &srcString, UINT32 &number)
{
  wchar_t *anEndPtr;
  number = wcstoul(srcString, &anEndPtr, 10);
  return (anEndPtr - srcString == srcString.Length());
}


static HRESULT CopyBlock(ISequentialInStream *inStream, ISequentialOutStream *outStream)
{
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

HRESULT Compress(
    const CActionSet &actionSet, 
    IArchiveHandler200 *archive,
    const CCompressionMethodMode &compressionMethod,
    const CSysString &archiveName, 
    const CArchiveItemInfoVector &archiveItems,
    const CArchiveStyleDirItemInfoVector &dirItems,
    bool enablePercents,
    bool sfxMode,
    const CSysString &sfxModule)
{
  CComPtr<IOutArchiveHandler200> outArchive;
  if(archive != NULL)
  {
    CComPtr<IArchiveHandler200> archive2 = archive;
    HRESULT result = archive2.QueryInterface(&outArchive);
    if(result != S_OK)
    {
      throw "update operations are not supported for this archive";
    }
  }
  else
  {
    #ifndef EXCLUDE_COM
    HRESULT result = outArchive.CoCreateInstance(compressionMethod.ClassID);
    if (result != S_OK)
    {
      throw "update operations are not supported for this archive";
      return E_FAIL;
    }
    #endif

    #ifdef FORMAT_7Z
    if (compressionMethod.Name.CompareNoCase(TEXT("7z")) == 0)
      outArchive = new CComObjectNoLock<NArchive::N7z::CHandler>;
    #endif

    #ifdef FORMAT_BZIP2
    if (compressionMethod.Name.CompareNoCase(TEXT("BZip2")) == 0)
      outArchive = new CComObjectNoLock<NArchive::NBZip2::CHandler>;
    #endif

    #ifdef FORMAT_GZIP
    if (compressionMethod.Name.CompareNoCase(TEXT("GZip")) == 0)
      outArchive = new CComObjectNoLock<NArchive::NGZip::CGZipHandler>;
    #endif

    #ifdef FORMAT_TAR
    if (compressionMethod.Name.CompareNoCase(TEXT("Tar")) == 0)
      outArchive = new CComObjectNoLock<NArchive::NTar::CTarHandler>;
    #endif
    
    #ifdef FORMAT_ZIP
    if (compressionMethod.Name.CompareNoCase(TEXT("Zip")) == 0)
      outArchive = new CComObjectNoLock<NArchive::NZip::CZipHandler>;
    #endif


    if (outArchive == 0)
    {
      throw "update operations are not supported for this archive";
      return E_FAIL;
    }
  }
  
  NFileTimeType::EEnum fileTimeType;
  UINT32 value;
  RETURN_IF_NOT_S_OK(outArchive->GetFileTimeType(&value));

  switch(value)
  {
    case NFileTimeType::kWindows:
    case NFileTimeType::kDOS:
    case NFileTimeType::kUnix:
      fileTimeType = NFileTimeType::EEnum(value);
      break;
    default:
      return E_FAIL;
  }

  CUpdatePairInfoVector updatePairs;
  GetUpdatePairInfoList(dirItems, archiveItems, fileTimeType, updatePairs); // must be done only once!!!
  
  CUpdatePairInfo2Vector operationChain;
  UpdateProduce(dirItems, archiveItems, updatePairs, actionSet,
      operationChain);
  
  CComObjectNoLock<CUpdateCallBackImp> *updateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBackImp>;
  CComPtr<IUpdateCallBack> updateCallback(updateCallBackSpec );
  
  updateCallBackSpec->Init(&dirItems, &archiveItems, &operationChain, enablePercents,
      compressionMethod.PasswordIsDefined, compressionMethod.Password, 
      compressionMethod.AskPassword);
  
  CComObjectNoLock<COutFileStream> *outStreamSpec =
    new CComObjectNoLock<COutFileStream>;
  CComPtr<IOutStream> outStream(outStreamSpec);

  {
    CSysString resultPath;
    int pos;
    if(! NFile::NDirectory::MyGetFullPathName(archiveName, resultPath, pos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(resultPath.Left(pos));
  }
  if (!outStreamSpec->Open(archiveName))
  {
    CSysString message;
    NError::MyFormatMessage(::GetLastError(), message);
    g_StdOut << GetOemString(message) << endl;
    return E_FAIL;
  }

  CComPtr<ISetProperties> setProperties;
  if (outArchive.QueryInterface(&setProperties) == S_OK)
  {
    CObjectVector<CComBSTR> realNames;
    std::vector<CPropVariant> values;
	  int i;
    for(i = 0; i < compressionMethod.Properties.Size(); i++)
    {
      const CProperty &property = compressionMethod.Properties[i];
      NCOM::CPropVariant propVariant;
      UINT32 number;
      if (!property.Value.IsEmpty())
      {
        if (ParseNumberString(property.Value, number))
          propVariant = number;
        else
          propVariant = property.Value;
      }
      CComBSTR comBSTR = property.Name;
      realNames.Add(comBSTR);
      values.push_back(propVariant);
    }
    std::vector<BSTR> names;
    for(i = 0; i < realNames.Size(); i++)
      names.push_back(realNames[i]);
 
    RETURN_IF_NOT_S_OK(setProperties->SetProperties(&names.front(), 
       &values.front(), names.size()));
  }


  if (sfxMode)
  {
    CComObjectNoLock<CInFileStream> *sfxStreamSpec = new CComObjectNoLock<CInFileStream>;
    CComPtr<IInStream> sfxStream(sfxStreamSpec);
    if (!sfxStreamSpec->Open(sfxModule))
      throw "Can't open sfx module";
    RETURN_IF_NOT_S_OK(CopyBlock(sfxStream, outStream));
  }

  return outArchive->UpdateItems(outStream, operationChain.Size(),
     updateCallback);
}
