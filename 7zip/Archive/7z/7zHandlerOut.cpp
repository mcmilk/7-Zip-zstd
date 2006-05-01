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
#include "../Common/ParseProperties.h"

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
#ifndef COMPRESS_DEFLATE_ENCODER
#define COMPRESS_DEFLATE_ENCODER
#endif
#endif

#ifdef COMPRESS_DEFLATE_ENCODER
static CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
#endif

#ifdef COMPRESS_BZIP2
#ifndef COMPRESS_BZIP2_ENCODER
#define COMPRESS_BZIP2_ENCODER
#endif
#endif

#ifdef COMPRESS_BZIP2_ENCODER
static CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };
#endif

const wchar_t *kCopyMethod = L"Copy";
const wchar_t *kLZMAMethodName = L"LZMA";
const wchar_t *kBZip2MethodName = L"BZip2";
const wchar_t *kPpmdMethodName = L"PPMd";
const wchar_t *kDeflateMethodName = L"Deflate";
const wchar_t *kDeflate64MethodName = L"Deflate64";

static const wchar_t *kLzmaMatchFinderX1 = L"HC4";
static const wchar_t *kLzmaMatchFinderX5 = L"BT4";

static const UInt32 kLzmaAlgorithmX1 = 0;
static const UInt32 kLzmaAlgorithmX5 = 1;

static const UInt32 kLzmaDicSizeX1 = 1 << 16;
static const UInt32 kLzmaDicSizeX3 = 1 << 20;
static const UInt32 kLzmaDicSizeX5 = 1 << 22;
static const UInt32 kLzmaDicSizeX7 = 1 << 24;
static const UInt32 kLzmaDicSizeX9 = 1 << 26;

static const UInt32 kLzmaFastBytesX1 = 32;
static const UInt32 kLzmaFastBytesX7 = 64;

static const UInt32 kPpmdMemSizeX1 = (1 << 22);
static const UInt32 kPpmdMemSizeX5 = (1 << 24);
static const UInt32 kPpmdMemSizeX7 = (1 << 26);
static const UInt32 kPpmdMemSizeX9 = (192 << 20);

static const UInt32 kPpmdOrderX1 = 4;
static const UInt32 kPpmdOrderX5 = 6;
static const UInt32 kPpmdOrderX7 = 16;
static const UInt32 kPpmdOrderX9 = 32;

static const UInt32 kDeflateFastBytesX1 = 32;
static const UInt32 kDeflateFastBytesX7 = 64;
static const UInt32 kDeflateFastBytesX9 = 128;

static const UInt32 kDeflatePassesX1 = 1;
static const UInt32 kDeflatePassesX7 = 3;
static const UInt32 kDeflatePassesX9 = 10;

static const UInt32 kBZip2NumPassesX1 = 1;
static const UInt32 kBZip2NumPassesX7 = 2;
static const UInt32 kBZip2NumPassesX9 = 7;

static const UInt32 kBZip2DicSizeX1 = 100000;
static const UInt32 kBZip2DicSizeX3 = 500000;
static const UInt32 kBZip2DicSizeX5 = 900000;

const wchar_t *kDefaultMethodName = kLZMAMethodName;

static const wchar_t *kLzmaMatchFinderForHeaders = L"BT2";
static const UInt32 kDictionaryForHeaders = 1 << 20;
static const UInt32 kNumFastBytesForHeaders = 273;
static const UInt32 kAlgorithmForHeaders = kLzmaAlgorithmX5;

static bool IsLZMAMethod(const UString &methodName)
  { return (methodName.CompareNoCase(kLZMAMethodName) == 0); }
static bool IsLZMethod(const UString &methodName)
  { return IsLZMAMethod(methodName); }

static bool IsBZip2Method(const UString &methodName)
  { return (methodName.CompareNoCase(kBZip2MethodName) == 0); }

static bool IsPpmdMethod(const UString &methodName)
  { return (methodName.CompareNoCase(kPpmdMethodName) == 0); }

