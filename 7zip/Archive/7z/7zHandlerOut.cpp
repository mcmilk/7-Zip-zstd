// 7z/OutHandler.cpp

#include "StdAfx.h"

#include "7zHandler.h"
#include "7zOut.h"
#include "7zUpdate.h"
#include "7zMethods.h"

#include "../../../Windows/PropVariant.h"
#include "../../../Common/ComTry.h"
#include "../../../Common/StringToInt.h"
#include "../../IPassword.h"
#include "../../ICoder.h"

#include "../Common/ItemNameUtils.h"

using namespace NWindows;

namespace NArchive {
namespace N7z {

#ifdef COMPRESS_LZMA
static CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };
#endif

#ifdef COMPRESS_PPMD
static CMethodID k_PPMD = { { 0x3, 0x4, 0x1 }, 3 };
#endif

#ifdef COMPRESS_BCJ_X86
static CMethodID k_BCJ_X86 = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
#endif

#ifdef COMPRESS_BCJ2
static CMethodID k_BCJ2 = { { 0x3, 0x3, 0x1, 0x1B }, 4 };
#endif

#ifdef COMPRESS_COPY
static CMethodID k_Copy = { { 0x0 }, 1 };
#endif

#ifdef COMPRESS_DEFLATE
static CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
#endif

#ifdef COMPRESS_BZIP2
static CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };
#endif

const wchar_t *kCopyMethod = L"Copy";
const wchar_t *kLZMAMethodName = L"LZMA";

const UInt32 kAlgorithmForX7 = 2;
const UInt32 kDicSizeForX7 = 1 << 23;
const UInt32 kFastBytesForX7 = 64;

const UInt32 kAlgorithmForX9 = 2;
const UInt32 kDicSizeForX9 = 1 << 25;
const UInt32 kFastBytesForX9 = 64;
static const wchar_t *kMatchFinderForX9 = L"BT4b";

const UInt32 kAlgorithmForFast = 0;
const UInt32 kDicSizeForFast = 1 << 15;
static const wchar_t *kMatchFinderForFast = L"HC3";

const wchar_t *kDefaultMethodName = kLZMAMethodName;

static const wchar_t *kMatchFinderForHeaders = L"BT2";
static const UInt32 kDictionaryForHeaders = 1 << 20;
static const UInt32 kNumFastBytesForHeaders = 254;

static bool IsLZMAMethod(const UString &methodName)
  { return (methodName.CompareNoCase(kLZMAMethodName) == 0); }
static bool IsLZMethod(const UString &methodName)
  { return IsLZMAMethod(methodName); }

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kWindows;
  return S_OK;
}

HRESULT CHandler::SetPassword(CCompressionMethodMode &methodMode,
    IArchiveUpdateCallback *updateCallback)
{
  CMyComPtr<ICryptoGetTextPassword2> getTextPassword;
  if (!getTextPassword)
  {
    CMyComPtr<IArchiveUpdateCallback> udateCallback2(updateCallback);
    udateCallback2.QueryInterface(IID_ICryptoGetTextPassword2, &getTextPassword);
  }
  
  if (getTextPassword)
  {
    CMyComBSTR password;
    Int32 passwordIsDefined;
    RINOK(getTextPassword->CryptoGetTextPassword2(
        &passwordIsDefined, &password));
    if (methodMode.PasswordIsDefined = IntToBool(passwordIsDefined))
      methodMode.Password = password;
  }
  else
    methodMode.PasswordIsDefined = false;
  return S_OK;
}

struct CNameToPropID
{
  PROPID PropID;
  VARTYPE VarType;
  const wchar_t *Name;
};

CNameToPropID g_NameToPropID[] = 
{
  { NCoderPropID::kOrder, VT_UI4, L"O" },
  { NCoderPropID::kPosStateBits, VT_UI4, L"PB" },
  { NCoderPropID::kLitContextBits, VT_UI4, L"LC" },
  { NCoderPropID::kLitPosBits, VT_UI4, L"LP" },

  { NCoderPropID::kNumPasses, VT_UI4, L"Pass" },
  { NCoderPropID::kNumFastBytes, VT_UI4, L"fb" },
  { NCoderPropID::kAlgorithm, VT_UI4, L"a" },
  { NCoderPropID::kMatchFinder, VT_BSTR, L"mf" },
  { NCoderPropID::kMultiThread, VT_BOOL, L"mt" }
};

