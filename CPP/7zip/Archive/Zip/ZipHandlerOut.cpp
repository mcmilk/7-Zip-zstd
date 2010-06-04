// ZipHandlerOut.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../IPassword.h"

#include "../../Common/OutBuffer.h"

#include "../../Crypto/WzAes.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/ParseProperties.h"

#include "ZipHandler.h"
#include "ZipUpdate.h"

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

namespace NArchive {
namespace NZip {

static const UInt32 kLzAlgoX1 = 0;
static const UInt32 kLzAlgoX5 = 1;

static const UInt32 kDeflateNumPassesX1  = 1;
static const UInt32 kDeflateNumPassesX7  = 3;
static const UInt32 kDeflateNumPassesX9  = 10;

static const UInt32 kDeflateNumFastBytesX1 = 32;
static const UInt32 kDeflateNumFastBytesX7 = 64;
static const UInt32 kDeflateNumFastBytesX9 = 128;

static const wchar_t *kLzmaMatchFinderX1 = L"HC4";
static const wchar_t *kLzmaMatchFinderX5 = L"BT4";

static const UInt32 kLzmaNumFastBytesX1 = 32;
static const UInt32 kLzmaNumFastBytesX7 = 64;

static const UInt32 kLzmaDicSizeX1 = 1 << 16;
static const UInt32 kLzmaDicSizeX3 = 1 << 20;
static const UInt32 kLzmaDicSizeX5 = 1 << 24;
static const UInt32 kLzmaDicSizeX7 = 1 << 25;
static const UInt32 kLzmaDicSizeX9 = 1 << 26;

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
  bool thereAreAesUpdates = false;
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
    bool existInArchive = (indexInArchive != (UInt32)-1);
    if (existInArchive && newData)
      if (m_Items[indexInArchive].IsAesEncrypted())
        thereAreAesUpdates = true;

