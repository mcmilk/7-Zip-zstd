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

static HRESULT GetTime(IArchiveUpdateCallback *callback, int index, PROPID propID, FILETIME &filetime)
{
  filetime.dwHighDateTime = filetime.dwLowDateTime = 0;
  NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(index, propID, &prop));
  if (prop.vt == VT_FILETIME)
    filetime = prop.filetime;
  else if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *callback)
{
  COM_TRY_BEGIN2
  CObjectVector<CUpdateItem> updateItems;
  for(UInt32 i = 0; i < numItems; i++)
  {
    CUpdateItem ui;
    Int32 newData;
    Int32 newProperties;
    UInt32 indexInArchive;
    if (!callback)
      return E_FAIL;
    RINOK(callback->GetUpdateItemInfo(i,
        &newData, // 1 - compress 0 - copy
        &newProperties,
        &indexInArchive));
    ui.NewProperties = IntToBool(newProperties);
    ui.NewData = IntToBool(newData);
    ui.IndexInArchive = indexInArchive;
    ui.IndexInClient = i;
    // bool existInArchive = (indexInArchive != UInt32(-1));
    if (IntToBool(newProperties))
    {
      UString name;
      bool isDirectoryStatusDefined;
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidAttributes, &prop));
        if (prop.vt == VT_EMPTY)
          ui.Attributes = 0;
        else if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        else
          ui.Attributes = prop.ulVal;
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
        RINOK(callback->GetProperty(i, kpidIsFolder, &prop));
        if (prop.vt == VT_EMPTY)
          isDirectoryStatusDefined = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
        {
          ui.IsDirectory = (prop.boolVal != VARIANT_FALSE);
          isDirectoryStatusDefined = true;
        }
      }

      {
        CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidTimeType, &prop));
        if (prop.vt == VT_UI4)
          ui.NtfsTimeIsDefined = (prop.ulVal == NFileTimeType::kWindows);
        else
          ui.NtfsTimeIsDefined = m_WriteNtfsTimeExtra;
      }
      RINOK(GetTime(callback, i, kpidLastWriteTime, ui.NtfsMTime));
      RINOK(GetTime(callback, i, kpidLastAccessTime, ui.NtfsATime));
      RINOK(GetTime(callback, i, kpidCreationTime, ui.NtfsCTime));

      {
        FILETIME localFileTime = { 0, 0 };
        if (ui.NtfsMTime.dwHighDateTime != 0 ||
            ui.NtfsMTime.dwLowDateTime != 0)
          if (!FileTimeToLocalFileTime(&ui.NtfsMTime, &localFileTime))
            return E_INVALIDARG;
        FileTimeToDosTime(localFileTime, ui.Time);
      }

      if (!isDirectoryStatusDefined)
        ui.IsDirectory = ((ui.Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

      name = NItemName::MakeLegalName(name);
      bool needSlash = ui.IsDirectory;
      const wchar_t kSlash = L'/';
      if (!name.IsEmpty())
      {
        if (name[name.Length() - 1] == kSlash)
        {
          if (!ui.IsDirectory)
            return E_INVALIDARG;
          needSlash = false;
        }
      }
      if (needSlash)
        name += kSlash;

      bool tryUtf8 = true;
      if (m_ForseLocal || !m_ForseUtf8)
      {
        bool defaultCharWasUsed;
        ui.Name = UnicodeStringToMultiByte(name, CP_OEMCP, '_', defaultCharWasUsed);
        tryUtf8 = (!m_ForseLocal && defaultCharWasUsed);
      }

      if (tryUtf8)
      {
        bool needUtf = false;
        for (int i = 0; i < name.Length(); i++)
          if ((unsigned)name[i] >= 0x80)
          {
            needUtf = true;
            break;
          }
        ui.IsUtf8 = needUtf;
        if (!ConvertUnicodeToUTF8(name, ui.Name))
          return E_INVALIDARG;
      }

      if (ui.Name.Length() > 0xFFFF)
        return E_INVALIDARG;

      ui.IndexInClient = i;
      /*
      if(existInArchive)
      {
        const CItemEx &itemInfo = m_Items[indexInArchive];
        // ui.Commented = itemInfo.IsCommented();
        ui.Commented = false;
        if(ui.Commented)
        {
          ui.CommentRange.Position = itemInfo.GetCommentPosition();
          ui.CommentRange.Size  = itemInfo.CommentSize;
        }
      }
      else
        ui.Commented = false;
      */
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

  CMyComPtr<ICryptoGetTextPassword2> getTextPassword;
  if (!getTextPassword)
  {
    CMyComPtr<IArchiveUpdateCallback> udateCallBack2(callback);
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
      m_ArchiveIsOpen ? &m_Archive : NULL, &options, callback);
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
    else if (name.CompareNoCase(L"TC") == 0)
      return SetBoolProperty(m_WriteNtfsTimeExtra, prop);
    else if (name.CompareNoCase(L"CL") == 0)
    {
      RINOK(SetBoolProperty(m_ForseLocal, prop));
      if (m_ForseLocal)
        m_ForseUtf8 = false;
      return S_OK;
    }
    else if (name.CompareNoCase(L"CU") == 0)
    {
      RINOK(SetBoolProperty(m_ForseUtf8, prop));
      if (m_ForseUtf8)
        m_ForseLocal = false;
      return S_OK;
    }
    else 
      return E_INVALIDARG;
  }
  return S_OK;
}  

}}