bool ConvertProperty(PROPVARIANT srcProp, VARTYPE varType, 
    NCOM::CPropVariant &destProp)
{
  if (varType == srcProp.vt)
  {
    destProp = srcProp;
    return true;
  }
  if (varType == VT_UI1)
  {
    if(srcProp.vt == VT_UI4)
    {
      UInt32 value = srcProp.ulVal;
      if (value > 0xFF)
        return false;
      destProp = Byte(value);
      return true;
    }
  }
  return false;
}
    
const int kNumNameToPropIDItems = sizeof(g_NameToPropID) / sizeof(g_NameToPropID[0]);

int FindPropIdFromStringName(const UString &name)
{
  for (int i = 0; i < kNumNameToPropIDItems; i++)
    if (name.CompareNoCase(g_NameToPropID[i].Name) == 0)
      return i;
  return -1;
}

HRESULT CHandler::SetCompressionMethod(
    CCompressionMethodMode &methodMode,
    CCompressionMethodMode &headerMethod)
{
  RINOK(SetCompressionMethod(methodMode, _methods, _multiThread));
  methodMode.Binds = _binds;
  if (_compressHeadersFull)
    _compressHeaders = true;

  if (_compressHeaders)
  {
    // headerMethod.Methods.Add(methodMode.Methods.Back());

    CObjectVector<COneMethodInfo> headerMethodInfoVector;
    COneMethodInfo oneMethodInfo;
    oneMethodInfo.MethodName = kLZMAMethodName;
    {
      CProperty property;
      property.PropID = NCoderPropID::kMatchFinder;
      property.Value = kMatchFinderForHeaders;
      oneMethodInfo.CoderProperties.Add(property);
    }
    {
      CProperty property;
      property.PropID = NCoderPropID::kAlgorithm;
      property.Value = kAlgorithmForX9;
      oneMethodInfo.CoderProperties.Add(property);
    }
    {
      CProperty property;
      property.PropID = NCoderPropID::kNumFastBytes;
      property.Value = UInt32(kNumFastBytesForHeaders);
      oneMethodInfo.CoderProperties.Add(property);
    }
    {
      CProperty property;
      property.PropID = NCoderPropID::kDictionarySize;
      property.Value = UInt32(kDictionaryForHeaders);
      oneMethodInfo.CoderProperties.Add(property);
    }
    headerMethodInfoVector.Add(oneMethodInfo);
    RINOK(SetCompressionMethod(headerMethod, headerMethodInfoVector, false));
  }
  return S_OK;
}

static void SetOneMethodProp(COneMethodInfo &oneMethodInfo, PROPID propID, 
    const NWindows::NCOM::CPropVariant &value)
{
  int j;
  for (j = 0; j < oneMethodInfo.CoderProperties.Size(); j++)
    if (oneMethodInfo.CoderProperties[j].PropID == propID)
      break;
  if (j != oneMethodInfo.CoderProperties.Size())
    return;
  CProperty property;
  property.PropID = propID;
  property.Value = value;
  oneMethodInfo.CoderProperties.Add(property);
}

