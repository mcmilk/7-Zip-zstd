// Zip/OutHandler.cpp

#include "StdAfx.h"

#include "GZipHandler.h"
#include "GZipUpdate.h"

#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/Time.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "../../Compress/Copy/CopyCoder.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NGZip {

static const kNumItemInArchive = 1;

STDMETHODIMP CHandler::GetFileTimeType(UINT32 *timeType)
{
  *timeType = NFileTimeType::kUnix;
  return S_OK;
}

static HRESULT CopyStreams(IInStream *inStream, IOutStream *outStream, 
    IArchiveUpdateCallback *updateCallback)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

STDMETHODIMP CHandler::UpdateItems(IOutStream *outStream, UINT32 numItems,
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

  CItem newItem = m_Item;
  newItem.ExtraFlags = 0;
  newItem.Flags = 0;
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
    
    newItem.Time = unixTime;
    newItem.Name = UnicodeStringToMultiByte(name, CP_ACP);
    int dirDelimiterPos = newItem.Name.ReverseFind('\\');
    if (dirDelimiterPos >= 0)
      newItem.Name = newItem.Name.Mid(dirDelimiterPos + 1);
    
    newItem.SetNameIsPresentFlag(!newItem.Name.IsEmpty());
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
    newItem.UnPackSize32 = (UINT32)size;
    return UpdateArchive(m_Stream, size, outStream, newItem, 
        m_Method, itemIndex, updateCallback);
  }
    
  if (indexInArchive != 0)
    return E_INVALIDARG;

  if (IntToBool(newProperties))
  {
    COutArchive outArchive;
    outArchive.Create(outStream);
    outArchive.WriteHeader(newItem);
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

STDMETHODIMP CHandler::SetProperties(const BSTR *names, const PROPVARIANT *values, INT32 numProperties)
{
  InitMethodProperties();
  for (int i = 0; i < numProperties; i++)
  {
    UString name = UString(names[i]);
    name.MakeUpper();
    const PROPVARIANT &value = values[i];
    if (name[0] == 'X')
    {
      name.Delete(0);
      UINT32 level = 9;
      if (value.vt == VT_UI4)
      {
        if (!name.IsEmpty())
          return E_INVALIDARG;
        level = value.ulVal;
      }
      else if (value.vt == VT_EMPTY)
      {
        if(!name.IsEmpty())
        {
          const wchar_t *start = name;
          const wchar_t *end;
          UINT64 v = ConvertStringToUINT64(start, &end);
          if (end - start != name.Length())
            return E_INVALIDARG;
          level = (UINT32)v;
        }
      }
      else
        return E_INVALIDARG;
      if (level < 8)
      {
        InitMethodProperties();
      }
      else
      {
        m_Method.NumPasses = kNumPassesMX;
        m_Method.NumFastBytes = kMatchFastLenMX;
      }
      continue;
    }
    else if (name == L"PASS")
    {
      if (value.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumPasses = value.ulVal;
      if (m_Method.NumPasses < 1 || m_Method.NumPasses > 4)
        return E_INVALIDARG;
    }
    else if (name == L"FB")
    {
      if (value.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumFastBytes = value.ulVal;
      if (m_Method.NumFastBytes < 3 || m_Method.NumFastBytes > 255)
        return E_INVALIDARG;
    }
    else
      return E_INVALIDARG;
  }
  return S_OK;
}  

}}
