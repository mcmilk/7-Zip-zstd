// BZip2/OutHandler.cpp

#include "StdAfx.h"

#include "BZip2Handler.h"
#include "BZip2Update.h"

#include "Common/Defs.h"
#include "Common/String.h"
#include "Common/StringToInt.h"
#include "Windows/PropVariant.h"

#include "../../Compress/Copy/CopyCoder.h"

using namespace NWindows;

namespace NArchive {
namespace NBZip2 {

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kUnix;
  return S_OK;
}

static HRESULT CopyStreams(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, IArchiveUpdateCallback *updateCallback)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  if (numItems != 1)
    return E_INVALIDARG;

  Int32 newData;
  Int32 newProperties;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0,
      &newData, &newProperties, &indexInArchive));
 
  if (IntToBool(newProperties))
  {
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(0, kpidIsFolder, &propVariant));
      if (propVariant.vt == VT_BOOL)
      {
        if (propVariant.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
      }
      else if (propVariant.vt != VT_EMPTY)
        return E_INVALIDARG;
    }
  }
  
  if (IntToBool(newData))
  {
    UInt64 size;
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(0, kpidSize, &propVariant));
      if (propVariant.vt != VT_UI8)
        return E_INVALIDARG;
      size = propVariant.uhVal.QuadPart;
    }
    return UpdateArchive(size, outStream, 0, _numPasses, updateCallback);
  }
  if (indexInArchive != 0)
    return E_INVALIDARG;
  RINOK(_stream->Seek(_streamStartPosition, STREAM_SEEK_SET, NULL));
  return CopyStreams(_stream, outStream, updateCallback);
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  InitMethodProperties();
  for (int i = 0; i < numProperties; i++)
  {
    UString name = UString(names[i]);
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &value = values[i];

    if (name[0] == 'X')
    {
      name.Delete(0);
      UInt32 level = 9;
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
          UInt64 v = ConvertStringToUInt64(start, &end);
          if (end - start != name.Length())
            return E_INVALIDARG;
          level = (UInt32)v;
        }
      }
      else
        return E_INVALIDARG;
      if (level < 7)
        _numPasses = 1;
      else if (level < 9)
        _numPasses = 2;
      else 
        _numPasses = 7;
      continue;
    }
    else if (name.Left(4) == L"PASS")
    {
      name.Delete(0, 4);
      UInt32 numPasses = 1;
      if (value.vt == VT_UI4)
      {
        if (!name.IsEmpty())
          return E_INVALIDARG;
        numPasses = value.ulVal;
      }
      else if (value.vt == VT_EMPTY)
      {
        if(!name.IsEmpty())
        {
          const wchar_t *start = name;
          const wchar_t *end;
          UInt64 v = ConvertStringToUInt64(start, &end);
          if (end - start != name.Length())
            return E_INVALIDARG;
          numPasses = (UInt32)v;
        }
      }
      else
        return E_INVALIDARG;

      if (numPasses < 1 || numPasses > 10)
        return E_INVALIDARG;
      _numPasses = numPasses;
      continue;
    }
    return E_INVALIDARG;
  }
  return S_OK;
}  

}}
