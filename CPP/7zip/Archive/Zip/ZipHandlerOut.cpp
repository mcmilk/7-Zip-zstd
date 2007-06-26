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
#include "../Common/ParseProperties.h"
#include "../../Crypto/WzAES/WzAES.h"
#include "../../Common/OutBuffer.h"

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

namespace NArchive {
namespace NZip {

static const UInt32 kDeflateAlgoX1 = 0;
static const UInt32 kDeflateAlgoX5 = 1;

static const UInt32 kDeflateNumPassesX1  = 1;
static const UInt32 kDeflateNumPassesX7  = 3;
static const UInt32 kDeflateNumPassesX9  = 10;

static const UInt32 kNumFastBytesX1 = 32;
static const UInt32 kNumFastBytesX7 = 64;
static const UInt32 kNumFastBytesX9 = 128;

static const UInt32 kBZip2NumPassesX1 = 1;
static const UInt32 kBZip2NumPassesX7 = 2;
static const UInt32 kBZip2NumPassesX9 = 7;

static const UInt32 kBZip2DicSizeX1 = 100000;
static const UInt32 kBZip2DicSizeX3 = 500000;
static const UInt32 kBZip2DicSizeX5 = 900000;

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *timeType)
{
  *timeType = NFileTimeType::kDOS;
  return S_OK;
}

static bool IsAsciiString(const UString &s)
{
  for (int i = 0; i < s.Length(); i++)
  {
    wchar_t c = s[i];
    if (c < 0x20 || c > 0x7F)
      return false;
  }
  return true;
}

#define COM_TRY_BEGIN2 try {
#define COM_TRY_END2 } \
catch(const CSystemException &e) { return e.ErrorCode; } \
catch(...) { return E_OUTOFMEMORY; }

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN2
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
    // bool existInArchive = (indexInArchive != UInt32(-1));
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
      {
        // return E_INVALIDARG;
      }

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
      if (updateItem.Name.Length() > 0xFFFF)
        return E_INVALIDARG;

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
  CCompressionMethodMode options;

  if (getTextPassword)
  {
    CMyComBSTR password;
    Int32 passwordIsDefined;
    RINOK(getTextPassword->CryptoGetTextPassword2(&passwordIsDefined, &password));
    options.PasswordIsDefined = IntToBool(passwordIsDefined);
    if (options.PasswordIsDefined)
    {
      if (!IsAsciiString((const wchar_t *)password))
        return E_INVALIDARG;
      if (m_IsAesMode)
      {
        if (options.Password.Length() > NCrypto::NWzAES::kPasswordSizeMax)
          return E_INVALIDARG;
      }
      options.Password = UnicodeStringToMultiByte((const wchar_t *)password, CP_OEMCP);
      options.IsAesMode = m_IsAesMode;
      options.AesKeyMode = m_AesKeyMode;
    }
  }
  else
    options.PasswordIsDefined = false;

  int level = m_Level;
  if (level < 0)
    level = 5;
  
  Byte mainMethod;
  if (m_MainMethod < 0)
    mainMethod = (Byte)(((level == 0) ?
        NFileHeader::NCompressionMethod::kStored :
        NFileHeader::NCompressionMethod::kDeflated));
  else
    mainMethod = (Byte)m_MainMethod;
  options.MethodSequence.Add(mainMethod);
  if (mainMethod != NFileHeader::NCompressionMethod::kStored)
    options.MethodSequence.Add(NFileHeader::NCompressionMethod::kStored);
  bool isDeflate = (mainMethod == NFileHeader::NCompressionMethod::kDeflated) || 
      (mainMethod == NFileHeader::NCompressionMethod::kDeflated64);
  bool isBZip2 = (mainMethod == NFileHeader::NCompressionMethod::kBZip2);
  options.NumPasses = m_NumPasses;
  options.DicSize = m_DicSize;
  options.NumFastBytes = m_NumFastBytes;
  options.NumMatchFinderCycles = m_NumMatchFinderCycles;
  options.NumMatchFinderCyclesDefined = m_NumMatchFinderCyclesDefined;
  options.Algo = m_Algo;
  #ifdef COMPRESS_MT
  options.NumThreads = _numThreads;
  #endif
  if (isDeflate)
  {
    if (options.NumPasses == 0xFFFFFFFF)
      options.NumPasses = (level >= 9 ? kDeflateNumPassesX9 :  
                          (level >= 7 ? kDeflateNumPassesX7 : 
                                        kDeflateNumPassesX1));
    if (options.NumFastBytes == 0xFFFFFFFF)
      options.NumFastBytes = (level >= 9 ? kNumFastBytesX9 : 
                             (level >= 7 ? kNumFastBytesX7 : 
                                           kNumFastBytesX1));
    if (options.Algo == 0xFFFFFFFF)
        options.Algo = 
                    (level >= 5 ? kDeflateAlgoX5 : 
                                  kDeflateAlgoX1); 
  }
  if (isBZip2)
  {
    if (options.NumPasses == 0xFFFFFFFF)
      options.NumPasses = (level >= 9 ? kBZip2NumPassesX9 : 
                          (level >= 7 ? kBZip2NumPassesX7 :  
                                        kBZip2NumPassesX1));
    if (options.DicSize == 0xFFFFFFFF)
      options.DicSize = (level >= 5 ? kBZip2DicSizeX5 : 
                        (level >= 3 ? kBZip2DicSizeX3 : 
                                      kBZip2DicSizeX1));
  }

