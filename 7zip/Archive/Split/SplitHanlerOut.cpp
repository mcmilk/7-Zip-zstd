// Split/OutHandler.cpp

#include "StdAfx.h"

#include "SplitHandler.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Common/ComTry.h"
#include "../../../Common/StringToInt.h"

using namespace NWindows;

namespace NArchive {
namespace NSplit {

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kWindows;
  return S_OK;
}


STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN

  if (numItems != 1)
    return E_INVALIDARG;

  UInt64 volumeSize = 0;

  CMyComPtr<IArchiveUpdateCallback2> callback2;
  updateCallback->QueryInterface(IID_IArchiveUpdateCallback2, 
      (void **)&callback2);

  RINOK(callback2->GetVolumeSize(0, &volumeSize));

  Int32 newData;
  Int32 newProperties;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;

  UInt32 fileIndex = 0;
  RINOK(updateCallback->GetUpdateItemInfo(fileIndex,
    &newData, &newProperties, &indexInArchive));

  if (newProperties != 0)
  {
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(fileIndex, kpidIsFolder, &propVariant));
      if (propVariant.vt == VT_EMPTY)
      {
      }
      else if (propVariant.vt != VT_BOOL)
        return E_INVALIDARG;
      else
      {
        if (propVariant.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
      }
    }
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(fileIndex, kpidIsAnti, &propVariant));
      if (propVariant.vt == VT_EMPTY)
      {
      }
      else if (propVariant.vt != VT_BOOL)
        return E_INVALIDARG;
      else
      {
        if (propVariant.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
      }
    }
  }
  UInt64 newSize;
  bool thereIsCopyData = false;
  if (newData != 0)
  {
    NCOM::CPropVariant propVariant;
    RINOK(updateCallback->GetProperty(fileIndex, kpidSize, &propVariant));
    if (propVariant.vt != VT_UI8)
      return E_INVALIDARG;
    newSize = *(const UInt64 *)(&propVariant.uhVal);
  }
  else
    thereIsCopyData = true;

  UInt64 pos = 0;
  while(pos < newSize)
  {

  }
  return S_OK;
  COM_TRY_END
}

}}
