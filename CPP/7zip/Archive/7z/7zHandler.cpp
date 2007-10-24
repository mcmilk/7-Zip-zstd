// 7zHandler.cpp

#include "StdAfx.h"

#include "7zHandler.h"
#include "7zProperties.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/ComTry.h"
#include "../../../Windows/Defs.h"

#include "../Common/ItemNameUtils.h"
#ifdef _7Z_VOL
#include "../Common/MultiStream.h"
#endif

#ifdef __7Z_SET_PROPERTIES
#ifdef EXTRACT_ONLY
#include "../Common/ParseProperties.h"
#endif
#endif

#ifdef COMPRESS_MT
#include "../../../Windows/System.h"
#endif

using namespace NWindows;

extern UString ConvertMethodIdToString(UInt64 id);

namespace NArchive {
namespace N7z {

CHandler::CHandler()
{
  _crcSize = 4;

  #ifdef EXTRACT_ONLY
  #ifdef COMPRESS_MT
  _numThreads = NWindows::NSystem::GetNumberOfProcessors();
  #endif
  #else
  Init();
  #endif
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 
  #ifdef _7Z_VOL
  _refs.Size();
  #else
  *numItems = _database.Files.Size();
  #endif
  return S_OK;
}

#ifdef _SFX

IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::GetNumberOfProperties(UInt32 * /* numProperties */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CHandler::GetPropertyInfo(UInt32 /* index */,     
      BSTR * /* name */, PROPID * /* propID */, VARTYPE * /* varType */)
{
  return E_NOTIMPL;
}


#else

STATPROPSTG kArcProps[] = 
{
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidSolid, VT_BOOL},
  { NULL, kpidNumBlocks, VT_UI4}
};

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidMethod:
    {
      UString resString;
      CRecordVector<UInt64> ids;
      int i;
      for (i = 0; i < _database.Folders.Size(); i++)
      {
        const CFolder &f = _database.Folders[i];
        for (int j = f.Coders.Size() - 1; j >= 0; j--)
          ids.AddToUniqueSorted(f.Coders[j].MethodID);
      }

      for (i = 0; i < ids.Size(); i++)
      {
        UInt64 id = ids[i];
        UString methodName;
        /* bool methodIsKnown = */ FindMethod(EXTERNAL_CODECS_VARS id, methodName);
        if (methodName.IsEmpty())
          methodName = ConvertMethodIdToString(id);
        if (!resString.IsEmpty())
          resString += L' ';
        resString += methodName;
      }
      prop = resString; 
      break;
    }
    case kpidSolid: prop = _database.IsSolid(); break;
    case kpidNumBlocks: prop = (UInt32)_database.Folders.Size(); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

IMP_IInArchive_ArcProps

#endif

static void MySetFileTime(bool timeDefined, FILETIME unixTime, NWindows::NCOM::CPropVariant &prop)
{
  if (timeDefined)
    prop = unixTime;
}

#ifndef _SFX

static UString ConvertUInt32ToString(UInt32 value)
{
  wchar_t buffer[32];
  ConvertUInt64ToString(value, buffer);
  return buffer;
}

static UString GetStringForSizeValue(UInt32 value)
{
  for (int i = 31; i >= 0; i--)
    if ((UInt32(1) << i) == value)
      return ConvertUInt32ToString(i);
  UString result;
  if (value % (1 << 20) == 0)
  {
    result += ConvertUInt32ToString(value >> 20);
    result += L"m";
  }
  else if (value % (1 << 10) == 0)
  {
    result += ConvertUInt32ToString(value >> 10);
    result += L"k";
  }
  else
  {
    result += ConvertUInt32ToString(value);
    result += L"b";
  }
  return result;
}

static const UInt64 k_Copy = 0x0;
static const UInt64 k_LZMA  = 0x030101;
static const UInt64 k_PPMD  = 0x030401;

static wchar_t GetHex(Byte value)
{
  return (wchar_t)((value < 10) ? (L'0' + value) : (L'A' + (value - 10)));
}
static inline UString GetHex2(Byte value)
{
  UString result;
  result += GetHex((Byte)(value >> 4));
  result += GetHex((Byte)(value & 0xF));
  return result;
}

#endif

static const UInt64 k_AES  = 0x06F10701;

#ifndef _SFX
static inline UInt32 GetUInt32FromMemLE(const Byte *p)
{
  return p[0] | (((UInt32)p[1]) << 8) | (((UInt32)p[2]) << 16) | (((UInt32)p[3]) << 24);
}
#endif

bool CHandler::IsEncrypted(UInt32 index2) const
{
  CNum folderIndex = _database.FileIndexToFolderIndexMap[index2];
  if (folderIndex != kNumNoIndex)
  {
    const CFolder &folderInfo = _database.Folders[folderIndex];
    for (int i = folderInfo.Coders.Size() - 1; i >= 0; i--)
      if (folderInfo.Coders[i].MethodID == k_AES)
        return true;
  }
  return false;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  
  /*
  const CRef2 &ref2 = _refs[index];
  if (ref2.Refs.IsEmpty())
    return E_FAIL;
  const CRef &ref = ref2.Refs.Front();
  */
  
  #ifdef _7Z_VOL
  const CRef &ref = _refs[index];
  const CVolume &volume = _volumes[ref.VolumeIndex];
  const CArchiveDatabaseEx &_database = volume.Database;
  UInt32 index2 = ref.ItemIndex;
  const CFileItem &item = _database.Files[index2];
  #else
  const CFileItem &item = _database.Files[index];
  UInt32 index2 = index;
  #endif

  switch(propID)
  {
    case kpidPath:
    {
      if (!item.Name.IsEmpty())
        prop = NItemName::GetOSName(item.Name);
      break;
    }
    case kpidIsFolder:
      prop = item.IsDirectory;
      break;
    case kpidSize:
    {
      prop = item.UnPackSize;
      // prop = ref2.UnPackSize;
      break;
    }
    case kpidPosition:
    {
      /*
      if (ref2.Refs.Size() > 1)
        prop = ref2.StartPos;
      else
      */
        if (item.IsStartPosDefined)
          prop = item.StartPos;
      break;
    }
    case kpidPackedSize:
    {
      // prop = ref2.PackSize;
      {
        CNum folderIndex = _database.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          if (_database.FolderStartFileIndex[folderIndex] == (CNum)index2)
            prop = _database.GetFolderFullPackSize(folderIndex);
          /*
          else
            prop = (UInt64)0;
          */
        }
        else
          prop = (UInt64)0;
      }
      break;
    }
    case kpidLastAccessTime:
      MySetFileTime(item.IsLastAccessTimeDefined, item.LastAccessTime, prop);
      break;
    case kpidCreationTime:
      MySetFileTime(item.IsCreationTimeDefined, item.CreationTime, prop);
      break;
    case kpidLastWriteTime:
      MySetFileTime(item.IsLastWriteTimeDefined, item.LastWriteTime, prop);
      break;
    case kpidAttributes:
      if (item.AreAttributesDefined)
        prop = item.Attributes;
      break;
    case kpidCRC:
      if (item.IsFileCRCDefined)
        prop = item.FileCRC;
      break;
    case kpidEncrypted:
    {
      prop = IsEncrypted(index2);
      break;
    }
    #ifndef _SFX
    case kpidMethod:
      {
        CNum folderIndex = _database.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          const CFolder &folderInfo = _database.Folders[folderIndex];
          UString methodsString;
          for (int i = folderInfo.Coders.Size() - 1; i >= 0; i--)
          {
            const CCoderInfo &coderInfo = folderInfo.Coders[i];
            if (!methodsString.IsEmpty())
              methodsString += L' ';

            {
              UString methodName;
              bool methodIsKnown = FindMethod(
                  EXTERNAL_CODECS_VARS 
                  coderInfo.MethodID, methodName);

              if (methodIsKnown)
              {
                methodsString += methodName;
                if (coderInfo.MethodID == k_LZMA)
                {
                  if (coderInfo.Properties.GetCapacity() >= 5)
                  {
                    methodsString += L":";
                    UInt32 dicSize = GetUInt32FromMemLE(
                      ((const Byte *)coderInfo.Properties + 1));
                    methodsString += GetStringForSizeValue(dicSize);
                  }
                }
                else if (coderInfo.MethodID == k_PPMD)
                {
                  if (coderInfo.Properties.GetCapacity() >= 5)
                  {
                    Byte order = *(const Byte *)coderInfo.Properties;
                    methodsString += L":o";
                    methodsString += ConvertUInt32ToString(order);
                    methodsString += L":mem";
                    UInt32 dicSize = GetUInt32FromMemLE(
                      ((const Byte *)coderInfo.Properties + 1));
                    methodsString += GetStringForSizeValue(dicSize);
                  }
                }
                else if (coderInfo.MethodID == k_AES)
                {
                  if (coderInfo.Properties.GetCapacity() >= 1)
                  {
                    methodsString += L":";
                    const Byte *data = (const Byte *)coderInfo.Properties;
                    Byte firstByte = *data++;
                    UInt32 numCyclesPower = firstByte & 0x3F;
                    methodsString += ConvertUInt32ToString(numCyclesPower);
                    /*
                    if ((firstByte & 0xC0) != 0)
                    {
                      methodsString += L":";
                      return S_OK;
                      UInt32 saltSize = (firstByte >> 7) & 1;
                      UInt32 ivSize = (firstByte >> 6) & 1;
                      if (coderInfo.Properties.GetCapacity() >= 2)
                      {
                        Byte secondByte = *data++;
                        saltSize += (secondByte >> 4);
                        ivSize += (secondByte & 0x0F);
                      }
                    }
                    */
                  }
                }
                else
                {
                  if (coderInfo.Properties.GetCapacity() > 0)
                  {
                    methodsString += L":[";
                    for (size_t bi = 0; bi < coderInfo.Properties.GetCapacity(); bi++)
                    {
                      if (bi > 5 && bi + 1 < coderInfo.Properties.GetCapacity())
                      {
                        methodsString += L"..";
                        break;
                      }
                      else
                        methodsString += GetHex2(coderInfo.Properties[bi]);
                    }
                    methodsString += L"]";
                  }
                }
              }
              else
              {
                methodsString += ConvertMethodIdToString(coderInfo.MethodID);
              }
            }
          }
          prop = methodsString;
        }
      }
      break;
    case kpidBlock:
      {
        CNum folderIndex = _database.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
          prop = (UInt32)folderIndex;
      }
      break;
    case kpidPackedSize0:
    case kpidPackedSize1:
    case kpidPackedSize2:
    case kpidPackedSize3:
    case kpidPackedSize4:
      {
        CNum folderIndex = _database.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          const CFolder &folderInfo = _database.Folders[folderIndex];
          if (_database.FolderStartFileIndex[folderIndex] == (CNum)index2 &&
              folderInfo.PackStreams.Size() > (int)(propID - kpidPackedSize0))
          {
            prop = _database.GetFolderPackStreamSize(folderIndex, propID - kpidPackedSize0);
          }
          else
            prop = (UInt64)0;
        }
        else
          prop = (UInt64)0;
      }
      break;
    #endif
    case kpidIsAnti:
      prop = item.IsAnti;
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

#ifdef _7Z_VOL

static const wchar_t *kExt = L"7z";
static const wchar_t *kAfterPart = L".7z";

class CVolumeName
{
  bool _first;
  UString _unchangedPart;
  UString _changedPart;    
  UString _afterPart;    
public:
  bool InitName(const UString &name)
  {
    _first = true;
    int dotPos = name.ReverseFind('.');
    UString basePart = name;
    if (dotPos >= 0)
    {
      UString ext = name.Mid(dotPos + 1);
      if (ext.CompareNoCase(kExt)==0 || 
        ext.CompareNoCase(L"EXE") == 0)
      {
        _afterPart = kAfterPart;
        basePart = name.Left(dotPos);
      }
    }

    int numLetters = 1;
    bool splitStyle = false;
    if (basePart.Right(numLetters) == L"1")
    {
      while (numLetters < basePart.Length())
      {
        if (basePart[basePart.Length() - numLetters - 1] != '0')
          break;
        numLetters++;
      }
    }
    else 
      return false;
    _unchangedPart = basePart.Left(basePart.Length() - numLetters);
    _changedPart = basePart.Right(numLetters);
    return true;
  }

