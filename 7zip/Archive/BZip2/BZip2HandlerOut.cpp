// BZip2/OutHandler.cpp

#include "StdAfx.h"

#include "BZip2Handler.h"
#include "BZip2Update.h"

#include "../../../Common/Defs.h"
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
    return UpdateArchive(size, outStream, 0, updateCallback);
  }
  if (indexInArchive != 0)
    return E_INVALIDARG;
  RINOK(_stream->Seek(_streamStartPosition, STREAM_SEEK_SET, NULL));
  return CopyStreams(_stream, outStream, updateCallback);
}

}}
