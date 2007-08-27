// Archive/GZip/OutHandler.cpp

#include "StdAfx.h"

#include "GZipHandler.h"
#include "GZipUpdate.h"

#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/Time.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "../../Compress/Copy/CopyCoder.h"
#include "../Common/ParseProperties.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NGZip {

static const UInt32 kAlgoX1 = 0;
static const UInt32 kAlgoX5 = 1;

static const UInt32 kNumPassesX1  = 1;
static const UInt32 kNumPassesX7  = 3;
static const UInt32 kNumPassesX9  = 10;

static const UInt32 kNumFastBytesX1 = 32;
static const UInt32 kNumFastBytesX7 = 64;
static const UInt32 kNumFastBytesX9 = 128;


STDMETHODIMP CHandler::GetFileTimeType(UInt32 *timeType)
{
  *timeType = NFileTimeType::kUnix;
  return S_OK;
}

static HRESULT CopyStreams(ISequentialInStream *inStream, ISequentialOutStream *outStream)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  if (numItems != 1)
    return E_INVALIDARG;

  UInt64 size;
  Int32 newData;
  Int32 newProperties;
  UInt32 indexInArchive;
  UInt32 itemIndex = 0;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProperties, &indexInArchive));

  CItem newItem = m_Item;
  newItem.ExtraFlags = 0;
  newItem.Flags = 0;
  if (IntToBool(newProperties))
  {
    UInt32 attributes;
    FILETIME utcTime;
    UString name;
    bool isDirectory;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(itemIndex, kpidAttributes, &prop));
      if (prop.vt == VT_EMPTY)
        attributes = 0;
      else if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      else
        attributes = prop.ulVal;
    }
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(itemIndex, kpidLastWriteTime, &prop));
      if (prop.vt != VT_FILETIME)
        return E_INVALIDARG;
      utcTime = prop.filetime;
    }
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(itemIndex, kpidPath, &prop));
      if (prop.vt == VT_EMPTY)
        name.Empty();
      else if (prop.vt != VT_BSTR)
        return E_INVALIDARG;
      else
        name = prop.bstrVal;
    }
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(itemIndex, kpidIsFolder, &prop));
      if (prop.vt == VT_EMPTY)
        isDirectory = false;
      else if (prop.vt != VT_BOOL)
        return E_INVALIDARG;
      else
        isDirectory = (prop.boolVal != VARIANT_FALSE);
    }
    if (isDirectory || NFile::NFind::NAttributes::IsDirectory(attributes))
      return E_INVALIDARG;
    if(!FileTimeToUnixTime(utcTime, newItem.Time))
      return E_INVALIDARG;
    newItem.Name = UnicodeStringToMultiByte(name, CP_ACP);
    int dirDelimiterPos = newItem.Name.ReverseFind(CHAR_PATH_SEPARATOR);
    if (dirDelimiterPos >= 0)
      newItem.Name = newItem.Name.Mid(dirDelimiterPos + 1);
    
    newItem.SetNameIsPresentFlag(!newItem.Name.IsEmpty());
  }

  if (IntToBool(newData))
  {
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(itemIndex, kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }
    newItem.UnPackSize32 = (UInt32)size;

    UInt32 level = m_Level;
    if (level == 0xFFFFFFFF)
      level = 5;
    if (m_Method.NumPasses == 0xFFFFFFFF)
      m_Method.NumPasses = (level >= 9 ? kNumPassesX9 : 
                           (level >= 7 ? kNumPassesX7 : 
                                         kNumPassesX1));
    if (m_Method.NumFastBytes == 0xFFFFFFFF)
      m_Method.NumFastBytes = (level >= 9 ? kNumFastBytesX9 : 
                              (level >= 7 ? kNumFastBytesX7 : 
                                            kNumFastBytesX1));
    if (m_Method.Algo == 0xFFFFFFFF)
      m_Method.Algo = 
                    (level >= 5 ? kAlgoX5 : 
                                  kAlgoX1); 

    return UpdateArchive(
        EXTERNAL_CODECS_VARS
        m_Stream, size, outStream, newItem, m_Method, itemIndex, updateCallback);
  }
    
  if (indexInArchive != 0)
    return E_INVALIDARG;

  if (IntToBool(newProperties))
  {
    COutArchive outArchive;
    outArchive.Create(outStream);
    outArchive.WriteHeader(newItem);
    RINOK(m_Stream->Seek(m_StreamStartPosition + m_DataOffset, STREAM_SEEK_SET, NULL));
  }
  else
  {
    RINOK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  }
  return CopyStreams(m_Stream, outStream);
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  InitMethodProperties();
  for (int i = 0; i < numProperties; i++)
  {
    UString name = names[i];
    name.MakeUpper();
    const PROPVARIANT &prop = values[i];
    if (name[0] == L'X')
    {
      UInt32 level = 9;
      RINOK(ParsePropValue(name.Mid(1), prop, level));
      m_Level = level;
    }
    else if (name.Left(4) == L"PASS")
    {
      UInt32 num = kNumPassesX9;
      RINOK(ParsePropValue(name.Mid(4), prop, num));
      m_Method.NumPasses = num;
    }
    else if (name.Left(2) == L"FB")
    {
      UInt32 num = kNumFastBytesX9;
      RINOK(ParsePropValue(name.Mid(2), prop, num));
      m_Method.NumFastBytes = num;
    }
    else if (name.Left(2) == L"MC")
    {
      UInt32 num = 0xFFFFFFFF;
      RINOK(ParsePropValue(name.Mid(2), prop, num));
      m_Method.NumMatchFinderCycles = num;
      m_Method.NumMatchFinderCyclesDefined = true;
    }
    else if (name.Left(1) == L"A")
    {
      UInt32 num = kAlgoX5;
      RINOK(ParsePropValue(name.Mid(1), prop, num));
      m_Method.Algo = num;
    }
    else
      return E_INVALIDARG;
  }
  return S_OK;
}  

}}
