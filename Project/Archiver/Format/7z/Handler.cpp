// 7z/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "Common/Defs.h"
#include "Common/CRC.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"

#include "../../../Compress/Interface/CompressInterface.h"

#include "Archive/Common/ItemNameUtils.h"

using namespace std;

using namespace NArchive;
using namespace N7z;

using namespace NWindows;
using namespace NTime;

CHandler::CHandler()
{
  Init();
}

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  #ifndef _SFX
  COM_TRY_BEGIN
  CComObjectNoLock<CEnumArchiveItemProperty> *enumeratorSpec = 
      new CComObjectNoLock<CEnumArchiveItemProperty>;
  if (enumeratorSpec == NULL)
    return E_OUTOFMEMORY;
  CComPtr<IEnumSTATPROPSTG> tempEnumerator(enumeratorSpec);
  enumeratorSpec->Init(_database.ArchiveInfo.FileInfoPopIDs);
  *enumerator = tempEnumerator.Detach();
  return S_OK;
  // return tempEnumerator->QueryInterface(IID_IEnumSTATPROPSTG, (LPVOID*)enumerator);
  COM_TRY_END
  #else
    return E_NOTIMPL;
  #endif
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  COM_TRY_BEGIN
  *numItems = _database.Files.Size();
  return S_OK;
  COM_TRY_END
}

static void MySetFileTime(bool timeDefined, FILETIME unixTime, 
    NWindows::NCOM::CPropVariant &propVariant)
{
  // FILETIME fileTime;
  if (timeDefined)
    propVariant = unixTime;
    // NTime::UnixTimeToFileTime((time_t)unixTime, fileTime);
  else
  {
    return;
    // fileTime.dwHighDateTime = fileTime.dwLowDateTime = 0;
  }
  // propVariant = fileTime;
}

/*
inline static wchar_t GetHex(BYTE value)
{
  return (value < 10) ? ('0' + value) : ('A' + (value - 10));
}

static UString ConvertBytesToHexString(const BYTE *data, UINT32 size)
{
  UString result;
  for (UINT32 i = 0; i < size; i++)
  {
    BYTE b = data[i];
    result += GetHex(b >> 4);
    result += GetHex(b & 0xF);
  }
  return result;
}
*/


#ifndef _SFX

static UString ConvertUINT32ToString(UINT32 value)
{
  wchar_t buffer[32];
  ConvertUINT64ToString(value, buffer);
  return buffer;
}

static UString GetStringForSizeValue(UINT32 value)
{
  for (int i = 31; i >= 0; i--)
    if ((UINT32(1) << i) == value)
      return ConvertUINT32ToString(i);
  UString result;
  if (value % (1 << 20) == 0)
  {
    result += ConvertUINT32ToString(value >> 20);
    result += L"m";
  }
  else if (value % (1 << 10) == 0)
  {
    result += ConvertUINT32ToString(value >> 10);
    result += L"k";
  }
  else
  {
    result += ConvertUINT32ToString(value);
    result += L"b";
  }
  return result;
}

static CMethodID k_Copy  = { { 0x0 }, 1 };
static CMethodID k_LZMA  = { { 0x3, 0x1, 0x1 }, 3 };
static CMethodID k_BCJ   = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
static CMethodID k_BCJ2  = { { 0x3, 0x3, 0x1, 0x1B }, 4 };
static CMethodID k_PPMD  = { { 0x3, 0x4, 0x1 }, 3 };
static CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
static CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };

static inline char GetHex(BYTE value)
{
  return (value < 10) ? ('0' + value) : ('A' + (value - 10));
}
static inline UString GetHex2(BYTE value)
{
  UString result;
  result += GetHex(value >> 4);
  result += GetHex(value & 0xF);
  return result;
}

