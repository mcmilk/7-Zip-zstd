// Zip/OutHandler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/StringConvert.h"

#include "Windows/Time.h"
#include "Windows/FileFind.h"

#include "CompressionMethod.h"

#include "Compression/CopyCoder.h"

#include "UpdateEngine.h"

using namespace NArchive;
using namespace NGZip;

using namespace NWindows;
using namespace NTime;

static const kNumItemInArchive = 1;

STDMETHODIMP CGZipHandler::GetFileTimeType(UINT32 *aType)
{
  *aType = NFileTimeType::kUnix;
  return S_OK;
}

STDMETHODIMP CGZipHandler::DeleteItems(IOutStream *anOutStream, 
    const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack)
{
  return E_FAIL;
}


static HRESULT CopyStreams(IInStream *anInStream, IOutStream *anOutStream, 
    IUpdateCallBack *anUpdateCallBack)
{
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> m_CopyCoder = aCopyCoderSpec;
  return m_CopyCoder->Code(anInStream, anOutStream, NULL, NULL, NULL);
}

STDMETHODIMP CGZipHandler::UpdateItems(IOutStream *anOutStream, UINT32 aNumItems,
    IUpdateCallBack *anUpdateCallBack)
{
  if (aNumItems > 2)
    return E_FAIL;

  CItemInfo aNewItemInfo;

  bool aCompressItemDefined = false;
  bool aCopyItemDefined = false;

  int anIndexInClient;

  UINT64 aSize;
  for(UINT32 i = 0; i < aNumItems; i++)
  {
    INT32 anCompress;
    INT32 anExistInArchive;
    INT32 anIndexInServer;
    UINT32 anAttributes;
    FILETIME aTime;
    CComBSTR aName;
    RETURN_IF_NOT_S_OK(anUpdateCallBack->GetUpdateItemInfo(i,
      &anCompress, // 1 - compress 0 - copy
      &anExistInArchive,
      &anIndexInServer,
      &anAttributes,
      NULL,
      NULL,
      &aTime, 
      &aSize, 
      &aName));
    if (MyBoolToBool(anCompress))
    {
      if (aCompressItemDefined)
        return E_FAIL;
      aCompressItemDefined = true;

      if (NFile::NFind::NAttributes::IsDirectory(anAttributes))
        return E_FAIL;

      time_t anUnixTime;
      if(!FileTimeToUnixTime(aTime, anUnixTime))
        return E_FAIL;

      aNewItemInfo.Time = anUnixTime;
      /*
      if(aSize > _UI32_MAX)
        return E_FAIL;
      */
      aNewItemInfo.UnPackSize32 = (UINT32)aSize;
      aNewItemInfo.ExtraFlags = 0;
      aNewItemInfo.Flags = 0;

      aNewItemInfo.Name = UnicodeStringToMultiByte((BSTR)aName, CP_ACP);
      int aDirDelimiterPos = aNewItemInfo.Name.ReverseFind('\\');
      if (aDirDelimiterPos >= 0)
        aNewItemInfo.Name = aNewItemInfo.Name.Mid(aDirDelimiterPos + 1);

      aNewItemInfo.SetNameIsPresentFlag(!aNewItemInfo.Name.IsEmpty());
      anIndexInClient = i;
      // aNewItemInfo.IndexInClient = i;
      // aNewItemInfo.ExistInArchive = MyBoolToBool(anExistInArchive);
    }
    else
    {
      if (anIndexInServer != 0)
        return E_FAIL;
      aCopyItemDefined = true;
    }
  }

  if (aCompressItemDefined)
  {
    if (aCopyItemDefined)
      return E_FAIL;
  }
  else
  {
    if (!aCopyItemDefined)
      return E_FAIL;
    return CopyStreams(m_Stream, anOutStream, anUpdateCallBack);
  }

  return UpdateArchive(m_Stream, &m_Item, aSize, anOutStream, aNewItemInfo, 
      m_Method, anIndexInClient, anUpdateCallBack);
}

static const UINT32 kMatchFastLenNormal  = 32;
static const UINT32 kMatchFastLenMX  = 64;

static const UINT32 kNumPassesNormal = 1;
static const UINT32 kNumPassesMX  = 3;

STDMETHODIMP CGZipHandler::SetProperties(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties)
{
  InitMethodProperties();
  for (int i = 0; i < aNumProperties; i++)
  {
    UString aString = UString(aNames[i]);
    aString.MakeUpper();
    const PROPVARIANT &aValue = aValues[i];
    if (aString == L"X")
    {
      m_Method.NumPasses = kNumPassesMX;
      m_Method.NumFastBytes = kMatchFastLenMX;
    }
    else if (aString == L"1" || aString == L"0")
    {
      InitMethodProperties();
    }
    else if (aString == L"PASS")
    {
      if (aValue.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumPasses = aValue.ulVal;
      if (m_Method.NumPasses < 1 || m_Method.NumPasses > 4)
        return E_INVALIDARG;
    }
    else if (aString == L"FB")
    {
      if (aValue.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumFastBytes = aValue.ulVal;
      if (m_Method.NumFastBytes < 3 || m_Method.NumFastBytes > 255)
        return E_INVALIDARG;
    }
    else
      return E_INVALIDARG;
  }
  return S_OK;
}  