  return Update(
      EXTERNAL_CODECS_VARS
      m_Items, updateItems, outStream, 
      m_ArchiveIsOpen ? &m_Archive : NULL, &options, updateCallback);
  COM_TRY_END2
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  #ifdef COMPRESS_MT
  const UInt32 numProcessors = NSystem::GetNumberOfProcessors();
  _numThreads = numProcessors;
  #endif
  InitMethodProperties();
  for (int i = 0; i < numProperties; i++)
  {
    UString name = UString(names[i]);
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &prop = values[i];

    if (name[0] == L'X')
    {
      UInt32 level = 9;
      RINOK(ParsePropValue(name.Mid(1), prop, level));
      m_Level = level;
      continue;
    }
    else if (name == L"M")
    {
      if (prop.vt == VT_BSTR)
      {
        UString valueString = prop.bstrVal;
        valueString.MakeUpper();
        if (valueString == L"COPY")
          m_MainMethod = NFileHeader::NCompressionMethod::kStored;
        else if (valueString == L"DEFLATE")
          m_MainMethod = NFileHeader::NCompressionMethod::kDeflated;
        else if (valueString == L"DEFLATE64")
          m_MainMethod = NFileHeader::NCompressionMethod::kDeflated64;
        else if (valueString == L"BZIP2")
          m_MainMethod = NFileHeader::NCompressionMethod::kBZip2;
        else 
          return E_INVALIDARG;
      }
      else if (prop.vt == VT_UI4)
      {
        switch(prop.ulVal)
        {
          case NFileHeader::NCompressionMethod::kStored:
          case NFileHeader::NCompressionMethod::kDeflated:
          case NFileHeader::NCompressionMethod::kDeflated64:
          case NFileHeader::NCompressionMethod::kBZip2:
            m_MainMethod = (Byte)prop.ulVal;
            break;
          default:
            return E_INVALIDARG;
        }
      }
      else
        return E_INVALIDARG;
    }
    else if (name.Left(2) == L"EM")
    {
      if (prop.vt == VT_BSTR)
      {
        UString valueString = prop.bstrVal;
        valueString.MakeUpper();
        if (valueString.Left(3) == L"AES")
        {
          valueString = valueString.Mid(3);
          if (valueString == L"128")
            m_AesKeyMode = 1;
          else if (valueString == L"192")
            m_AesKeyMode = 2;
          else if (valueString == L"256" || valueString.IsEmpty())
            m_AesKeyMode = 3;
          else
            return E_INVALIDARG;
          m_IsAesMode = true;
        }
        else if (valueString == L"ZIPCRYPTO")
          m_IsAesMode = false;
        else
          return E_INVALIDARG;
      }
      else
        return E_INVALIDARG;
    }
    else if (name[0] == L'D')
    {
      UInt32 dicSize = kBZip2DicSizeX5;
      RINOK(ParsePropDictionaryValue(name.Mid(1), prop, dicSize));
      m_DicSize = dicSize;
    }
    else if (name.Left(4) == L"PASS")
    {
      UInt32 num = kDeflateNumPassesX9;
      RINOK(ParsePropValue(name.Mid(4), prop, num));
      m_NumPasses = num;
    }
    else if (name.Left(2) == L"FB")
    {
      UInt32 num = kNumFastBytesX9;
      RINOK(ParsePropValue(name.Mid(2), prop, num));
      m_NumFastBytes = num;
    }
    else if (name.Left(2) == L"MC")
    {
      UInt32 num = 0xFFFFFFFF;
      RINOK(ParsePropValue(name.Mid(2), prop, num));
      m_NumMatchFinderCycles = num;
      m_NumMatchFinderCyclesDefined = true;
    }
    else if (name.Left(2) == L"MT")
    {
      #ifdef COMPRESS_MT
      RINOK(ParseMtProp(name.Mid(2), prop, numProcessors, _numThreads));
      #endif
    }
    else if (name.Left(1) == L"A")
    {
      UInt32 num = kDeflateAlgoX5;
      RINOK(ParsePropValue(name.Mid(1), prop, num));
      m_Algo = num;
    }
    else 
      return E_INVALIDARG;
  }
  return S_OK;
}  

}}
