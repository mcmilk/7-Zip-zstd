// Zip/OutHandler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/StringConvert.h"

#include "Windows/Time.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "CompressionMethod.h"

#include "Compression/CopyCoder.h"

#include "UpdateEngine.h"

using namespace NArchive;
using namespace NGZip;

using namespace NWindows;
using namespace NTime;

static const kNumItemInArchive = 1;

STDMETHODIMP CGZipHandler::GetFileTimeType(UINT32 *timeType)
{
  *timeType = NFileTimeType::kUnix;
  return S_OK;
}

static HRESULT CopyStreams(IInStream *inStream, IOutStream *outStream, 
    IArchiveUpdateCallback *updateCallback)
{
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

STDMETHODIMP CGZipHandler::UpdateItems(IOutStream *outStream, UINT32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  if (numItems != 1)
    return E_INVALIDARG;

  UINT64 size;
  INT32 newData;
  INT32 newProperties;
  UINT32 indexInArchive;
  UINT32 itemIndex = 0;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0, 
      &newData, &newProperties, &indexInArchive));

  CItemInfo newItemInfo = m_Item;
  newItemInfo.ExtraFlags = 0;
  newItemInfo.Flags = 0;
  if (IntToBool(newProperties))
  {
    UINT32 attributes;
    FILETIME utcTime;
    UString name;
    bool isDirectory;
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(itemIndex, kpidAttributes, &propVariant));
      if (propVariant.vt == VT_EMPTY)
        attributes = 0;
      else if (propVariant.vt != VT_UI4)
        return E_INVALIDARG;
      else
        attributes = propVariant.ulVal;
    }
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(itemIndex, kpidLastWriteTime, &propVariant));
      if (propVariant.vt != VT_FILETIME)
        return E_INVALIDARG;
      utcTime = propVariant.filetime;
    }
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(itemIndex, kpidPath, &propVariant));
      if (propVariant.vt == VT_EMPTY)
        name.Empty();
      else if (propVariant.vt != VT_BSTR)
        return E_INVALIDARG;
      else
        name = propVariant.bstrVal;
    }
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(itemIndex, kpidIsFolder, &propVariant));
      if (propVariant.vt == VT_EMPTY)
        isDirectory = false;
      else if (propVariant.vt != VT_BOOL)
        return E_INVALIDARG;
      else
        isDirectory = (propVariant.boolVal != VARIANT_FALSE);
    }
    if (isDirectory || NFile::NFind::NAttributes::IsDirectory(attributes))
      return E_INVALIDARG;
    time_t unixTime;
    if(!FileTimeToUnixTime(utcTime, unixTime))
      return E_INVALIDARG;
    
    newItemInfo.Time = unixTime;
    newItemInfo.Name = UnicodeStringToMultiByte(name, CP_ACP);
    int dirDelimiterPos = newItemInfo.Name.ReverseFind('\\');
    if (dirDelimiterPos >= 0)
      newItemInfo.Name = newItemInfo.Name.Mid(dirDelimiterPos + 1);
    
    newItemInfo.SetNameIsPresentFlag(!newItemInfo.Name.IsEmpty());
  }

  if (IntToBool(newData))
  {
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(itemIndex, kpidSize, &propVariant));
      if (propVariant.vt != VT_UI8)
        return E_INVALIDARG;
      size = *(UINT64 *)(&propVariant.uhVal);
    }
    newItemInfo.UnPackSize32 = (UINT32)size;
    return UpdateArchive(m_Stream, size, outStream, newItemInfo, 
        m_Method, itemIndex, updateCallback);
  }
    
  if (indexInArchive != 0)
    return E_INVALIDARG;

  if (IntToBool(newProperties))
  {
    COutArchive outArchive;
    outArchive.Create(outStream);
    outArchive.WriteHeader(newItemInfo);
    RINOK(m_Stream->Seek(m_Item.DataPosition, STREAM_SEEK_SET, NULL));
  }
  else
  {
    RINOK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  }
  return CopyStreams(m_Stream, outStream, updateCallback);
}

static const UINT32 kMatchFastLenNormal  = 32;
static const UINT32 kMatchFastLenMX  = 64;

static const UINT32 kNumPassesNormal = 1;
static const UINT32 kNumPassesMX  = 3;

STDMETHODIMP CGZipHandler::SetProperties(const BSTR *names, const PROPVARIANT *values, INT32 numProperties)
{
  InitMethodProperties();
  for (int i = 0; i < numProperties; i++)
  {
    UString string = UString(names[i]);
    string.MakeUpper();
    const PROPVARIANT &aValue = values[i];
    if (string == L"X")
    {
      m_Method.NumPasses = kNumPassesMX;
      m_Method.NumFastBytes = kMatchFastLenMX;
    }
    else if (string == L"1" || string == L"0")
    {
      InitMethodProperties();
    }
    else if (string == L"PASS")
    {
      if (aValue.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumPasses = aValue.ulVal;
      if (m_Method.NumPasses < 1 || m_Method.NumPasses > 4)
        return E_INVALIDARG;
    }
    else if (string == L"FB")
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