HRESULT CHandler::SetCompressionMethod(
    CCompressionMethodMode &methodMode,
    CObjectVector<COneMethodInfo> &methodsInfo,
    bool multiThread)
{
  #ifndef EXCLUDE_COM
  /*
  CObjectVector<CMethodInfo2> methodInfoVector;
  if (!NRegistryInfo::EnumerateAllMethods(methodInfoVector))
    return E_FAIL;
  */
  #endif
 

  if (methodsInfo.IsEmpty())
  {
    COneMethodInfo oneMethodInfo;
    oneMethodInfo.MethodName = _copyMode ? kCopyMethod : kDefaultMethodName;
    methodsInfo.Add(oneMethodInfo);
  }

  for(int i = 0; i < methodsInfo.Size(); i++)
  {
    COneMethodInfo &oneMethodInfo = methodsInfo[i];
    if (oneMethodInfo.MethodName.IsEmpty())
      oneMethodInfo.MethodName = kDefaultMethodName;

    if (IsLZMethod(oneMethodInfo.MethodName))
    {
      if (IsLZMAMethod(oneMethodInfo.MethodName))
      {
        SetOneMethodProp(oneMethodInfo, 
            NCoderPropID::kDictionarySize, _defaultDicSize);
        SetOneMethodProp(oneMethodInfo, 
            NCoderPropID::kAlgorithm, _defaultAlgorithm);
        SetOneMethodProp(oneMethodInfo, 
            NCoderPropID::kNumFastBytes, _defaultFastBytes);
        SetOneMethodProp(oneMethodInfo, 
            NCoderPropID::kMatchFinder, (const wchar_t *)_defaultMatchFinder);
        if (multiThread)
          SetOneMethodProp(oneMethodInfo, 
              NCoderPropID::kMultiThread, true);
      }
    }
    CMethodFull methodFull;
    methodFull.NumInStreams = 1;
    methodFull.NumOutStreams = 1;

    bool defined = false;

    #ifdef COMPRESS_LZMA
    if (oneMethodInfo.MethodName.CompareNoCase(L"LZMA") == 0)
    {
      defined = true;
      methodFull.MethodID = k_LZMA;
    }
    #endif

    #ifdef COMPRESS_PPMD
    if (oneMethodInfo.MethodName.CompareNoCase(L"PPMD") == 0)
    {
      defined = true;
      methodFull.MethodID = k_PPMD;
    }
    #endif

    #ifdef COMPRESS_BCJ_X86
    if (oneMethodInfo.MethodName.CompareNoCase(L"BCJ") == 0)
    {
      defined = true;
      methodFull.MethodID = k_BCJ_X86;
    }
    #endif

    #ifdef COMPRESS_BCJ2
    if (oneMethodInfo.MethodName.CompareNoCase(L"BCJ2") == 0)
    {
      defined = true;
      methodFull.MethodID = k_BCJ2;
      methodFull.NumInStreams = 4;
      methodFull.NumOutStreams = 1;
    }
    #endif

    #ifdef COMPRESS_DEFLATE
    if (oneMethodInfo.MethodName.CompareNoCase(L"Deflate") == 0)
    {
      defined = true;
      methodFull.MethodID = k_Deflate;
    }
    #endif

    #ifdef COMPRESS_BZIP2
    if (oneMethodInfo.MethodName.CompareNoCase(L"BZip2") == 0)
    {
      defined = true;
      methodFull.MethodID = k_BZip2;
    }
    #endif

    #ifdef COMPRESS_COPY
    if (oneMethodInfo.MethodName.CompareNoCase(L"Copy") == 0)
    {
      defined = true;
      methodFull.MethodID = k_Copy;
    }

    #endif
    
    #ifdef EXCLUDE_COM
    
    if (defined)
    {
  
      methodFull.CoderProperties = oneMethodInfo.CoderProperties;
      methodMode.Methods.Add(methodFull);
      continue;
    }
    
    #else

    CMethodInfo2 methodInfo;
    if (!GetMethodInfo(oneMethodInfo.MethodName, methodInfo))
      return E_FAIL;
    if (!methodInfo.EncoderIsAssigned)
      return E_FAIL;

    methodFull.MethodID = methodInfo.MethodID;
    methodFull.NumInStreams = methodInfo.NumInStreams;
    methodFull.NumOutStreams = methodInfo.NumOutStreams;

    methodFull.EncoderClassID = methodInfo.Encoder;
    methodFull.FilePath = methodInfo.FilePath;
    methodFull.CoderProperties = oneMethodInfo.CoderProperties;
    defined = true;
    
    #endif
    if (!defined)
      return E_FAIL;
    
    methodMode.Methods.Add(methodFull);
  }
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  COM_TRY_BEGIN

  const CArchiveDatabaseEx *database = 0;
  #ifdef _7Z_VOL
  if(_volumes.Size() > 1)
    return E_FAIL;
  const CVolume *volume = 0;
  if (_volumes.Size() == 1)
  {
    volume = &_volumes.Front();
    database = &volume->Database;
  }
  #else
  if (_inStream != 0)
    database = &_database;
  #endif

  // CRecordVector<bool> compressStatuses;
  CObjectVector<CUpdateItem> updateItems;
  // CRecordVector<UInt32> copyIndices;
  
  // CMyComPtr<IUpdateCallback2> updateCallback2;
  // updateCallback->QueryInterface(&updateCallback2);

  int index = 0;
  for(UInt32 i = 0; i < numItems; i++)
  {
    Int32 newData;
    Int32 newProperties;
    UInt32 indexInArchive;
    if (!updateCallback)
      return E_FAIL;
    RINOK(updateCallback->GetUpdateItemInfo(i,
        &newData, &newProperties, &indexInArchive));
    CUpdateItem updateItem;
    updateItem.NewProperties = IntToBool(newProperties);
    updateItem.NewData = IntToBool(newData);
    updateItem.IndexInArchive = indexInArchive;
    updateItem.IndexInClient = i;
    updateItem.IsAnti = false;
    updateItem.Size = 0;

    if (updateItem.IndexInArchive != -1)
    {
      const CFileItem &fileItem = database->Files[updateItem.IndexInArchive];
      updateItem.Name = fileItem.Name;
      updateItem.IsDirectory = fileItem.IsDirectory;
      updateItem.Size = fileItem.UnPackSize;
      updateItem.IsAnti = fileItem.IsAnti;
      updateItem.LastWriteTime = fileItem.LastWriteTime;
      updateItem.LastWriteTimeIsDefined = fileItem.IsLastWriteTimeDefined;
    }

    if (updateItem.NewProperties)
    {
      bool nameIsDefined;
      bool folderStatusIsDefined;
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidAttributes, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          updateItem.AttributesAreDefined = false;
        else if (propVariant.vt != VT_UI4)
          return E_INVALIDARG;
        else
        {
          updateItem.Attributes = propVariant.ulVal;
          updateItem.AttributesAreDefined = true;
        }
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidCreationTime, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          updateItem.CreationTimeIsDefined = false;
        else if (propVariant.vt != VT_FILETIME)
          return E_INVALIDARG;
        else
        {
          updateItem.CreationTime = propVariant.filetime;
          updateItem.CreationTimeIsDefined = true;
        }
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidLastWriteTime, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          updateItem.LastWriteTimeIsDefined = false;
        else if (propVariant.vt != VT_FILETIME)
          return E_INVALIDARG;
        else
        {
          updateItem.LastWriteTime = propVariant.filetime;
          updateItem.LastWriteTimeIsDefined = true;
        }
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidPath, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          nameIsDefined = false;
        else if (propVariant.vt != VT_BSTR)
          return E_INVALIDARG;
        else
        {
          updateItem.Name = NItemName::MakeLegalName(propVariant.bstrVal);
          nameIsDefined = true;
        }
      }
      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidIsFolder, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          folderStatusIsDefined = false;
        else if (propVariant.vt != VT_BOOL)
          return E_INVALIDARG;
        else
        {
          updateItem.IsDirectory = (propVariant.boolVal != VARIANT_FALSE);
          folderStatusIsDefined = true;
        }
      }

      {
        NCOM::CPropVariant propVariant;
        RINOK(updateCallback->GetProperty(i, kpidIsAnti, &propVariant));
        if (propVariant.vt == VT_EMPTY)
          updateItem.IsAnti = false;
        else if (propVariant.vt != VT_BOOL)
          return E_INVALIDARG;
        else
          updateItem.IsAnti = (propVariant.boolVal != VARIANT_FALSE);
      }

      if (updateItem.IsAnti)
      {
        updateItem.AttributesAreDefined = false;
        updateItem.CreationTimeIsDefined = false;
        updateItem.LastWriteTimeIsDefined = false;
        updateItem.Size = 0;
      }

      if (!folderStatusIsDefined && updateItem.AttributesAreDefined)
        updateItem.SetDirectoryStatusFromAttributes();
    }

    if (updateItem.NewData)
    {
      NCOM::CPropVariant propVariant;
      RINOK(updateCallback->GetProperty(i, kpidSize, &propVariant));
      if (propVariant.vt != VT_UI8)
        return E_INVALIDARG;
      updateItem.Size = (UInt64)propVariant.uhVal.QuadPart;
      if (updateItem.Size != 0 && updateItem.IsAnti)
        return E_INVALIDARG;
    }
    /*
    else
      thereIsCopyData = true;
    */

    updateItems.Add(updateItem);
  }

  /*
  if (thereIsCopyData)
  {
    for(int i = 0; i < _database.NumUnPackStreamsVector.Size(); i++)
      if (_database.NumUnPackStreamsVector[i] != 1)
        return E_NOTIMPL;
    if (!_solidIsSpecified)
      _solid = false;
    if (_solid)
      return E_NOTIMPL;
  }
  */


  CCompressionMethodMode methodMode, headerMethod;
  RINOK(SetCompressionMethod(methodMode, headerMethod));
  methodMode.MultiThread = _multiThread;
  // methodMode.MultiThreadMult = _multiThreadMult;

  headerMethod.MultiThread = false;
  // headerMethod.MultiThreadMult = _multiThreadMult;

  RINOK(SetPassword(methodMode, updateCallback));

  bool useAdditionalHeaderStreams = true;
  bool compressMainHeader = false; 

  if (_compressHeadersFull)
  {
    useAdditionalHeaderStreams = false;
    compressMainHeader = true; 
  }
  if (methodMode.PasswordIsDefined)
  {
    useAdditionalHeaderStreams = false;
    compressMainHeader = true; 
    if(_encryptHeaders)
      RINOK(SetPassword(headerMethod, updateCallback));
  }

  if (numItems < 2)
    compressMainHeader = false;

  CUpdateOptions options;
  options.Method = &methodMode;
  options.HeaderMethod = (_compressHeaders || 
      (methodMode.PasswordIsDefined && _encryptHeaders)) ? 
      &headerMethod : 0;
  options.UseFilters = _level != 0 && _autoFilter;
  options.MaxFilter = _level >= 8;
  options.UseAdditionalHeaderStreams = useAdditionalHeaderStreams;
  options.CompressMainHeader = compressMainHeader;
  options.NumSolidFiles = _numSolidFiles;
  options.NumSolidBytes = _numSolidBytes;
  options.SolidExtension = _solidExtension;
  options.RemoveSfxBlock = _removeSfxBlock;
  options.VolumeMode = _volumeMode;
  return Update(
      #ifdef _7Z_VOL
      volume ? volume->Stream: 0, 
      volume ? database: 0, 
      #else
      _inStream, 
      database,
      #endif
      updateItems, outStream, updateCallback, options);
  COM_TRY_END
}