static bool IsDeflateMethod(const UString &methodName)
  { return (methodName.CompareNoCase(kDeflateMethodName) == 0) || 
  (methodName.CompareNoCase(kDeflate64MethodName) == 0); }

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
  { NCoderPropID::kMatchFinderCycles, VT_UI4, L"mc" },
  { NCoderPropID::kAlgorithm, VT_UI4, L"a" },
  { NCoderPropID::kMatchFinder, VT_BSTR, L"mf" },
  { NCoderPropID::kNumThreads, VT_UI4, L"mt" }
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
  HRESULT res = SetCompressionMethod(methodMode, _methods
  #ifdef COMPRESS_MT
  , _numThreads
  #endif
  );
  RINOK(res);
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
      property.Value = kLzmaMatchFinderForHeaders;
      oneMethodInfo.CoderProperties.Add(property);
    }
    {
      CProperty property;
      property.PropID = NCoderPropID::kAlgorithm;
      property.Value = kAlgorithmForHeaders;
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
    HRESULT res = SetCompressionMethod(headerMethod, headerMethodInfoVector
      #ifdef COMPRESS_MT
      ,1
      #endif
    );
    RINOK(res);
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
    CObjectVector<COneMethodInfo> &methodsInfo
    #ifdef COMPRESS_MT
    , UInt32 numThreads
    #endif
    )
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

  UInt32 level = _level;
  
  for(int i = 0; i < methodsInfo.Size(); i++)
  {
    COneMethodInfo &oneMethodInfo = methodsInfo[i];
    if (oneMethodInfo.MethodName.IsEmpty())
      oneMethodInfo.MethodName = kDefaultMethodName;

    if (IsLZMAMethod(oneMethodInfo.MethodName))
    {
      UInt32 dicSize = _defaultDicSize;
      if (dicSize == 0xFFFFFFFF)
        dicSize = (level >= 9 ? kLzmaDicSizeX9 : 
                  (level >= 7 ? kLzmaDicSizeX7 : 
                  (level >= 5 ? kLzmaDicSizeX5 : 
                  (level >= 3 ? kLzmaDicSizeX3 : 
                                kLzmaDicSizeX1)))); 

      UInt32 algorithm = _defaultAlgorithm;
      if (algorithm == 0xFFFFFFFF)
        algorithm = (level >= 5 ? kLzmaAlgorithmX5 : 
                                  kLzmaAlgorithmX1); 

      UInt32 fastBytes = _defaultFastBytes;
      if (fastBytes == 0xFFFFFFFF)
        fastBytes = (level >= 7 ? kLzmaFastBytesX7 : 
                                  kLzmaFastBytesX1); 

      const wchar_t *matchFinder = 0;
      if (_defaultMatchFinder.IsEmpty())
        matchFinder = (level >= 5 ? kLzmaMatchFinderX5 : 
                                    kLzmaMatchFinderX1); 
      else
        matchFinder = (const wchar_t *)_defaultMatchFinder;

      SetOneMethodProp(oneMethodInfo, NCoderPropID::kDictionarySize, dicSize);
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kAlgorithm, algorithm);
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kNumFastBytes, fastBytes);
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kMatchFinder, matchFinder);
      #ifdef COMPRESS_MT
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kNumThreads, numThreads);
      #endif
    }
    else if (IsDeflateMethod(oneMethodInfo.MethodName))
    {
      UInt32 fastBytes = _defaultFastBytes;
      if (fastBytes == 0xFFFFFFFF)
        fastBytes = (level >= 9 ? kDeflateFastBytesX9 : 
                    (level >= 7 ? kDeflateFastBytesX7 : 
                                  kDeflateFastBytesX1));

      UInt32 numPasses = _defaultPasses;
      if (numPasses == 0xFFFFFFFF)
        numPasses = (level >= 9 ? kDeflatePassesX9 :  
                    (level >= 7 ? kDeflatePassesX7 : 
                                  kDeflatePassesX1));

      SetOneMethodProp(oneMethodInfo, NCoderPropID::kNumFastBytes, fastBytes);
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kNumPasses, numPasses);
    }
    else if (IsBZip2Method(oneMethodInfo.MethodName))
    {
      UInt32 numPasses = _defaultPasses;
      if (numPasses == 0xFFFFFFFF)
        numPasses = (level >= 9 ? kBZip2NumPassesX9 : 
                    (level >= 7 ? kBZip2NumPassesX7 :  
                                  kBZip2NumPassesX1));

      UInt32 dicSize = _defaultDicSize;
      if (dicSize == 0xFFFFFFFF)
        dicSize = (level >= 5 ? kBZip2DicSizeX5 : 
                  (level >= 3 ? kBZip2DicSizeX3 : 
                                kBZip2DicSizeX1));

      SetOneMethodProp(oneMethodInfo, NCoderPropID::kNumPasses, numPasses);
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kDictionarySize, dicSize);
      #ifdef COMPRESS_MT
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kNumThreads, numThreads);
      #endif
    }
    else if (IsPpmdMethod(oneMethodInfo.MethodName))
    {
      UInt32 useMemSize = _defaultPpmdMemSize;
      if (useMemSize == 0xFFFFFFFF)
        useMemSize = (level >= 9 ? kPpmdMemSizeX9 : 
                     (level >= 7 ? kPpmdMemSizeX7 : 
                     (level >= 5 ? kPpmdMemSizeX5 : 
                                   kPpmdMemSizeX1)));

      UInt32 order = _defaultPpmdOrder;
      if (order == 0xFFFFFFFF)
        order = (level >= 9 ? kPpmdOrderX9 : 
                (level >= 7 ? kPpmdOrderX7 : 
                (level >= 5 ? kPpmdOrderX5 : 
                              kPpmdOrderX1)));

      SetOneMethodProp(oneMethodInfo, NCoderPropID::kUsedMemorySize, useMemSize);
      SetOneMethodProp(oneMethodInfo, NCoderPropID::kOrder, order);
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

    #ifdef COMPRESS_DEFLATE_ENCODER
    if (oneMethodInfo.MethodName.CompareNoCase(L"Deflate") == 0)
    {
      defined = true;
      methodFull.MethodID = k_Deflate;
    }
    #endif

    #ifdef COMPRESS_BZIP2_ENCODER
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
  #ifdef COMPRESS_MT
  methodMode.NumThreads = _numThreads;
  headerMethod.NumThreads = 1;
  #endif

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
    RINOK(ParsePropDictionaryValue(value, dicSize));
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
    i += (int)(end - start);
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
  COM_TRY_BEGIN
  _methods.Clear();
  _binds.Clear();
  Init();
  #ifdef COMPRESS_MT
  const UInt32 numProcessors = NSystem::GetNumberOfProcessors();
  #endif

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
      RINOK(ParsePropValue(name, value, _level));
      if (_level == 0)
        _copyMode = true;
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
      if(name.Left(2).CompareNoCase(L"MT") == 0)
      {
        #ifdef COMPRESS_MT
        RINOK(ParseMtProp(name.Mid(2), value, numProcessors, _numThreads));
        #endif
        continue;
      }
      else if (name.CompareNoCase(L"RSFX") == 0)
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
      if (realName.Left(1).CompareNoCase(L"D") == 0)
      {
        UInt32 dicSize;
        RINOK(ParsePropDictionaryValue(realName.Mid(1), value, dicSize));
        property.PropID = NCoderPropID::kDictionarySize;
        property.Value = dicSize;
        oneMethodInfo.CoderProperties.Add(property);
      }
      else if (realName.Left(3).CompareNoCase(L"MEM") == 0)
      {
        UInt32 dicSize;
        RINOK(ParsePropDictionaryValue(realName.Mid(3), value, dicSize));
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