    if (IntToBool(newProperties))
    {
      UString name;
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidAttrib, &prop));
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
        RINOK(callback->GetProperty(i, kpidIsDir, &prop));
        if (prop.vt == VT_EMPTY)
          ui.IsDir = false;
        else if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        else
          ui.IsDir = (prop.boolVal != VARIANT_FALSE);
      }

      {
        CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidTimeType, &prop));
        if (prop.vt == VT_UI4)
          ui.NtfsTimeIsDefined = (prop.ulVal == NFileTimeType::kWindows);
        else
          ui.NtfsTimeIsDefined = m_WriteNtfsTimeExtra;
      }
      RINOK(GetTime(callback, i, kpidMTime, ui.NtfsMTime));
      RINOK(GetTime(callback, i, kpidATime, ui.NtfsATime));
      RINOK(GetTime(callback, i, kpidCTime, ui.NtfsCTime));

      {
        FILETIME localFileTime = { 0, 0 };
        if (ui.NtfsMTime.dwHighDateTime != 0 ||
            ui.NtfsMTime.dwLowDateTime != 0)
          if (!FileTimeToLocalFileTime(&ui.NtfsMTime, &localFileTime))
            return E_INVALIDARG;
        FileTimeToDosTime(localFileTime, ui.Time);
      }

      name = NItemName::MakeLegalName(name);
      bool needSlash = ui.IsDir;
      const wchar_t kSlash = L'/';
      if (!name.IsEmpty())
      {
        if (name[name.Length() - 1] == kSlash)
        {
          if (!ui.IsDir)
            return E_INVALIDARG;
          needSlash = false;
        }
      }
      if (needSlash)
        name += kSlash;

      bool tryUtf8 = true;
      if (m_ForceLocal || !m_ForceUtf8)
      {
        bool defaultCharWasUsed;
        ui.Name = UnicodeStringToMultiByte(name, CP_OEMCP, '_', defaultCharWasUsed);
        tryUtf8 = (!m_ForceLocal && (defaultCharWasUsed ||
          MultiByteToUnicodeString(ui.Name, CP_OEMCP) != name));
      }

      if (tryUtf8)
      {
        int i;
        for (i = 0; i < name.Length() && (unsigned)name[i] < 0x80; i++);
        ui.IsUtf8 = (i != name.Length());
        if (!ConvertUnicodeToUTF8(name, ui.Name))
          return E_INVALIDARG;
      }

      if (ui.Name.Length() >= (1 << 16))
        return E_INVALIDARG;

      ui.IndexInClient = i;
      /*
      if (existInArchive)
      {
        const CItemEx &itemInfo = m_Items[indexInArchive];
        // ui.Commented = itemInfo.IsCommented();
        ui.Commented = false;
        if (ui.Commented)
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
      options.IsAesMode = (m_ForceAesMode ? m_IsAesMode : thereAreAesUpdates);
      options.AesKeyMode = m_AesKeyMode;

      if (!IsAsciiString((const wchar_t *)password))
        return E_INVALIDARG;
      if (options.IsAesMode)
      {
        if (options.Password.Length() > NCrypto::NWzAes::kPasswordSizeMax)
          return E_INVALIDARG;
      }
      options.Password = UnicodeStringToMultiByte((const wchar_t *)password, CP_OEMCP);
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
  bool isLZMA = (mainMethod == NFileHeader::NCompressionMethod::kLZMA);
  bool isLz = (isLZMA || isDeflate);
  options.NumPasses = m_NumPasses;
  options.DicSize = m_DicSize;
  options.NumFastBytes = m_NumFastBytes;
  options.NumMatchFinderCycles = m_NumMatchFinderCycles;
  options.NumMatchFinderCyclesDefined = m_NumMatchFinderCyclesDefined;
  options.Algo = m_Algo;
  options.MemSize = m_MemSize;
  options.Order = m_Order;
  #ifndef _7ZIP_ST
  options.NumThreads = _numThreads;
  #endif
  if (isLz)
  {
    if (isDeflate)
    {
      if (options.NumPasses == 0xFFFFFFFF)
        options.NumPasses = (level >= 9 ? kDeflateNumPassesX9 :
                            (level >= 7 ? kDeflateNumPassesX7 :
                                          kDeflateNumPassesX1));
      if (options.NumFastBytes == 0xFFFFFFFF)
        options.NumFastBytes = (level >= 9 ? kDeflateNumFastBytesX9 :
                               (level >= 7 ? kDeflateNumFastBytesX7 :
                                             kDeflateNumFastBytesX1));
    }
    else if (isLZMA)
    {
      if (options.DicSize == 0xFFFFFFFF)
        options.DicSize =
          (level >= 9 ? kLzmaDicSizeX9 :
          (level >= 7 ? kLzmaDicSizeX7 :
          (level >= 5 ? kLzmaDicSizeX5 :
          (level >= 3 ? kLzmaDicSizeX3 :
                        kLzmaDicSizeX1))));

      if (options.NumFastBytes == 0xFFFFFFFF)
        options.NumFastBytes = (level >= 7 ? kLzmaNumFastBytesX7 :
                                             kLzmaNumFastBytesX1);

      options.MatchFinder =
        (level >= 5 ? kLzmaMatchFinderX5 :
                      kLzmaMatchFinderX1);
    }

    if (options.Algo == 0xFFFFFFFF)
        options.Algo = (level >= 5 ? kLzAlgoX5 :
                                     kLzAlgoX1);
  }
  if (mainMethod == NFileHeader::NCompressionMethod::kBZip2)
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
  if (mainMethod == NFileHeader::NCompressionMethod::kPPMd)
  {
    int level2 = level;
    if (level2 < 1) level2 = 1;
    if (level2 > 9) level2 = 9;

    if (options.MemSize == 0xFFFFFFFF)
      options.MemSize = (1 << (19 + (level2 > 8 ? 8 : level2)));

    if (options.Order == 0xFFFFFFFF)
      options.Order = 3 + level2;

    if (options.Algo == 0xFFFFFFFF)
      options.Algo = (level2 >= 7 ? 1 : 0);
  }

  return Update(
      EXTERNAL_CODECS_VARS
      m_Items, updateItems, outStream,
      m_Archive.IsOpen() ? &m_Archive : NULL, &options, callback);
  COM_TRY_END2
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  #ifndef _7ZIP_ST
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
        UString m = prop.bstrVal;
        m.MakeUpper();
        if (m == L"COPY") m_MainMethod = NFileHeader::NCompressionMethod::kStored;
        else if (m == L"DEFLATE") m_MainMethod = NFileHeader::NCompressionMethod::kDeflated;
        else if (m == L"DEFLATE64") m_MainMethod = NFileHeader::NCompressionMethod::kDeflated64;
        else if (m == L"BZIP2") m_MainMethod = NFileHeader::NCompressionMethod::kBZip2;
        else if (m == L"LZMA") m_MainMethod = NFileHeader::NCompressionMethod::kLZMA;
        else if (m == L"PPMD") m_MainMethod = NFileHeader::NCompressionMethod::kPPMd;
        else return E_INVALIDARG;
      }
      else if (prop.vt == VT_UI4)
      {
        switch(prop.ulVal)
        {
          case NFileHeader::NCompressionMethod::kStored:
          case NFileHeader::NCompressionMethod::kDeflated:
          case NFileHeader::NCompressionMethod::kDeflated64:
          case NFileHeader::NCompressionMethod::kBZip2:
          case NFileHeader::NCompressionMethod::kLZMA:
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
          m_ForceAesMode = true;
        }
        else if (valueString == L"ZIPCRYPTO")
        {
          m_IsAesMode = false;
          m_ForceAesMode = true;
        }
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
    else if (name.Left(3) == L"MEM")
    {
      UInt32 memSize = 1 << 24;
      RINOK(ParsePropDictionaryValue(name.Mid(3), prop, memSize));
      m_MemSize = memSize;
    }
    else if (name[0] == L'O')
    {
      UInt32 order = 8;
      RINOK(ParsePropValue(name.Mid(1), prop, order));
      m_Order = order;
    }
    else if (name.Left(4) == L"PASS")
    {
      UInt32 num = kDeflateNumPassesX9;
      RINOK(ParsePropValue(name.Mid(4), prop, num));
      m_NumPasses = num;
    }
    else if (name.Left(2) == L"FB")
    {
      UInt32 num = kDeflateNumFastBytesX9;
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
      #ifndef _7ZIP_ST
      RINOK(ParseMtProp(name.Mid(2), prop, numProcessors, _numThreads));
      #endif
    }
    else if (name.Left(1) == L"A")
    {
      UInt32 num = kLzAlgoX5;
      RINOK(ParsePropValue(name.Mid(1), prop, num));
      m_Algo = num;
    }
    else if (name.CompareNoCase(L"TC") == 0)
    {
      RINOK(SetBoolProperty(m_WriteNtfsTimeExtra, prop));
    }
    else if (name.CompareNoCase(L"CL") == 0)
    {
      RINOK(SetBoolProperty(m_ForceLocal, prop));
      if (m_ForceLocal)
        m_ForceUtf8 = false;
    }
    else if (name.CompareNoCase(L"CU") == 0)
    {
      RINOK(SetBoolProperty(m_ForceUtf8, prop));
      if (m_ForceUtf8)
        m_ForceLocal = false;
    }
    else
      return E_INVALIDARG;
  }
  return S_OK;
}

}}