static int ParseStringToUInt32(const UString &srcString, UInt32 &number)
{
  const wchar_t *start = srcString;
  const wchar_t *end;
  UInt64 number64 = ConvertStringToUInt64(start, &end);
  if (number64 > 0xFFFFFFFF) 
  {
    number = 0;
    return 0;
  }
  number = (UInt32)number64;
  return end - start;
}

static const int kLogarithmicSizeLimit = 32;
static const char kByteSymbol = 'B';
static const char kKiloByteSymbol = 'K';
static const char kMegaByteSymbol = 'M';

HRESULT ParseDictionaryValues(const UString &srcStringSpec, UInt32 &dicSize)
{
  UString srcString = srcStringSpec;
  srcString.MakeUpper();

  const wchar_t *start = srcString;
  const wchar_t *end;
  UInt64 number = ConvertStringToUInt64(start, &end);
  int numDigits = end - start;
  if (numDigits == 0 || srcString.Length() > numDigits + 1)
    return E_INVALIDARG;
  if (srcString.Length() == numDigits)
  {
    if (number >= kLogarithmicSizeLimit)
      return E_INVALIDARG;
    dicSize = (UInt32)1 << (int)number;
    return S_OK;
  }
  switch (srcString[numDigits])
  {
    case kByteSymbol:
      if (number >= ((UInt64)1 << kLogarithmicSizeLimit))
        return E_INVALIDARG;
      dicSize = (UInt32)number;
      break;
    case kKiloByteSymbol:
      if (number >= ((UInt64)1 << (kLogarithmicSizeLimit - 10)))
        return E_INVALIDARG;
      dicSize = UInt32(number << 10);
      break;
    case kMegaByteSymbol:
      if (number >= ((UInt64)1 << (kLogarithmicSizeLimit - 20)))
        return E_INVALIDARG;
      dicSize = UInt32(number << 20);
      break;
    default:
      return E_INVALIDARG;
  }
  return S_OK;
}

