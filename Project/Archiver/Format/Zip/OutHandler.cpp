// Zip/OutHandler.cpp

#include "StdAfx.h"

// #include "../../Handler/FileTimeType.h"
#include "Handler.h"
#include "Archive/Zip/OutEngine.h"
#include "Archive/Common/ItemNameUtils.h"
#include "Common/StringConvert.h"
#include "UpdateMain.h"

#include "Interface/CryptoInterface.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

using namespace NArchive;
using namespace NZip;

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

STDMETHODIMP CZipHandler::GetFileTimeType(UINT32 *timeType)
{
  *timeType = NFileTimeType::kDOS;
  return S_OK;
}

STDMETHODIMP CZipHandler::UpdateItems(IOutStream *outStream, UINT32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN
  CObjectVector<CUpdateItemInfo> updateItems;
  for(int i = 0; i < numItems; i++)
  {
    CUpdateItemInfo updateItem;
    INT32 newData;
    INT32 newProperties;
    UINT32 indexInArchive;
    if (!updateCallback)
      return E_FAIL;
    RINOK(updateCallback->GetUpdateItemInfo(i,
        &newData, // 1 - compress 0 - copy
        &newProperties,
        &indexInArchive));
    updateItem.NewProperties = IntToBool(newProperties);
    updateItem.NewData = IntToBool(newData);
    updateItem.IndexInArchive = indexInArchive;
    updateItem.IndexInClient = i;
    bool existInArchive = (indexInArchive != UINT32(-1));
    if (IntToBool(newProperties))
    {
      FILETIME utcFileTime;
      UString name;
      bool isDirectoryStatusDefined;
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidAttributes, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          updateItem.Attributes = 0;
        else if (propVariant.vt != VT_UI4)
          return E_INVALIDARG;
        else
          updateItem.Attributes = propVariant.ulVal;
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidLastWriteTime, &propVariant));
        if (propVariant.vt != VT_FILETIME)
          return E_INVALIDARG;
        utcFileTime = propVariant.filetime;
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
      FILETIME localFileTime;
      if(!FileTimeToLocalFileTime(&utcFileTime, &localFileTime))
        return E_INVALIDARG;
      if(!FileTimeToDosTime(localFileTime, updateItem.Time))
        return E_INVALIDARG;
      updateItem.Name = UnicodeStringToMultiByte(
          NItemName::MakeLegalName(name), CP_OEMCP);
      if (!isDirectoryStatusDefined)
        updateItem.IsDirectory = ((updateItem.Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
      if (updateItem.IsDirectory)
        updateItem.Name += '/';
      updateItem.IndexInClient = i;
      if(existInArchive)
      {
        const NArchive::NZip::CItemInfoEx &itemInfo = m_Items[indexInArchive];
        // updateItem.Commented = itemInfo.IsCommented();
        updateItem.Commented = false;
        if(updateItem.Commented)
        {
          updateItem.CommentRange.Position = itemInfo.GetCommentPosition();
          updateItem.CommentRange.Size  = itemInfo.CommentSize;
        }
      }
      else
        updateItem.Commented = false;
    }
    if (IntToBool(newData))
    {
      UINT64 size;
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidSize, &propVariant));
        if (propVariant.vt != VT_UI8)
          return E_INVALIDARG;
        size = *(UINT64 *)(&propVariant.uhVal);
      }
      if(size > _UI32_MAX)
        return E_INVALIDARG;
      updateItem.Size = size;
    }
    updateItems.Add(updateItem);
  }

  CComPtr<ICryptoGetTextPassword2> getTextPassword;
  if (!getTextPassword)
  {
    CComPtr<IArchiveUpdateCallback> udateCallBack2(updateCallback);
    udateCallBack2.QueryInterface(&getTextPassword);
  }
  if (getTextPassword)
  {
    CComBSTR password;
    INT32 passwordIsDefined;
    RINOK(getTextPassword->CryptoGetTextPassword2(
        &passwordIsDefined, &password));
    if (m_Method.PasswordIsDefined = IntToBool(passwordIsDefined))
      m_Method.Password = UnicodeStringToMultiByte(
          (const wchar_t *)password, CP_OEMCP);
  }
  else
    m_Method.PasswordIsDefined = false;

  return UpdateMain(m_Items, updateItems, outStream, 
      m_ArchiveIsOpen ? &m_Archive : NULL, &m_Method, updateCallback);
  COM_TRY_END
}

static const UINT32 kNumPassesNormal = 1;
static const UINT32 kNumPassesMX  = 3;

static const UINT32 kMatchFastLenNormal  = 32;
static const UINT32 kMatchFastLenMX  = 64;


STDMETHODIMP CZipHandler::SetProperties(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties)
{
  InitMethodProperties();
  BYTE mainMethod = NFileHeader::NCompressionMethod::kDeflated;
  for (int i = 0; i < aNumProperties; i++)
  {
    UString aString = UString(aNames[i]);
    aString.MakeUpper();
    const PROPVARIANT &aValue = aValues[i];
    if (aString == L"X")
    {
      m_Method.NumPasses = kNumPassesMX;
      m_Method.NumFastBytes = kMatchFastLenMX;
      if (mainMethod == NFileHeader::NCompressionMethod::kStored)
        mainMethod = NFileHeader::NCompressionMethod::kDeflated;
    }
    else if (aString == L"0")
    {
      mainMethod = NFileHeader::NCompressionMethod::kStored;
    }
    else if (aString == L"1")
    {
      InitMethodProperties();
      if (mainMethod == NFileHeader::NCompressionMethod::kStored)
        mainMethod = NFileHeader::NCompressionMethod::kDeflated;
    }
    else if (aString == L"M")
    {
      if (aValue.vt == VT_BSTR)
      {
        UString valueString = aValue.bstrVal;
        valueString.MakeUpper();
        if (valueString == L"COPY")
          mainMethod = NFileHeader::NCompressionMethod::kStored;
        else if (valueString == L"DEFLATE")
          mainMethod = NFileHeader::NCompressionMethod::kDeflated;
        else if (valueString == L"DEFLATE64")
          mainMethod = NFileHeader::NCompressionMethod::kDeflated64;
        else 
          return E_INVALIDARG;
      }
      else if (aValue.vt == VT_UI4)
      {
        switch(aValue.ulVal)
        {
          case NFileHeader::NCompressionMethod::kStored:
          case NFileHeader::NCompressionMethod::kDeflated:
          case NFileHeader::NCompressionMethod::kDeflated64:
            mainMethod = aValue.ulVal;
            break;
          default:
            return E_INVALIDARG;
        }
      }
      else
        return E_INVALIDARG;
    }
    else if (aString == L"PASS")
    {
      if (aValue.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumPasses = aValue.ulVal;
      if (m_Method.NumPasses < 1 || m_Method.NumPasses > 4)
        return E_INVALIDARG;
    }
    else if (aString == L"FB")
    {
      if (aValue.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumFastBytes = aValue.ulVal;
      if (m_Method.NumFastBytes < 3 || m_Method.NumFastBytes > 255)
        return E_INVALIDARG;
    }
    else
      return E_INVALIDARG;
  }
  m_Method.MethodSequence.Clear();
  if (mainMethod != NFileHeader::NCompressionMethod::kStored)
    m_Method.MethodSequence.Add(mainMethod);
  m_Method.MethodSequence.Add(NFileHeader::NCompressionMethod::kStored);
  return S_OK;
}  
