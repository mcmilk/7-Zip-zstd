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

static bool ParseNumberString(const UString &aString, UINT32 &aNumber)
{
  wchar_t *anEndPtr;
  aNumber = wcstoul(aString, &anEndPtr, 10);
  return (anEndPtr - aString == aString.Length());
}


static HRESULT CopyBlock(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream)
{
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> aCopyCoder = aCopyCoderSpec;
  return aCopyCoder->Code(anInStream, anOutStream, NULL, NULL, NULL);
}

HRESULT Compress(
    const CActionSet &anActionSet, 
    IArchiveHandler200 *anArchive,
    const CCompressionMethodMode &aCompressionMethod,
    const CSysString &anArchiveName, 
    const CArchiveItemInfoVector &anArchiveItems,
    const CArchiveStyleDirItemInfoVector &aDirItems,
    bool anEnablePercents,
    bool aSfxMode,
    const CSysString &aSfxModule)
{
  CComPtr<IOutArchiveHandler200> anOutArchive;
  if(anArchive != NULL)
  {
    CComPtr<IArchiveHandler200> anArchive2 = anArchive;
    HRESULT aResult = anArchive2.QueryInterface(&anOutArchive);
    if(aResult != S_OK)
    {
      throw "update operations are not supported for this archive";
    }
  }
  else
  {
    #ifndef EXCLUDE_COM
    HRESULT aResult = anOutArchive.CoCreateInstance(aCompressionMethod.ClassID);
    if (aResult != S_OK)
    {
      throw "update operations are not supported for this archive";
      return E_FAIL;
    }
    #endif

    #ifdef FORMAT_7Z
    if (aCompressionMethod.Name.CompareNoCase(TEXT("7z")) == 0)
      anOutArchive = new CComObjectNoLock<NArchive::N7z::CHandler>;
    #endif

    #ifdef FORMAT_BZIP2
    if (aCompressionMethod.Name.CompareNoCase(TEXT("BZip2")) == 0)
      anOutArchive = new CComObjectNoLock<NArchive::NBZip2::CHandler>;
    #endif

    #ifdef FORMAT_GZIP
    if (aCompressionMethod.Name.CompareNoCase(TEXT("GZip")) == 0)
      anOutArchive = new CComObjectNoLock<NArchive::NGZip::CGZipHandler>;
    #endif

    #ifdef FORMAT_TAR
    if (aCompressionMethod.Name.CompareNoCase(TEXT("Tar")) == 0)
      anOutArchive = new CComObjectNoLock<NArchive::NTar::CTarHandler>;
    #endif
    
    #ifdef FORMAT_ZIP
    if (aCompressionMethod.Name.CompareNoCase(TEXT("Zip")) == 0)
      anOutArchive = new CComObjectNoLock<NArchive::NZip::CZipHandler>;
    #endif


    if (anOutArchive == 0)
    {
      throw "update operations are not supported for this archive";
      return E_FAIL;
    }
  }
  
  NFileTimeType::EEnum aFileTimeType;
  UINT32 aValue;
  RETURN_IF_NOT_S_OK(anOutArchive->GetFileTimeType(&aValue));

  switch(aValue)
  {
    case NFileTimeType::kWindows:
    case NFileTimeType::kDOS:
    case NFileTimeType::kUnix:
      aFileTimeType = NFileTimeType::EEnum(aValue);
      break;
    default:
      return E_FAIL;
  }

  CUpdatePairInfoVector anUpdatePairs;
  GetUpdatePairInfoList(aDirItems, anArchiveItems, aFileTimeType, anUpdatePairs); // must be done only once!!!
  
  CUpdatePairInfo2Vector anOperationChain;
  UpdateProduce(aDirItems, anArchiveItems, anUpdatePairs, anActionSet,
      anOperationChain);
  
  CComObjectNoLock<CUpdateCallBackImp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBackImp>;
  CComPtr<IUpdateCallBack> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(&aDirItems, &anArchiveItems, &anOperationChain, anEnablePercents);
  
  CComObjectNoLock<COutFileStream> *anOutStreamSpec =
    new CComObjectNoLock<COutFileStream>;
  CComPtr<IOutStream> anOutStream(anOutStreamSpec);

  {
    CSysString aResultPath;
    int aPos;
    if(! NFile::NDirectory::MyGetFullPathName(anArchiveName, aResultPath, aPos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(aResultPath.Left(aPos));
  }
  if (!anOutStreamSpec->Open(anArchiveName))
  {
    CSysString aMessage;
    NError::MyFormatMessage(::GetLastError(), aMessage);
    g_StdOut << GetOemString(aMessage) << endl;
    return E_FAIL;
  }

  CComPtr<ISetProperties> aSetProperties;
  if (anOutArchive.QueryInterface(&aSetProperties) == S_OK)
  {
    CObjectVector<CComBSTR> aNamesReal;
    std::vector<CPropVariant> aValues;
    for(int i = 0; i < aCompressionMethod.Properties.Size(); i++)
    {
      const CProperty &aProperty = aCompressionMethod.Properties[i];
      NCOM::CPropVariant aPropVariant;
      UINT32 aNumber;
      if (!aProperty.Value.IsEmpty())
      {
        if (ParseNumberString(aProperty.Value, aNumber))
          aPropVariant = aNumber;
        else
          aPropVariant = aProperty.Value;
      }
      CComBSTR aComBSTR = aProperty.Name;
      aNamesReal.Add(aComBSTR);
      aValues.push_back(aPropVariant);
    }
    std::vector<BSTR> aNames;
    for(i = 0; i < aNamesReal.Size(); i++)
      aNames.push_back(aNamesReal[i]);
 
    RETURN_IF_NOT_S_OK(aSetProperties->SetProperties(&aNames.front(), 
       &aValues.front(), aNames.size()));
  }


  if (aSfxMode)
  {
    CComObjectNoLock<CInFileStream> *aSFXStreamSpec = new CComObjectNoLock<CInFileStream>;
    CComPtr<IInStream> aSFXStream(aSFXStreamSpec);
    if (!aSFXStreamSpec->Open(aSfxModule))
      throw "Can't open sfx module";
    RETURN_IF_NOT_S_OK(CopyBlock(aSFXStream, anOutStream));
  }

  return anOutArchive->UpdateItems(anOutStream, anOperationChain.Size(),
     anUpdateCallBack);
}