static inline UINT GetCurrentFileCodePage()
{
  return AreFileApisANSI() ? CP_ACP : CP_OEMCP;
}

static HRESULT SetBoolProperty(bool &dest, const PROPVARIANT &value)
{
  switch(value.vt)
  {
    case VT_EMPTY:
      dest = true;
      break;
    /*
    case VT_UI4:
      dest = (value.ulVal != 0);
      break;
    */
    case VT_BSTR:
    {
      UString valueString = value.bstrVal;
      valueString.MakeUpper();
      if (valueString.Compare(L"ON") == 0)
        dest = true;
      else if (valueString.Compare(L"OFF") == 0)
        dest = false;
      else
        return E_INVALIDARG;
      break;
    }
    default:
      return E_INVALIDARG;
  }
  return S_OK;
}

/*
static HRESULT SetComplexProperty(bool &boolStatus, UInt32 &number, 
    const PROPVARIANT &value)
{
  switch(value.vt)
  {
    case VT_EMPTY:
    case VT_BSTR:
    {
      RINOK(SetBoolProperty(boolStatus, value));
      return S_OK;
    }
    case VT_UI4:
      boolStatus = true;
      number = (value.ulVal);
      break;
    default:
      return E_INVALIDARG;
  }
  return S_OK;
}
*/

