// Tar/HandlerOut.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/ComTry.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../Common/ItemNameUtils.h"

#include "TarHandler.h"
#include "TarUpdate.h"

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

namespace NArchive {
namespace NTar {

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kUnix;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN
  CObjectVector<CUpdateItemInfo> updateItems;
  for(UInt32 i = 0; i < numItems; i++)
  {
    CUpdateItemInfo updateItem;
    Int32 newData;
    Int32 newProperties;
    UInt32 indexInArchive;
    if (!updateCallback)
      return E_FAIL;
    RINOK(updateCallback->GetUpdateItemInfo(i,
        &newData, &newProperties, &indexInArchive));
    updateItem.NewProperties = IntToBool(newProperties);
    updateItem.NewData = IntToBool(newData);
    updateItem.IndexInArchive = indexInArchive;
    updateItem.IndexInClient = i;

    if (IntToBool(newProperties))
    {
      FILETIME utcTime;
      UString name;
      bool isDirectoryStatusDefined;
      UInt32 attributes;
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidAttributes, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          attributes = 0;
        else if (propVariant.vt != VT_UI4)
          return E_INVALIDARG;
        else
          attributes = propVariant.ulVal;
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidLastWriteTime, &propVariant));
        if (propVariant.vt != VT_FILETIME)
          return E_INVALIDARG;
        utcTime = propVariant.filetime;
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidPath, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          name.Empty();
        else if (propVariant.vt != VT_BSTR)
          return E_INVALIDARG;
        else
          name = propVariant.bstrVal;
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidIsFolder, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          isDirectoryStatusDefined = false;
        else if (propVariant.vt != VT_BOOL)
          return E_INVALIDARG;
        else
        {
          updateItem.IsDirectory = (propVariant.boolVal != VARIANT_FALSE);
          isDirectoryStatusDefined = true;
        }
      }
      updateItem.Name = UnicodeStringToMultiByte(
          NItemName::MakeLegalName(name), CP_OEMCP);
      if (!isDirectoryStatusDefined)
        updateItem.IsDirectory = ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
      if (updateItem.IsDirectory)
        updateItem.Name += '/';
      if(!FileTimeToUnixTime(utcTime, updateItem.Time))
        return E_INVALIDARG;
    }
    if (IntToBool(newData))
    {
      UInt64 size;
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidSize, &propVariant));
        if (propVariant.vt != VT_UI8)
          return E_INVALIDARG;
        size = *(UInt64 *)(&propVariant.uhVal);
      }
      updateItem.Size = size;
    }
    updateItems.Add(updateItem);
  }
  return UpdateArchive(_inStream, outStream, _items, updateItems, updateCallback);
  COM_TRY_END
}

}}
