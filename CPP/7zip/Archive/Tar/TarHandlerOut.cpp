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
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidAttributes, &prop));
        if (prop.vt == VT_EMPTY)
          attributes = 0;
        else if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        else
          attributes = prop.ulVal;
      }
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidLastWriteTime, &prop));
        if (prop.vt != VT_FILETIME)
          return E_INVALIDARG;
        utcTime = prop.filetime;
      }
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidPath, &prop));
        if (prop.vt == VT_EMPTY)
          name.Empty();
        else if (prop.vt != VT_BSTR)
          return E_INVALIDARG;
        else
          name = prop.bstrVal;
      }
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidIsFolder, &prop));
        if (prop.vt == VT_EMPTY)
          isDirectoryStatusDefined = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
        {
          updateItem.IsDirectory = (prop.boolVal != VARIANT_FALSE);
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
      {
        updateItem.Time = 0;
        // return E_INVALIDARG;
      }
    }
    if (IntToBool(newData))
    {
      UInt64 size;
      {
        NCOM::CPropVariant prop;
        RINOK(updateCallback->GetProperty(i, kpidSize, &prop));
        if (prop.vt != VT_UI8)
          return E_INVALIDARG;
        size = prop.uhVal.QuadPart;
      }
      updateItem.Size = size;
    }
    updateItems.Add(updateItem);
  }
  return UpdateArchive(_inStream, outStream, _items, updateItems, updateCallback);
  COM_TRY_END
}

}}