static HRESULT GetBindInfoPart(UString &srcString, UInt32 &coder, UInt32 &stream)
{
  stream = 0;
  int index = ParseStringToUInt32(srcString, coder);
  if (index == 0)
    return E_INVALIDARG;
  srcString.Delete(0, index);
  if (srcString[0] == 'S')
  {
    srcString.Delete(0);
    int index = ParseStringToUInt32(srcString, stream);
    if (index == 0)
      return E_INVALIDARG;
    srcString.Delete(0, index);
  }
  return S_OK;
}

static HRESULT GetBindInfo(UString &srcString, CBind &bind)
{
  RINOK(GetBindInfoPart(srcString, bind.OutCoder, bind.OutStream));
  if (srcString[0] != ':')
    return E_INVALIDARG;
  srcString.Delete(0);
  RINOK(GetBindInfoPart(srcString, bind.InCoder, bind.InStream));
  if (!srcString.IsEmpty())
    return E_INVALIDARG;
  return S_OK;
}

static void SplitParams(const UString &srcString, UStringVector &subStrings)
{
  subStrings.Clear();
  UString name;
  int len = srcString.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
  {
    wchar_t c = srcString[i];
    if (c == L':')
    {
      subStrings.Add(name);
      name.Empty();
    }
    else
      name += c;
  }
  subStrings.Add(name);
}

static void SplitParam(const UString &param, UString &name, UString &value)
{
  int eqPos = param.Find(L'=');
  if (eqPos >= 0)
  {
    name = param.Left(eqPos);
    value = param.Mid(eqPos + 1);
    return;
  }
  for(int i = 0; i < param.Length(); i++)
  {
    wchar_t c = param[i];
    if (c >= L'0' && c <= L'9')
    {
      name = param.Left(i);
      value = param.Mid(i);
      return;
    }
  }
  name = param;
}

HRESULT CHandler::SetParam(COneMethodInfo &oneMethodInfo, const UString &name, const UString &value)
{
  CProperty property;
  if (name.CompareNoCase(L"D") == 0 || name.CompareNoCase(L"MEM") == 0)
  {
    UInt32 dicSize;
    RINOK(ParseDictionaryValues(value, dicSize));
    if (name.CompareNoCase(L"D") == 0)
      property.PropID = NCoderPropID::kDictionarySize;
    else
      property.PropID = NCoderPropID::kUsedMemorySize;
    property.Value = dicSize;
    oneMethodInfo.CoderProperties.Add(property);
  }
  else
  {
    int index = FindPropIdFromStringName(name);
    if (index < 0)
      return E_INVALIDARG;
    
    const CNameToPropID &nameToPropID = g_NameToPropID[index];
    property.PropID = nameToPropID.PropID;
    
    NCOM::CPropVariant propValue;
    
    
    if (nameToPropID.VarType == VT_BSTR)
      propValue = value;
    else
    {
      UInt32 number;
      if (ParseStringToUInt32(value, number) == value.Length())
        propValue = number;
      else
        propValue = value;
    }
    
    if (!ConvertProperty(propValue, nameToPropID.VarType, property.Value))
      return E_INVALIDARG;
    
    oneMethodInfo.CoderProperties.Add(property);
  }
  return S_OK;
}

