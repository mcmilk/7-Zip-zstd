// arj/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"
#include "Interface/StreamObjects.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"

#include "Archive/Common/ItemNameUtils.h"

#include "Common/Defs.h"
#include "Common/CRC.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"
#include "Interface/StreamObjects.h"
#include "Interface/LimitedStreams.h"

#include "../Common/OutStreamWithCRC.h"
// #include "../Common/CoderMixer.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"
#include "../Common/FormatCryptoInterface.h"


#include "../../../Compress/LZ/arj/Decoder1.h"
#include "../../../Compress/LZ/arj/Decoder2.h"

#ifdef CRYPTO_arj
#include "../../../Crypto/Cipher/arj/Coder.h"
#else
/* {23170F69-40C1-278A-1000-000250030000}
DEFINE_GUID(CLSID_CCryptoZipDecoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x03, 0x00, 0x00);
*/
#endif

using namespace std;

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace Narj{

const wchar_t *kHostOS[] = 
{
  L"MSDOS",
  L"PRIMOS",
  L"Unix",
  L"AMIGA",
  L"Mac",
  L"OS/2",
  L"APPLE GS",
  L"Atari ST",
  L"Next",
  L"VAX VMS",
  L"WIN95"
};


const kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

const wchar_t *kUnknownOS = L"Unknown";


/*
enum // PropID
{
  kaipidHostOS = kaipidUserDefined,
  kaipidUnPackVersion, 
  kaipidMethod, 
};
*/

STATPROPSTG kProperties[] = 
{
  { NULL, kaipidPath, VT_BSTR},
  { NULL, kaipidIsFolder, VT_BOOL},
  { NULL, kaipidSize, VT_UI8},
  { NULL, kaipidPackedSize, VT_UI8},
  { NULL, kaipidLastWriteTime, VT_FILETIME},
  { NULL, kaipidAttributes, VT_UI4},

  { NULL, kaipidEncrypted, VT_BOOL},
  // { NULL, kaipidComment, VT_BOOL},
    
  { NULL, kaipidCRC, VT_UI4},

  { NULL, kaipidMethod, VT_UI1},
  { NULL, kaipidHostOS, VT_BSTR}

  // { L"UnPack Version", kaipidUnPackVersion, VT_UI1},
  // { L"Method", kaipidMethod, VT_UI1},
  // { L"Host OS", kaipidHostOS, VT_BSTR}
};

static const kNumProperties = sizeof(kProperties) / sizeof(kProperties[0]);

class CEnumArchiveItemProperty:
  public IEnumSTATPROPSTG,
  public CComObjectRoot
{
public:
  int m_Index;

  BEGIN_COM_MAP(CEnumArchiveItemProperty)
    COM_INTERFACE_ENTRY(IEnumSTATPROPSTG)
  END_COM_MAP()
    
  DECLARE_NOT_AGGREGATABLE(CEnumArchiveItemProperty)
    
  DECLARE_NO_REGISTRY()
public:
  CEnumArchiveItemProperty(): m_Index(0) {};

  STDMETHOD(Next) (ULONG aNumItems, STATPROPSTG *anItems, ULONG *aNumFetched);
  STDMETHOD(Skip)  (ULONG aNumItems);
  STDMETHOD(Reset) ();
  STDMETHOD(Clone) (IEnumSTATPROPSTG **anEnum);
};

STDMETHODIMP CEnumArchiveItemProperty::Reset()
{
  m_Index = 0;
  return S_OK;
}

STDMETHODIMP CEnumArchiveItemProperty::Next(ULONG aNumItems, 
    STATPROPSTG *anItems, ULONG *aNumFetched)
{
  COM_TRY_BEGIN
  HRESULT aResult = S_OK;
  if(aNumItems > 1 && !aNumFetched)
    return E_INVALIDARG;

  for(DWORD anIndex = 0; anIndex < aNumItems; anIndex++, m_Index++)
  {
    if(m_Index >= kNumProperties)
    {
      aResult =  S_FALSE;
      break;
    }
    const STATPROPSTG &aSrcItem = kProperties[m_Index];
    STATPROPSTG &aDestItem = anItems[anIndex];
    aDestItem.propid = aSrcItem.propid;
    aDestItem.vt = aSrcItem.vt;
    if(aSrcItem.lpwstrName != NULL)
    {
      aDestItem.lpwstrName = (wchar_t *)CoTaskMemAlloc((wcslen(aSrcItem.lpwstrName) + 1) * sizeof(wchar_t));
      wcscpy(aDestItem.lpwstrName, aSrcItem.lpwstrName);
    }
    else
      aDestItem.lpwstrName = aSrcItem.lpwstrName;
  }
  if (aNumFetched)
    *aNumFetched = anIndex;
  return aResult;
  COM_TRY_END
}

STDMETHODIMP CEnumArchiveItemProperty::Skip(ULONG aNumSkip)
  {  return E_NOTIMPL; }

STDMETHODIMP CEnumArchiveItemProperty::Clone(IEnumSTATPROPSTG **anEnum)
  {  return E_NOTIMPL; }


CHandler::CHandler()
{}

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **anEnumProperty)
{
  COM_TRY_BEGIN
  CComObjectNoLock<CEnumArchiveItemProperty> *anEnumObject = 
      new CComObjectNoLock<CEnumArchiveItemProperty>;
  if (anEnumObject == NULL)
    return E_OUTOFMEMORY;
  CComPtr<IEnumSTATPROPSTG> anEnum(anEnumObject);
  // ((CComObjectNoLock<CTestEnumIDList>*)(anEnumObject))->Init(this, m_IDList, aFlags); // TODO : Add any addl. params as needed
  return anEnum->QueryInterface(IID_IEnumSTATPROPSTG, (LPVOID*)anEnumProperty);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = m_Items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UINT32 anIndex, PROPID aPropID,  PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;
  const CItemInfoEx &anItem = m_Items[anIndex];
  switch(aPropID)
  {
    case kaipidPath:
      aPropVariant = 
      NItemName::GetOSName(MultiByteToUnicodeString(anItem.Name, CP_OEMCP));
      /*
                     NItemName::GetOSName2(
          MultiByteToUnicodeString(anItem.Name, anItem.GetCodePage()));
          */
      break;
    case kaipidIsFolder:
      aPropVariant = anItem.IsDirectory();
      break;
    case kaipidSize:
      aPropVariant = anItem.Size;
      break;
    case kaipidPackedSize:
      aPropVariant = anItem.PackSize;
      break;
    case kaipidLastWriteTime:
    {
      FILETIME aLocalFileTime, anUTCFileTime;
      if (DosTimeToFileTime(anItem.ModifiedTime, aLocalFileTime))
      {
        if (!LocalFileTimeToFileTime(&aLocalFileTime, &anUTCFileTime))
          anUTCFileTime.dwHighDateTime = anUTCFileTime.dwLowDateTime = 0;
      }
      else
        anUTCFileTime.dwHighDateTime = anUTCFileTime.dwLowDateTime = 0;
      aPropVariant = anUTCFileTime;
      break;
    }
    case kaipidAttributes:
      aPropVariant = anItem.GetWinAttributes();
      break;
    case kaipidEncrypted:
      aPropVariant = anItem.IsEncrypted();
      break;
    /*
    case kaipidComment:
      aPropVariant = anItem.IsCommented();
      break;
    */
    case kaipidCRC:
      aPropVariant = anItem.FileCRC;
      break;
    case kaipidMethod:
      aPropVariant = anItem.Method;
      break;
    case kaipidHostOS:
      aPropVariant = (anItem.HostOS < kNumHostOSes) ?
        (kHostOS[anItem.HostOS]) : kUnknownOS;
      break;
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

class CPropgressImp: public CProgressVirt
{
  CComPtr<IOpenArchive2CallBack> m_OpenArchiveCallBack;
public:
  STDMETHOD(SetCompleted)(const UINT64 *aNumFiles);
  void Init(IOpenArchive2CallBack *anOpenArchiveCallBack)
    { m_OpenArchiveCallBack = anOpenArchiveCallBack; }
};

STDMETHODIMP CPropgressImp::SetCompleted(const UINT64 *aNumFiles)
{
  if (m_OpenArchiveCallBack)
    return m_OpenArchiveCallBack->SetCompleted(aNumFiles, NULL);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition, IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  try
  {
    m_Items.Clear();
    CInArchive anArchive;
    if(!anArchive.Open(aStream, aMaxCheckStartPosition))
      return S_FALSE;
    if (anOpenArchiveCallBack != NULL)
    {
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetTotal(NULL, NULL));
      UINT64 aNumFiles = m_Items.Size();
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
    }
    while(true)
    {
      CItemInfoEx anItemInfo;
      bool aFilled;
      HRESULT aResult = anArchive.GetNextItem(aFilled, anItemInfo);
      if (aResult == S_FALSE)
        return S_FALSE;
      if (aResult != S_OK)
        return S_FALSE;
      if (!aFilled)
        break;
      m_Items.Add(anItemInfo);
      anArchive.IncreaseRealPosition(anItemInfo.PackSize);
      if (anOpenArchiveCallBack != NULL)
      {
        UINT64 aNumFiles = m_Items.Size();
        RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
      }
    }
    m_Stream = aStream;
  }
  catch(...)
  {
    return S_FALSE;
  }
  COM_TRY_END
  return S_OK;
}

STDMETHODIMP CHandler::Close()
{
  m_Stream.Release();
  return S_OK;
}



//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  CComPtr<ICryptoGetTextPassword> aGetTextPassword;
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
  UINT64 aTotalUnPacked = 0, aTotalPacked = 0;
  if(aNumItems == 0)
    return S_OK;
  for(int i = 0; i < aNumItems; i++)
  {
    const CItemInfoEx &anItemInfo = m_Items[anIndexes[i]];
    aTotalUnPacked += anItemInfo.Size;
    aTotalPacked += anItemInfo.PackSize;
  }
  anExtractCallBack->SetTotal(aTotalUnPacked);

  UINT64 aCurrentTotalUnPacked = 0, aCurrentTotalPacked = 0;
  UINT64 aCurrentItemUnPacked, aCurrentItemPacked;
  
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = NULL;
  CComPtr<ICompressCoder> anArj1Decoder;
  CComPtr<ICompressCoder> anArj2Decoder;
  CComPtr<ICompressCoder> aCopyCoder;
  CComPtr<ICompressCoder> aCryptoDecoder;
  // CComObjectNoLock<CCoderMixer> *aMixerCoderSpec;
  // CComPtr<ICompressCoder> aMixerCoder;

  // UINT16 aMixerCoderMethod;

  for(i = 0; i < aNumItems; i++, aCurrentTotalUnPacked += aCurrentItemUnPacked,
      aCurrentTotalPacked += aCurrentItemPacked)
  {
    aCurrentItemUnPacked = 0;
    aCurrentItemPacked = 0;

    RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalUnPacked));
    CComPtr<ISequentialOutStream> aRealOutStream;
    INT32 anAskMode;
    anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
        NArchiveHandler::NExtract::NAskMode::kExtract;
    INT32 anIndex = anIndexes[i];
    const CItemInfoEx &anItemInfo = m_Items[anIndex];
    RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(anIndex, &aRealOutStream, anAskMode));

    if(anItemInfo.IsDirectory())
    {
      // if (!aTestMode)
      {
        RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
      }
      continue;
    }

    if (!aTestMode && (!aRealOutStream)) 
      continue;

    RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
    aCurrentItemUnPacked = anItemInfo.Size;
    aCurrentItemPacked = anItemInfo.PackSize;

    {
      CComObjectNoLock<COutStreamWithCRC> *anOutStreamSpec = 
        new CComObjectNoLock<COutStreamWithCRC>;
      CComPtr<ISequentialOutStream> anOutStream(anOutStreamSpec);
      anOutStreamSpec->Init(aRealOutStream);
      aRealOutStream.Release();
      
      CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
          CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<ISequentialInStream> anInStream(aStreamSpec);
      
      UINT64 aPos;
      m_Stream->Seek(anItemInfo.DataPosition, STREAM_SEEK_SET, &aPos);

      aStreamSpec->Init(m_Stream, anItemInfo.PackSize);


      CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
      aLocalProgressSpec->Init(anExtractCallBack, false);


      CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
      aLocalCompressProgressSpec->Init(aProgress, 
          &aCurrentTotalPacked,
          &aCurrentTotalUnPacked);

      if (anItemInfo.IsEncrypted())
      {
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
          NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
        /*
        if (!aCryptoDecoder)
        {
          #ifdef CRYPTO_ZIP
          aCryptoDecoder = new CComObjectNoLock<NCrypto::NZip::CDecoder>;
          #else
          RETURN_IF_NOT_S_OK(aCryptoDecoder.CoCreateInstance(CLSID_CCryptoZipDecoder));
          #endif
        }
        CComPtr<ICryptoSetPassword> aCryptoSetPassword;
        RETURN_IF_NOT_S_OK(aCryptoDecoder.QueryInterface(&aCryptoSetPassword));

        if (!aGetTextPassword)
          anExtractCallBack.QueryInterface(&aGetTextPassword);

        if (aGetTextPassword)
        {
          CComBSTR aPassword;
          RETURN_IF_NOT_S_OK(aGetTextPassword->CryptoGetTextPassword(&aPassword));
          AString anOemPassword = UnicodeStringToMultiByte(
              (const wchar_t *)aPassword, CP_OEMCP);
          RETURN_IF_NOT_S_OK(aCryptoSetPassword->CryptoSetPassword(
              (const BYTE *)(const char *)anOemPassword, anOemPassword.Length()));
        }
        else
        {
          RETURN_IF_NOT_S_OK(aCryptoSetPassword->CryptoSetPassword(0, 0));
        }
        */
      }

      HRESULT aResult;

      switch(anItemInfo.Method)
      {
        case NFileHeader::NCompressionMethod::kStored:
          {
            if(aCopyCoderSpec == NULL)
            {
              aCopyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
              aCopyCoder = aCopyCoderSpec;
            }
            try
            {
              if (anItemInfo.IsEncrypted())
              {
                RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
                continue;
              }
              else
              {
                aResult = aCopyCoder->Code(anInStream, anOutStream,
                    NULL, NULL, aCompressProgress);
              }
              if (aResult == S_FALSE)
                throw "data error";
              if (aResult != S_OK)
                return aResult;
            }
            catch(...)
            {
              anOutStream.Release();
              RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        case NFileHeader::NCompressionMethod::kCompressed1a:
        case NFileHeader::NCompressionMethod::kCompressed1b:
        case NFileHeader::NCompressionMethod::kCompressed1c:
          {
            if(!anArj1Decoder)
            {
              anArj1Decoder = new CComObjectNoLock<NCompress::Narj::NDecoder1::CCoder>;
            }
            try
            {
              if (anItemInfo.IsEncrypted())
              {
                RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
                continue;
              }
              else
              {
                aResult = anArj1Decoder->Code(anInStream, anOutStream,
                    NULL, &aCurrentItemUnPacked, aCompressProgress);
              }
              if (aResult == S_FALSE)
                throw "data error";
              if (aResult != S_OK)
                return aResult;
            }
            catch(...)
            {
              anOutStream.Release();
              RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        case NFileHeader::NCompressionMethod::kCompressed2:
          {
            if(!anArj2Decoder)
            {
              anArj2Decoder = new CComObjectNoLock<NCompress::Narj::NDecoder2::CCoder>;
            }
            try
            {
              if (anItemInfo.IsEncrypted())
              {
                RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
                continue;
              }
              else
              {
                aResult = anArj2Decoder->Code(anInStream, anOutStream,
                    NULL, &aCurrentItemUnPacked, aCompressProgress);
              }
              if (aResult == S_FALSE)
                throw "data error";
              if (aResult != S_OK)
                return aResult;
            }
            catch(...)
            {
              anOutStream.Release();
              RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        default:
            RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
            continue;
      }
      bool aCRC_Ok = anOutStreamSpec->GetCRC() == anItemInfo.FileCRC;
      anOutStream.Release();
      if(aCRC_Ok)
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK))
      else
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kCRCError))
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 aTestMode,
      IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> anIndexes;
  for(int i = 0; i < m_Items.Size(); i++)
    anIndexes.Add(i);
  return Extract(&anIndexes.Front(), m_Items.Size(), aTestMode, anExtractCallBack);
  COM_TRY_END
}

}}