  UString GetNextName()
  {
    UString newName; 
    // if (_newStyle || !_first)
    {
      int i;
      int numLetters = _changedPart.Length();
      for (i = numLetters - 1; i >= 0; i--)
      {
        wchar_t c = _changedPart[i];
        if (c == L'9')
        {
          c = L'0';
          newName = c + newName;
          if (i == 0)
            newName = UString(L'1') + newName;
          continue;
        }
        c++;
        newName = UString(c) + newName;
        i--;
        for (; i >= 0; i--)
          newName = _changedPart[i] + newName;
        break;
      }
      _changedPart = newName;
    }
    _first = false;
    return _unchangedPart + _changedPart + _afterPart;
  }
};

#endif

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 *maxCheckStartPosition, 
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  #ifndef _SFX
  _fileInfoPopIDs.Clear();
  #endif
  try
  {
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackTemp = openArchiveCallback;
    #ifdef _7Z_VOL
    CVolumeName seqName;

    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    #endif

    #ifndef _NO_CRYPTO
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    if (openArchiveCallback)
    {
      openArchiveCallbackTemp.QueryInterface(
          IID_ICryptoGetTextPassword, &getTextPassword);
    }
    #endif
    #ifdef _7Z_VOL
    if (openArchiveCallback)
    {
      openArchiveCallbackTemp.QueryInterface(IID_IArchiveOpenVolumeCallback, &openVolumeCallback);
    }
    for (;;)
    {
      CMyComPtr<IInStream> inStream;
      if (!_volumes.IsEmpty())
      {
        if (!openVolumeCallback)
          break;
        if(_volumes.Size() == 1)
        {
          UString baseName;
          {
            NCOM::CPropVariant prop;
            RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
            if (prop.vt != VT_BSTR)
              break;
            baseName = prop.bstrVal;
          }
          seqName.InitName(baseName);
        }

        UString fullName = seqName.GetNextName();
        HRESULT result = openVolumeCallback->GetStream(fullName, &inStream);
        if (result == S_FALSE)
          break;
        if (result != S_OK)
          return result;
        if (!stream)
          break;
      }
      else
        inStream = stream;

      CInArchive archive;
      RINOK(archive.Open(inStream, maxCheckStartPosition));

      _volumes.Add(CVolume());
      CVolume &volume = _volumes.Back();
      CArchiveDatabaseEx &database = volume.Database;
      volume.Stream = inStream;
      volume.StartRef2Index = _refs.Size();

      HRESULT result = archive.ReadDatabase(database
          #ifndef _NO_CRYPTO
          , getTextPassword
          #endif
          );
      if (result != S_OK)
      {
        _volumes.Clear();
        return result;
      }
      database.Fill();
      for(int i = 0; i < database.Files.Size(); i++)
      {
        CRef refNew;
        refNew.VolumeIndex = _volumes.Size() - 1;
        refNew.ItemIndex = i;
        _refs.Add(refNew);
        /*
        const CFileItem &file = database.Files[i];
        int j;
        */
        /*
        for (j = _refs.Size() - 1; j >= 0; j--)
        {
          CRef2 &ref2 = _refs[j];
          const CRef &ref = ref2.Refs.Back();
          const CVolume &volume2 = _volumes[ref.VolumeIndex];
          const CArchiveDatabaseEx &database2 = volume2.Database;
          const CFileItem &file2 = database2.Files[ref.ItemIndex];
          if (file2.Name.CompareNoCase(file.Name) == 0)
          {
            if (!file.IsStartPosDefined)
              continue;
            if (file.StartPos != ref2.StartPos + ref2.UnPackSize)
              continue;
            ref2.Refs.Add(refNew);
            break;
          }
        }
        */
        /*
        j = -1;
        if (j < 0)
        {
          CRef2 ref2New;
          ref2New.Refs.Add(refNew);
          j = _refs.Add(ref2New);
        }
        CRef2 &ref2 = _refs[j];
        ref2.UnPackSize += file.UnPackSize;
        ref2.PackSize += database.GetFilePackSize(i);
        if (ref2.Refs.Size() == 1 && file.IsStartPosDefined)
          ref2.StartPos = file.StartPos;
        */
      }
      if (database.Files.Size() != 1)
        break;
      const CFileItem &file = database.Files.Front();
      if (!file.IsStartPosDefined)
        break;
    }
    #else
    CInArchive archive;
    RINOK(archive.Open(stream, maxCheckStartPosition));
    HRESULT result = archive.ReadDatabase(
      EXTERNAL_CODECS_VARS
      _database
      #ifndef _NO_CRYPTO
      , getTextPassword
      #endif
      );
    RINOK(result);
    _database.Fill();
    _inStream = stream;
    #endif
  }
  catch(...)
  {
    Close();
    return S_FALSE;
  }
  // _inStream = stream;
  #ifndef _SFX
  FillPopIDs();
  #endif
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  COM_TRY_BEGIN
  #ifdef _7Z_VOL
  _volumes.Clear();
  _refs.Clear();
  #else
  _inStream.Release();
  _database.Clear();
  #endif
  return S_OK;
  COM_TRY_END
}