HRESULT CHandler::SetParams(COneMethodInfo &oneMethodInfo, const UString &srcString)
{
  UStringVector params;
  SplitParams(srcString, params);
  if (params.Size() > 0)
    oneMethodInfo.MethodName = params[0];
  for (int i = 1; i < params.Size(); i++)
  {
    const UString &param = params[i];
    UString name, value;
    SplitParam(param, name, value);
    RINOK(SetParam(oneMethodInfo, name, value));
  }
  return S_OK;
}

HRESULT CHandler::SetSolidSettings(const UString &s)
{
  UString s2 = s;
  s2.MakeUpper();
  if (s2.IsEmpty() || s2.Compare(L"ON") == 0)
  {
    InitSolid();
    return S_OK;
  }
  if (s2.Compare(L"OFF") == 0)
  {
    _numSolidFiles = 1;
    return S_OK;
  }
  for (int i = 0; i < s2.Length();)
  {
    const wchar_t *start = ((const wchar_t *)s2) + i;
    const wchar_t *end;
    UInt64 v = ConvertStringToUInt64(start, &end);
    if (start == end)
    {
      if (s2[i++] != 'E')
        return E_INVALIDARG;
      _solidExtension = true;
      continue;
    }
    i += end - start;
    if (i == s2.Length())
      return E_INVALIDARG;
    wchar_t c = s2[i++];
    switch(c)
    {
      case 'F':
        if (v < 1)
          v = 1;
        _numSolidFiles = v;
        break;
      case 'B':
        _numSolidBytes = v;
        _numSolidBytesDefined = true;
        break;
      case 'K':
        _numSolidBytes = (v << 10);
        _numSolidBytesDefined = true;
        break;
      case 'M':
        _numSolidBytes = (v << 20);
        _numSolidBytesDefined = true;
        break;
      case 'G':
        _numSolidBytes = (v << 30);
        _numSolidBytesDefined = true;
        break;
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

HRESULT CHandler::SetSolidSettings(const PROPVARIANT &value)
{
  switch(value.vt)
  {
    case VT_EMPTY:
      InitSolid();
      return S_OK;
    case VT_BSTR:
      return SetSolidSettings(value.bstrVal);
    default:
      return E_INVALIDARG;
  }
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  UINT codePage = GetCurrentFileCodePage();
  COM_TRY_BEGIN
  _methods.Clear();
  _binds.Clear();
  Init();
  UInt32 minNumber = 0;

  for (int i = 0; i < numProperties; i++)
  {
    UString name = names[i];
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &value = values[i];

    if (name[0] == 'X')
    {
      name.Delete(0);
      _level = 9;
      if (value.vt == VT_UI4)
      {
        if (!name.IsEmpty())
          return E_INVALIDARG;
        _level = value.ulVal;
      }
      else if (value.vt == VT_EMPTY)
      {
        if(!name.IsEmpty())
        {
          int index = ParseStringToUInt32(name, _level);
          if (index != name.Length())
            return E_INVALIDARG;
        }
      }
      else
        return E_INVALIDARG;
      if (_level == 0)
      {
        _copyMode = true;
      }
      else if (_level < 5)
      {
        _defaultAlgorithm = kAlgorithmForFast;
        _defaultDicSize = kDicSizeForFast;
        _defaultMatchFinder = kMatchFinderForFast;
      }
      else if (_level < 7)
      {
        // normal;
      }
      else if(_level < 9)
      {
        _defaultAlgorithm = kAlgorithmForX7;
        _defaultDicSize = kDicSizeForX7;
        _defaultFastBytes = kFastBytesForX7;
      }
      else
      {
        _defaultAlgorithm = kAlgorithmForX9;
        _defaultDicSize = kDicSizeForX9;
        _defaultFastBytes = kFastBytesForX9;
        _defaultMatchFinder = kMatchFinderForX9;
      }
      continue;
    }

    if (name[0] == 'B')
    {
      name.Delete(0);
      CBind bind;
      RINOK(GetBindInfo(name, bind));
      _binds.Add(bind);
      continue;
    }

    if (name[0] == L'S')
    {
      name.Delete(0);
      if (name.IsEmpty())
      {
        RINOK(SetSolidSettings(value));
      }
      else
      {
        RINOK(SetSolidSettings(name));
      }
      continue;
    }
    
      
    UInt32 number;
    int index = ParseStringToUInt32(name, number);
    UString realName = name.Mid(index);
    if (index == 0)
    {
      if (name.CompareNoCase(L"RSFX") == 0)
      {
        RINOK(SetBoolProperty(_removeSfxBlock, value));
        continue;
      }
      else if (name.CompareNoCase(L"F") == 0)
      {
        RINOK(SetBoolProperty(_autoFilter, value));
        continue;
      }
      else if (name.CompareNoCase(L"HC") == 0)
      {
        RINOK(SetBoolProperty(_compressHeaders, value));
        continue;
      }
      else if (name.CompareNoCase(L"HCF") == 0)
      {
        RINOK(SetBoolProperty(_compressHeadersFull, value));
        continue;
      }
      else if (name.CompareNoCase(L"HE") == 0)
      {
        RINOK(SetBoolProperty(_encryptHeaders, value));
        continue;
      }
      else if (name.CompareNoCase(L"MT") == 0)
      {
        // _multiThreadMult = 200;
        RINOK(SetBoolProperty(_multiThread, value));
        // RINOK(SetComplexProperty(MultiThread, _multiThreadMult, value));
        continue;
      }
      else if (name.CompareNoCase(L"V") == 0)
      {
        RINOK(SetBoolProperty(_volumeMode, value));
        continue;
      }
      number = 0;
    }
    if (number > 10000)
      return E_FAIL;
    if (number < minNumber)
      return E_INVALIDARG;
    number -= minNumber;
    for(int j = _methods.Size(); j <= (int)number; j++)
    {
      COneMethodInfo oneMethodInfo;
      _methods.Add(oneMethodInfo);
    }

    COneMethodInfo &oneMethodInfo = _methods[number];

    if (realName.Length() == 0)
    {
      if (value.vt != VT_BSTR)
        return E_INVALIDARG;
      
      // oneMethodInfo.MethodName = UnicodeStringToMultiByte(UString(value.bstrVal));
      RINOK(SetParams(oneMethodInfo, value.bstrVal));
    }
    else
    {
      CProperty property;
      if (realName.CompareNoCase(L"D") == 0 || realName.CompareNoCase(L"MEM") == 0)
      {
        UInt32 dicSize;
        if (value.vt == VT_UI4)
        {
          UInt32 logDicSize = value.ulVal;
          if (logDicSize >= 32)
            return E_INVALIDARG;
          dicSize = (UInt32)1 << logDicSize;
        }
        else if (value.vt == VT_BSTR)
        {
          RINOK(ParseDictionaryValues(value.bstrVal, dicSize));
        }
        else 
          return E_FAIL;
        if (realName.CompareNoCase(L"D") == 0)
          property.PropID = NCoderPropID::kDictionarySize;
        else
          property.PropID = NCoderPropID::kUsedMemorySize;
        property.Value = dicSize;
        oneMethodInfo.CoderProperties.Add(property);
      }
      else
      {
        int index = FindPropIdFromStringName(realName);
        if (index < 0)
          return E_INVALIDARG;
        
        const CNameToPropID &nameToPropID = g_NameToPropID[index];
        property.PropID = nameToPropID.PropID;
        
        if (!ConvertProperty(value, nameToPropID.VarType, property.Value))
          return E_INVALIDARG;
        
        oneMethodInfo.CoderProperties.Add(property);
      }
    }
  }
  CheckAndSetSolidBytesLimit();

  return S_OK;
  COM_TRY_END
}  

}}
