// Tar/OutHandler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "UpdateEngine.h"

#include "Archive/Common/ItemNameUtils.h"

using namespace NArchive;
using namespace NTar;

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

STDMETHODIMP CHandler::GetFileTimeType(UINT32 *type)
{
  *type = NFileTimeType::kUnix;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(IOutStream *outStream, UINT32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN
  CObjectVector<CUpdateItemInfo> updateItems;
  for(UINT32 i = 0; i < numItems; i++)
  {
    CUpdateItemInfo updateItem;
    INT32 newData;
    INT32 newProperties;
    UINT32 indexInArchive;
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
      UINT32 attributes;
      {
        NCOM::CPropVariant propVariant;
        RETURN_IF_NOT_S_OK(updateCallback->GetProperty(i, kpidAttributes, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          attributes = 0;
        else if (propVariant.vt != VT_UI4)
          return E_INVALIDARG;
        else
          attributes = propVariant.ulVal;
      }
      {
        NCOM::CPropVariant propVariant;
        RETURN_IF_NOT_S_OK(updateCallback->GetProperty(i, kpidLastWriteTime, &propVariant));
        if (propVariant.vt != VT_FILETIME)
          return E_INVALIDARG;
        utcTime = propVariant.filetime;
      }
      {
        NCOM::CPropVariant propVariant;
        RETURN_IF_NOT_S_OK(updateCallback->GetProperty(i, kpidPath, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          name.Empty();
        else if (propVariant.vt != VT_BSTR)
          return E_INVALIDARG;
        else
          name = propVariant.bstrVal;
      }
      {
        NCOM::CPropVariant propVariant;
        RETURN_IF_NOT_S_OK(updateCallback->GetProperty(i, kpidIsFolder, &propVariant));
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
      UINT64 size;
      {
        NCOM::CPropVariant propVariant;
        RETURN_IF_NOT_S_OK(updateCallback->GetProperty(i, kpidSize, &propVariant));
        if (propVariant.vt != VT_UI8)
          return E_INVALIDARG;
        size = *(UINT64 *)(&propVariant.uhVal);
      }
      updateItem.Size = size;
    }
    updateItems.Add(updateItem);
  }
  return UpdateArchive(_inStream, outStream, _items, updateItems, updateCallback);
  COM_TRY_END
}