#ifdef _7Z_VOL
STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  if (index != 0)
    return E_INVALIDARG;
  *stream = 0;
  CMultiStream *streamSpec = new CMultiStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  
  UInt64 pos = 0;
  const UString *fileName;
  for (int i = 0; i < _refs.Size(); i++)
  {
    const CRef &ref = _refs[i];
    const CVolume &volume = _volumes[ref.VolumeIndex];
    const CArchiveDatabaseEx &database = volume.Database;
    const CFileItem &file = database.Files[ref.ItemIndex];
    if (i == 0)
      fileName = &file.Name;
    else
      if (fileName->Compare(file.Name) != 0)
        return S_FALSE;
    if (!file.IsStartPosDefined)
      return S_FALSE;
    if (file.StartPos != pos)
      return S_FALSE;
    CNum folderIndex = database.FileIndexToFolderIndexMap[ref.ItemIndex];
    if (folderIndex == kNumNoIndex)
    {
      if (file.UnPackSize != 0)
        return E_FAIL;
      continue;
    }
    if (database.NumUnPackStreamsVector[folderIndex] != 1)
      return S_FALSE;
    const CFolder &folder = database.Folders[folderIndex];
    if (folder.Coders.Size() != 1)
      return S_FALSE;
    const CCoderInfo &coder = folder.Coders.Front();
    if (coder.NumInStreams != 1 || coder.NumOutStreams != 1)
      return S_FALSE;
    if (coder.MethodID != k_Copy)
      return S_FALSE;

    pos += file.UnPackSize;
    CMultiStream::CSubStreamInfo subStreamInfo;
    subStreamInfo.Stream = volume.Stream;
    subStreamInfo.Pos = database.GetFolderStreamPos(folderIndex, 0);
    subStreamInfo.Size = file.UnPackSize;
    streamSpec->Streams.Add(subStreamInfo);
  }
  streamSpec->Init();
  *stream = streamTemp.Detach();
  return S_OK;
}
#endif


#ifdef __7Z_SET_PROPERTIES
#ifdef EXTRACT_ONLY

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  COM_TRY_BEGIN
  #ifdef COMPRESS_MT
  const UInt32 numProcessors = NSystem::GetNumberOfProcessors();
  _numThreads = numProcessors;
  #endif

  for (int i = 0; i < numProperties; i++)
  {
    UString name = names[i];
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;
    const PROPVARIANT &value = values[i];
    UInt32 number;
    int index = ParseStringToUInt32(name, number);
    if (index == 0)
    {
      if(name.Left(2).CompareNoCase(L"MT") == 0)
      {
        #ifdef COMPRESS_MT
        RINOK(ParseMtProp(name.Mid(2), value, numProcessors, _numThreads));
        #endif
        continue;
      }
      else
        return E_INVALIDARG;
    }
  }
  return S_OK;
  COM_TRY_END
}  

#endif
#endif

IMPL_ISetCompressCodecsInfo

}}
