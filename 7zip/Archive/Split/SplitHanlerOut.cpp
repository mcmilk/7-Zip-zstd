// Split/OutHandler.cpp

#include "StdAfx.h"

#include "SplitHandler.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Common/ComTry.h"
#include "../../../Common/StringToInt.h"

using namespace NWindows;

namespace NArchive {
namespace NSplit {

/*
STDMETHODIMP CHandler::GetFileTimeType(UINT32 *type)
{
  *type = NFileTimeType::kWindows;
  return S_OK;
}


STDMETHODIMP CHandler::UpdateItems(IOutStream *outStream, UINT32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN

  if (numItems != 1)
    return E_INVALIDARG;

  UINT64 volumeSize;
  RINOK(updateCallback->GetVolumeSize(0, &volumeSize));

  INT32 newData;
  INT32 newProperties;
  UINT32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;

  UINT32 fileIndex;
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
  UINT64 newSize;
  bool thereIsCopyData = false;
  if (newData != 0)
  {
    NCOM::CPropVariant propVariant;
    RINOK(updateCallback->GetProperty(fileIndex, kpidSize, &propVariant));
    if (propVariant.vt != VT_UI8)
      return E_INVALIDARG;
    newSize = *(const UINT64 *)(&propVariant.uhVal);
  }
  else
    thereIsCopyData = true;

  return S_OK;
  COM_TRY_END
}
*/

}}
