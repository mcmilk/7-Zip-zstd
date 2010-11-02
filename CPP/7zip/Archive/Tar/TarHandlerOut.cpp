// TarHandlerOut.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "TarHandler.h"
#include "TarUpdate.h"

using namespace NWindows;

namespace NArchive {
namespace NTar {

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kUnix;
  return S_OK;
}

static HRESULT GetPropString(IArchiveUpdateCallback *callback, UInt32 index, PROPID propId, AString &res)
{
  NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(index, propId, &prop));
  if (prop.vt == VT_BSTR)
    res = UnicodeStringToMultiByte(prop.bstrVal, CP_OEMCP);
  else if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *callback)
{
  COM_TRY_BEGIN
  if ((_stream && !_errorMessage.IsEmpty()) || _seqStream)
    return E_NOTIMPL;
  CObjectVector<CUpdateItem> updateItems;
  for (UInt32 i = 0; i < numItems; i++)
  {
    CUpdateItem ui;
    Int32 newData;
    Int32 newProps;
    UInt32 indexInArchive;
    if (!callback)
      return E_FAIL;
    RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArchive));
    ui.NewProps = IntToBool(newProps);
    ui.NewData = IntToBool(newData);
    ui.IndexInArchive = indexInArchive;
    ui.IndexInClient = i;

    if (IntToBool(newProps))
    {
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

      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidPosixAttrib, &prop));
        if (prop.vt == VT_EMPTY)
          ui.Mode = 0777 | (ui.IsDir ? 0040000 : 0100000);
        else if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        else
          ui.Mode = prop.ulVal;
      }
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidMTime, &prop));
        if (prop.vt == VT_EMPTY)
          ui.Time = 0;
        else if (prop.vt != VT_FILETIME)
          return E_INVALIDARG;
        else if (!NTime::FileTimeToUnixTime(prop.filetime, ui.Time))
          ui.Time = 0;
      }
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidPath, &prop));
        if (prop.vt == VT_BSTR)
          ui.Name = UnicodeStringToMultiByte(NItemName::MakeLegalName(prop.bstrVal), CP_OEMCP);
        else if (prop.vt != VT_EMPTY)
          return E_INVALIDARG;
        if (ui.IsDir)
          ui.Name += '/';
      }
      RINOK(GetPropString(callback, i, kpidUser, ui.User));
      RINOK(GetPropString(callback, i, kpidGroup, ui.Group));
    }
    if (IntToBool(newData))
    {
      NCOM::CPropVariant prop;
      RINOK(callback->GetProperty(i, kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      ui.Size = prop.uhVal.QuadPart;
      /*
      // now we support GNU extension for big files
      if (ui.Size >= ((UInt64)1 << 33))
        return E_INVALIDARG;
      */
    }
    updateItems.Add(ui);
  }
  return UpdateArchive(_stream, outStream, _items, updateItems, callback);
  COM_TRY_END
}

}}