#endif

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const CFileItemInfo &item = _database.Files[index];

  switch(propID)
  {
    case kpidPath:
    {
      propVariant = NArchive::NItemName::GetOSName(item.Name);
      break;
    }
    case kpidIsFolder:
      propVariant = item.IsDirectory;
      break;
    case kpidSize:
      propVariant = item.UnPackSize;
      break;
    case kpidPackedSize:
    {
      {
        int folderIndex = _database.FileIndexToFolderIndexMap[index];
        if (folderIndex >= 0)
        {
          const CFolderItemInfo &folderInfo = _database.Folders[folderIndex];
          if (_database.FolderStartFileIndex[folderIndex] == index)
            propVariant = _database.GetFolderFullPackSize(folderIndex);
          else
            propVariant = UINT64(0);
        }
        else
          propVariant = UINT64(0);
      }
      break;
    }
    case kpidLastAccessTime:
      MySetFileTime(item.IsLastAccessTimeDefined, item.LastAccessTime, propVariant);
      break;
    case kpidCreationTime:
      MySetFileTime(item.IsCreationTimeDefined, item.CreationTime, propVariant);
      break;
    case kpidLastWriteTime:
      MySetFileTime(item.IsLastWriteTimeDefined, item.LastWriteTime, propVariant);
      break;
    case kpidAttributes:
      if (item.AreAttributesDefined)
        propVariant = item.Attributes;
      break;
    case kpidCRC:
      if (item.FileCRCIsDefined)
        propVariant = item.FileCRC;
      break;
    #ifndef _SFX
    case kpidMethod:
      {
        int folderIndex = _database.FileIndexToFolderIndexMap[index];
        if (folderIndex >= 0)
        {
          const CFolderItemInfo &folderInfo = _database.Folders[folderIndex];
          UString methodsString;
          for (int i = folderInfo.CodersInfo.Size() - 1; i >= 0; i--)
          {
            const CCoderInfo &coderInfo = folderInfo.CodersInfo[i];
            if (!methodsString.IsEmpty())
              methodsString += L' ';
            NRegistryInfo::CMethodInfo methodInfo;

            bool methodIsKnown;

            for (int j = 0; j < coderInfo.AltCoders.Size(); j++)
            {
              if (j > 0)
                methodsString += L"|";
              const CAltCoderInfo &altCoderInfo = coderInfo.AltCoders[j];

              UString methodName;
              #ifdef NO_REGISTRY

              methodIsKnown = true;
              if (altCoderInfo.DecompressionMethod == k_Copy)
                methodName = L"Copy";            
              else if (altCoderInfo.DecompressionMethod == k_LZMA)
                methodName = L"LZMA";
              else if (altCoderInfo.DecompressionMethod == k_BCJ)
                methodName = L"BCJ";
              else if (altCoderInfo.DecompressionMethod == k_BCJ2)
                methodName = L"BCJ2";
              else if (altCoderInfo.DecompressionMethod == k_PPMD)
                methodName = L"PPMD";
              else if (altCoderInfo.DecompressionMethod == k_Deflate)
                methodName = L"Deflate";
              else if (altCoderInfo.DecompressionMethod == k_BZip2)
                methodName = L"BZip2";
              else
                methodIsKnown = false;
              
              #else
            
              methodIsKnown = _methodMap.GetMethodInfoAlways(
                altCoderInfo.DecompressionMethod, methodInfo);
              methodName = GetUnicodeString(methodInfo.Name);
              
              #endif

              if (methodIsKnown)
              {
                methodsString += methodName;
                if (altCoderInfo.DecompressionMethod == k_LZMA)
                {
                  if (altCoderInfo.Properties.GetCapacity() == 5)
                  {
                    methodsString += L":";
                    UINT32 dicSize = *(const UINT32 *)
                      ((const BYTE *)altCoderInfo.Properties + 1);
                    methodsString += GetStringForSizeValue(dicSize);
                  }
                }
                else if (altCoderInfo.DecompressionMethod == k_PPMD)
                {
                  if (altCoderInfo.Properties.GetCapacity() == 5)
                  {
                    BYTE order = *(const BYTE *)altCoderInfo.Properties;
                    methodsString += L":o";
                    methodsString += ConvertUINT32ToString(order);
                    methodsString += L":mem";
                    UINT32 dicSize = *(const UINT32 *)
                      ((const BYTE *)altCoderInfo.Properties + 1);
                    methodsString += GetStringForSizeValue(dicSize);
                  }
                }
                else
                {
                  if (altCoderInfo.Properties.GetCapacity() > 0)
                  {
                    methodsString += L":[";
                    for (int bi = 0; bi < altCoderInfo.Properties.GetCapacity(); bi++)
                    {
                      if (bi > 2 && bi + 1 < altCoderInfo.Properties.GetCapacity())
                      {
                        methodsString += L"..";
                        break;
                      }
                      else
                        methodsString += GetHex2(altCoderInfo.Properties[bi]);
                    }
                    methodsString += L"]";
                  }
                }
              }
              else
              {
                methodsString += MultiByteToUnicodeString(altCoderInfo.DecompressionMethod.ConvertToString());
              }
            }
          }
          propVariant = methodsString;
        }
      }
      break;
    case kpidBlock:
      {
        int folderIndex = _database.FileIndexToFolderIndexMap[index];
        if (folderIndex >= 0)
          propVariant = (UINT32)folderIndex;
      }
      break;
    #endif
    case kpidPackedSize0:
    case kpidPackedSize1:
    case kpidPackedSize2:
    case kpidPackedSize3:
    case kpidPackedSize4:
      {
        int folderIndex = _database.FileIndexToFolderIndexMap[index];
        if (folderIndex >= 0)
        {
          const CFolderItemInfo &folderInfo = _database.Folders[folderIndex];
          if (_database.FolderStartFileIndex[folderIndex] == index &&
              folderInfo.PackStreams.Size() > propID - kpidPackedSize0)
          {
            propVariant = _database.GetFolderPackStreamSize(folderIndex, propID - kpidPackedSize0);
          }
          else
            propVariant = UINT64(0);
        }
        else
          propVariant = UINT64(0);
      }
      break;
    case kpidIsAnti:
      propVariant = item.IsAnti;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UINT64 *maxCheckStartPosition, 
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  _inStream.Release();
  _database.Clear();
  try
  {
    CInArchive archive;
    RETURN_IF_NOT_S_OK(archive.Open(stream, maxCheckStartPosition));

    CComPtr<ICryptoGetTextPassword> getTextPassword;
    if (openArchiveCallback)
    {
      CComPtr<IArchiveOpenCallback> openArchiveCallbackTemp = openArchiveCallback;
      openArchiveCallbackTemp.QueryInterface(&getTextPassword);
    }

    RETURN_IF_NOT_S_OK(archive.ReadDatabase(_database, getTextPassword));
    HRESULT result = archive.CheckIntegrity();
    if (result != S_OK)
      return E_FAIL;
    _database.FillFolderStartPackStream();
    _database.FillStartPos();
    _database.FillFolderStartFileIndex();
  }
  catch(...)
  {
    return S_FALSE;
  }
  _inStream = stream;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  COM_TRY_BEGIN
  _inStream.Release();
  return S_OK;
  COM_TRY_END
}
