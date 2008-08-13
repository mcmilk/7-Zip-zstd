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

namespace NArchive {
namespace NTar {

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kUnix;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *callback)
{
  COM_TRY_BEGIN
  CObjectVector<CUpdateItem> updateItems;
  for (UInt32 i = 0; i < numItems; i++)
  {
    CUpdateItem ui;
    Int32 newData;
    Int32 newProperties;
    UInt32 indexInArchive;
    if (!callback)
      return E_FAIL;
    RINOK(callback->GetUpdateItemInfo(i, &newData, &newProperties, &indexInArchive));
    ui.NewProperties = IntToBool(newProperties);
    ui.NewData = IntToBool(newData);
    ui.IndexInArchive = indexInArchive;
    ui.IndexInClient = i;

    if (IntToBool(newProperties))
    {
      FILETIME utcTime;
      UString name;
      /*
      UInt32 attributes;
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidAttrib, &prop));
        if (prop.vt == VT_EMPTY)
          attributes = 0;
        else if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        else
          attributes = prop.ulVal;
      }
      */
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidMTime, &prop));
        if (prop.vt != VT_FILETIME)
          return E_INVALIDARG;
        utcTime = prop.filetime;
      }
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidPath, &prop));
        if (prop.vt == VT_EMPTY)
          name.Empty();
        else if (prop.vt != VT_BSTR)
          return E_INVALIDARG;
        else
          name = prop.bstrVal;
      }
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidIsDir, &prop));
        if (prop.vt == VT_EMPTY)
          ui.IsDir = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
          ui.IsDir = (prop.boolVal != VARIANT_FALSE);
      }
      ui.Name = UnicodeStringToMultiByte(NItemName::MakeLegalName(name), CP_OEMCP);
      if (ui.IsDir)
        ui.Name += '/';

      if (!NTime::FileTimeToUnixTime(utcTime, ui.Time))
      {
        ui.Time = 0;
        // return E_INVALIDARG;
      }
    }
    if (IntToBool(newData))
    {
      UInt64 size;
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidSize, &prop));
        if (prop.vt != VT_UI8)
          return E_INVALIDARG;
        size = prop.uhVal.QuadPart;
      }
      ui.Size = size;
    }
    updateItems.Add(ui);
  }
  return UpdateArchive(_inStream, outStream, _items, updateItems, callback);
  COM_TRY_END
}

}}
