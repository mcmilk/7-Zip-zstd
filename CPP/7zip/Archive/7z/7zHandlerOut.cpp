// 7zHandlerOut.cpp

#include "StdAfx.h"

#include "7zHandler.h"
#include "7zOut.h"
#include "7zUpdate.h"

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

static const wchar_t *kLZMAMethodName = L"LZMA";
static const wchar_t *kCopyMethod = L"Copy";
static const wchar_t *kDefaultMethodName = kLZMAMethodName;

static const UInt32 kLzmaAlgorithmX5 = 1;
static const wchar_t *kLzmaMatchFinderForHeaders = L"BT2";
static const UInt32 kDictionaryForHeaders = 1 << 20;
static const UInt32 kNumFastBytesForHeaders = 273;
static const UInt32 kAlgorithmForHeaders = kLzmaAlgorithmX5;

static inline bool IsCopyMethod(const UString &methodName)
  { return (methodName.CompareNoCase(kCopyMethod) == 0); }

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
    methodMode.PasswordIsDefined = IntToBool(passwordIsDefined);
    if (methodMode.PasswordIsDefined)
      methodMode.Password = password;
  }
  else
    methodMode.PasswordIsDefined = false;
  return S_OK;
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

  if (_compressHeaders)
  {
    // headerMethod.Methods.Add(methodMode.Methods.Back());

    CObjectVector<COneMethodInfo> headerMethodInfoVector;
    COneMethodInfo oneMethodInfo;
    oneMethodInfo.MethodName = kLZMAMethodName;
    {
      CProp property;
      property.Id = NCoderPropID::kMatchFinder;
      property.Value = kLzmaMatchFinderForHeaders;
      oneMethodInfo.Properties.Add(property);
    }
    {
      CProp property;
      property.Id = NCoderPropID::kAlgorithm;
      property.Value = kAlgorithmForHeaders;
      oneMethodInfo.Properties.Add(property);
    }
    {
      CProp property;
      property.Id = NCoderPropID::kNumFastBytes;
      property.Value = UInt32(kNumFastBytesForHeaders);
      oneMethodInfo.Properties.Add(property);
    }
    {
      CProp property;
      property.Id = NCoderPropID::kDictionarySize;
      property.Value = UInt32(kDictionaryForHeaders);
      oneMethodInfo.Properties.Add(property);
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

HRESULT CHandler::SetCompressionMethod(
    CCompressionMethodMode &methodMode,
    CObjectVector<COneMethodInfo> &methodsInfo
    #ifdef COMPRESS_MT
    , UInt32 numThreads
    #endif
    )
{
  UInt32 level = _level;
  
  if (methodsInfo.IsEmpty())
  {
    COneMethodInfo oneMethodInfo;
    oneMethodInfo.MethodName = ((level == 0) ? kCopyMethod : kDefaultMethodName);
    methodsInfo.Add(oneMethodInfo);
  }

  bool needSolid = false;
  for(int i = 0; i < methodsInfo.Size(); i++)
  {
    COneMethodInfo &oneMethodInfo = methodsInfo[i];
    SetCompressionMethod2(oneMethodInfo
      #ifdef COMPRESS_MT
      , numThreads
      #endif
      );

    if (!IsCopyMethod(oneMethodInfo.MethodName))
      needSolid = true;

    CMethodFull methodFull;

    if (!FindMethod(
        EXTERNAL_CODECS_VARS
        oneMethodInfo.MethodName, methodFull.Id, methodFull.NumInStreams, methodFull.NumOutStreams))
      return E_INVALIDARG;
    methodFull.Properties = oneMethodInfo.Properties;
    methodMode.Methods.Add(methodFull);

    if (!_numSolidBytesDefined)
    {
      for (int j = 0; j < methodFull.Properties.Size(); j++)
      {
        const CProp &prop = methodFull.Properties[j];
        if ((prop.Id == NCoderPropID::kDictionarySize || 
             prop.Id == NCoderPropID::kUsedMemorySize) && prop.Value.vt == VT_UI4)
        {
          _numSolidBytes = ((UInt64)prop.Value.ulVal) << 7;
          const UInt64 kMinSize = (1 << 24);
          if (_numSolidBytes < kMinSize)
            _numSolidBytes = kMinSize;
          _numSolidBytesDefined = true;
          break;
        }
      }
    }
  }

  if (!needSolid && !_numSolidBytesDefined)
  {
    _numSolidBytesDefined = true;
    _numSolidBytes  = 0;
  }
  return S_OK;
}

static HRESULT GetTime(IArchiveUpdateCallback *updateCallback, int index, PROPID propID, CArchiveFileTime &filetime, bool &filetimeIsDefined)
{
  filetimeIsDefined = false;
  NCOM::CPropVariant propVariant;
  RINOK(updateCallback->GetProperty(index, propID, &propVariant));
  if (propVariant.vt == VT_FILETIME)
  {
    filetime = propVariant.filetime;
    filetimeIsDefined = true;
  }
  else if (propVariant.vt != VT_EMPTY)
    return E_INVALIDARG;
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
      
      updateItem.CreationTime = fileItem.CreationTime;
      updateItem.IsCreationTimeDefined = fileItem.IsCreationTimeDefined;
      updateItem.LastWriteTime = fileItem.LastWriteTime;
      updateItem.IsLastWriteTimeDefined = fileItem.IsLastWriteTimeDefined;
      updateItem.LastAccessTime = fileItem.LastAccessTime;
      updateItem.IsLastAccessTimeDefined = fileItem.IsLastAccessTimeDefined;
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
      
      RINOK(GetTime(updateCallback, i, kpidCreationTime, updateItem.CreationTime, updateItem.IsCreationTimeDefined));
      RINOK(GetTime(updateCallback, i, kpidLastWriteTime, updateItem.LastWriteTime , updateItem.IsLastWriteTimeDefined));
      RINOK(GetTime(updateCallback, i, kpidLastAccessTime, updateItem.LastAccessTime, updateItem.IsLastAccessTimeDefined));

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

        updateItem.IsCreationTimeDefined = false;
        updateItem.IsLastWriteTimeDefined = false;
        updateItem.IsLastAccessTimeDefined = false;
        
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
    updateItems.Add(updateItem);
  }

  CCompressionMethodMode methodMode, headerMethod;
  RINOK(SetCompressionMethod(methodMode, headerMethod));
  #ifdef COMPRESS_MT
  methodMode.NumThreads = _numThreads;
  headerMethod.NumThreads = 1;
  #endif

  RINOK(SetPassword(methodMode, updateCallback));

  bool compressMainHeader = _compressHeaders;  // check it

  if (methodMode.PasswordIsDefined)
  {
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

  options.HeaderOptions.CompressMainHeader = compressMainHeader;
  options.HeaderOptions.WriteModified = WriteModified;
  options.HeaderOptions.WriteCreated = WriteCreated;
  options.HeaderOptions.WriteAccessed = WriteAccessed;
  
  options.NumSolidFiles = _numSolidFiles;
  options.NumSolidBytes = _numSolidBytes;
  options.SolidExtension = _solidExtension;
  options.RemoveSfxBlock = _removeSfxBlock;
  options.VolumeMode = _volumeMode;
  return Update(
      EXTERNAL_CODECS_VARS
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

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  COM_TRY_BEGIN
  _binds.Clear();
  BeforeSetProperty();

  for (int i = 0; i < numProperties; i++)
  {
    UString name = names[i];
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &value = values[i];

    if (name[0] == 'B')
    {
      name.Delete(0);
      CBind bind;
      RINOK(GetBindInfo(name, bind));
      _binds.Add(bind);
      continue;
    }

    RINOK(SetProperty(name, value));
  }

  return S_OK;
  COM_TRY_END
}  

}}
