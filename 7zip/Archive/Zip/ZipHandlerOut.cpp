// Zip/HandlerOut.cpp

#include "StdAfx.h"

#include "ZipHandler.h"
#include "ZipUpdate.h"

#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Common/StringToInt.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../IPassword.h"
#include "../Common/ItemNameUtils.h"

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

namespace NArchive {
namespace NZip {

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *timeType)
{
  *timeType = NFileTimeType::kDOS;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN
  CObjectVector<CUpdateItem> updateItems;
  for(UInt32 i = 0; i < numItems; i++)
  {
    CUpdateItem updateItem;
    Int32 newData;
    Int32 newProperties;
    UInt32 indexInArchive;
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
    bool existInArchive = (indexInArchive != UInt32(-1));
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

      if (!isDirectoryStatusDefined)
        updateItem.IsDirectory = ((updateItem.Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

      name = NItemName::MakeLegalName(name);
      bool needSlash = updateItem.IsDirectory;
      const wchar_t kSlash = L'/';
      if (!name.IsEmpty())
      {
        if (name[name.Length() - 1] == kSlash)
        {
          if (!updateItem.IsDirectory)
            return E_INVALIDARG;
          needSlash = false;
        }
      }
      if (needSlash)
        name += kSlash;
      updateItem.Name = UnicodeStringToMultiByte(name, CP_OEMCP);

      updateItem.IndexInClient = i;
      /*
      if(existInArchive)
      {
        const CItemEx &itemInfo = m_Items[indexInArchive];
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
      */
    }
    if (IntToBool(newData))
    {
      UInt64 size;
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidSize, &propVariant));
        if (propVariant.vt != VT_UI8)
          return E_INVALIDARG;
        size = propVariant.uhVal.QuadPart;
      }
      updateItem.Size = size;
    }
    updateItems.Add(updateItem);
  }

  CMyComPtr<ICryptoGetTextPassword2> getTextPassword;
  if (!getTextPassword)
  {
    CMyComPtr<IArchiveUpdateCallback> udateCallBack2(updateCallback);
    udateCallBack2.QueryInterface(IID_ICryptoGetTextPassword2, &getTextPassword);
  }
  if (getTextPassword)
  {
    CMyComBSTR password;
    Int32 passwordIsDefined;
    RINOK(getTextPassword->CryptoGetTextPassword2(
        &passwordIsDefined, &password));
    if (m_Method.PasswordIsDefined = IntToBool(passwordIsDefined))
      m_Method.Password = UnicodeStringToMultiByte(
          (const wchar_t *)password, CP_OEMCP);
  }
  else
    m_Method.PasswordIsDefined = false;

  return Update(m_Items, updateItems, outStream, 
      m_ArchiveIsOpen ? &m_Archive : NULL, &m_Method, updateCallback);
  COM_TRY_END
}

static const UInt32 kNumPassesNormal = 1;
static const UInt32 kNumPassesMX  = 3;

static const UInt32 kMatchFastLenNormal  = 32;
static const UInt32 kMatchFastLenMX  = 64;

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  InitMethodProperties();
  Byte mainMethod = NFileHeader::NCompressionMethod::kDeflated;
  for (int i = 0; i < numProperties; i++)
  {
    UString name = UString(names[i]);
    name.MakeUpper();
    const PROPVARIANT &value = values[i];

    if (name[0] == 'X')
    {
      name.Delete(0);
      UInt32 level = 9;
      if (value.vt == VT_UI4)
      {
        if (!name.IsEmpty())
          return E_INVALIDARG;
        level = value.ulVal;
      }
      else if (value.vt == VT_EMPTY)
      {
        if(!name.IsEmpty())
        {
          const wchar_t *start = name;
          const wchar_t *end;
          UInt64 v = ConvertStringToUInt64(start, &end);
          if (end - start != name.Length())
            return E_INVALIDARG;
          level = (UInt32)v;
        }
      }
      else
        return E_INVALIDARG;
      if (level == 0)
      {
        mainMethod = NFileHeader::NCompressionMethod::kStored;
      }
      else if (level < 7)
      {
        InitMethodProperties();
        if (mainMethod == NFileHeader::NCompressionMethod::kStored)
          mainMethod = NFileHeader::NCompressionMethod::kDeflated;
      }
      else
      {
        m_Method.NumPasses = kNumPassesMX;
        m_Method.NumFastBytes = kMatchFastLenMX;
        if (mainMethod == NFileHeader::NCompressionMethod::kStored)
          mainMethod = NFileHeader::NCompressionMethod::kDeflated;
      }
      continue;
    }
    else if (name == L"M")
    {
      if (value.vt == VT_BSTR)
      {
        UString valueString = value.bstrVal;
        valueString.MakeUpper();
        if (valueString == L"COPY")
          mainMethod = NFileHeader::NCompressionMethod::kStored;
        else if (valueString == L"DEFLATE")
          mainMethod = NFileHeader::NCompressionMethod::kDeflated;
        else if (valueString == L"DEFLATE64")
          mainMethod = NFileHeader::NCompressionMethod::kDeflated64;
        else if (valueString == L"BZIP2")
          mainMethod = NFileHeader::NCompressionMethod::kBZip2;
        else 
          return E_INVALIDARG;
      }
      else if (value.vt == VT_UI4)
      {
        switch(value.ulVal)
        {
          case NFileHeader::NCompressionMethod::kStored:
          case NFileHeader::NCompressionMethod::kDeflated:
          case NFileHeader::NCompressionMethod::kDeflated64:
          case NFileHeader::NCompressionMethod::kBZip2:
            mainMethod = (Byte)value.ulVal;
            break;
          default:
            return E_INVALIDARG;
        }
      }
      else
        return E_INVALIDARG;
    }
    else if (name == L"PASS")
    {
      if (value.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumPasses = value.ulVal;
      if (m_Method.NumPasses < 1 || m_Method.NumPasses > 4)
        return E_INVALIDARG;
    }
    else if (name == L"FB")
    {
      if (value.vt != VT_UI4)
        return E_INVALIDARG;
      m_Method.NumFastBytes = value.ulVal;
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

}}
