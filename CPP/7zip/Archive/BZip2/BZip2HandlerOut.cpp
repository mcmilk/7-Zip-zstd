// BZip2HandlerOut.cpp

#include "StdAfx.h"

#include "BZip2Handler.h"
#include "BZip2Update.h"

#include "Common/Defs.h"

#include "Windows/PropVariant.h"

#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/ParseProperties.h"

using namespace NWindows;

static const UInt32 kNumPassesX1 = 1;
static const UInt32 kNumPassesX7 = 2;
static const UInt32 kNumPassesX9 = 7;

static const UInt32 kDicSizeX1 = 100000;
static const UInt32 kDicSizeX3 = 500000;
static const UInt32 kDicSizeX5 = 900000;

namespace NArchive {
namespace NBZip2 {

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kUnix;
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

  Int32 newData;
  Int32 newProperties;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0,&newData, &newProperties, &indexInArchive));
 
  if (IntToBool(newProperties))
  {
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidIsFolder, &prop));
      if (prop.vt == VT_BOOL)
      {
        if (prop.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
      }
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }
  }
  
  if (IntToBool(newData))
  {
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }
  
    UInt32 dicSize = _dicSize;
    if (dicSize == 0xFFFFFFFF)
      dicSize = (_level >= 5 ? kDicSizeX5 : 
                (_level >= 3 ? kDicSizeX3 : 
                               kDicSizeX1));

    UInt32 numPasses = _numPasses;
    if (numPasses == 0xFFFFFFFF)
      numPasses = (_level >= 9 ? kNumPassesX9 : 
                  (_level >= 7 ? kNumPassesX7 : 
                                 kNumPassesX1));

    return UpdateArchive(
        EXTERNAL_CODECS_VARS
        size, outStream, 0, dicSize, numPasses, 
        #ifdef COMPRESS_MT
        _numThreads, 
        #endif
        updateCallback);
  }
  if (indexInArchive != 0)
    return E_INVALIDARG;
  RINOK(_stream->Seek(_streamStartPosition, STREAM_SEEK_SET, NULL));
  return CopyStreams(_stream, outStream);
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  InitMethodProperties();
  #ifdef COMPRESS_MT
  const UInt32 numProcessors = NSystem::GetNumberOfProcessors();
  _numThreads = numProcessors;
  #endif

  for (int i = 0; i < numProperties; i++)
  {
    UString name = UString(names[i]);
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &prop = values[i];

    if (name[0] == 'X')
    {
      UInt32 level = 9;
      RINOK(ParsePropValue(name.Mid(1), prop, level));
      _level = level;
      continue;
    }
    if (name[0] == 'D')
    {
      UInt32 dicSize = kDicSizeX5;
      RINOK(ParsePropDictionaryValue(name.Mid(1), prop, dicSize));
      _dicSize = dicSize;
      continue;
    }
    if (name.Left(4) == L"PASS")
    {
      UInt32 num = kNumPassesX9;
      RINOK(ParsePropValue(name.Mid(4), prop, num));
      _numPasses = num;
      continue;
    }
    if (name.Left(2) == L"MT")
    {
      #ifdef COMPRESS_MT
      RINOK(ParseMtProp(name.Mid(2), prop, numProcessors, _numThreads));
      #endif
      continue;
    }
    return E_INVALIDARG;
  }
  return S_OK;
}  

}}
