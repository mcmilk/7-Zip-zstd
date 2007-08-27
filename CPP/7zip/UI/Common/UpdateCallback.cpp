// UpdateCallback.cpp

#include "StdAfx.h"

#include "UpdateCallback.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"
#include "Common/Defs.h"
#include "Common/ComTry.h"

#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

using namespace NWindows;

CArchiveUpdateCallback::CArchiveUpdateCallback():
  Callback(0),
  ShareForWrite(false),
  StdInMode(false),
  DirItems(0),
  ArchiveItems(0),
  UpdatePairs(0)
  {}


STDMETHODIMP CArchiveUpdateCallback::SetTotal(UInt64 size)
{
  COM_TRY_BEGIN
  return Callback->SetTotal(size);
  COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const UInt64 *completeValue)
{
  COM_TRY_BEGIN
  return Callback->SetCompleted(completeValue);
  COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  COM_TRY_BEGIN
  return Callback->SetRatioInfo(inSize, outSize);
  COM_TRY_END
}


/*
STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidLastAccessTime, VT_FILETIME},
  { NULL, kpidCreationTime, VT_FILETIME},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},
  { NULL, kpidIsAnti, VT_BOOL}
};
*/

STDMETHODIMP CArchiveUpdateCallback::EnumProperties(IEnumSTATPROPSTG **)
{
  return E_NOTIMPL;
  /*
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  */
}

STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(UInt32 index, 
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive)
{
  COM_TRY_BEGIN
  RINOK(Callback->CheckBreak());
  const CUpdatePair2 &updatePair = (*UpdatePairs)[index];
  if(newData != NULL)
    *newData = BoolToInt(updatePair.NewData);
  if(newProperties != NULL)
    *newProperties = BoolToInt(updatePair.NewProperties);
  if(indexInArchive != NULL)
  {
    if (updatePair.ExistInArchive)
    {
      if (ArchiveItems == 0)
        *indexInArchive = updatePair.ArchiveItemIndex;
      else
        *indexInArchive = (*ArchiveItems)[updatePair.ArchiveItemIndex].IndexInServer;
    }
    else
      *indexInArchive = UInt32(-1);
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  const CUpdatePair2 &updatePair = (*UpdatePairs)[index];
  NWindows::NCOM::CPropVariant propVariant;
  
  if (propID == kpidIsAnti)
  {
    propVariant = updatePair.IsAnti;
    propVariant.Detach(value);
    return S_OK;
  }

  if (updatePair.IsAnti)
  {
    switch(propID)
    {
      case kpidIsFolder:
      case kpidPath:
        break;
      case kpidSize:
        propVariant = (UInt64)0;
        propVariant.Detach(value);
        return S_OK;
      default:
        propVariant.Detach(value);
        return S_OK;
    }
  }
  
  if(updatePair.ExistOnDisk)
  {
    const CDirItem &dirItem = (*DirItems)[updatePair.DirItemIndex];
    switch(propID)
    {
      case kpidPath:
        propVariant = dirItem.Name;
        break;
      case kpidIsFolder:
        propVariant = dirItem.IsDirectory();
        break;
      case kpidSize:
        propVariant = dirItem.Size;
        break;
      case kpidAttributes:
        propVariant = dirItem.Attributes;
        break;
      case kpidLastAccessTime:
        propVariant = dirItem.LastAccessTime;
        break;
      case kpidCreationTime:
        propVariant = dirItem.CreationTime;
        break;
      case kpidLastWriteTime:
        propVariant = dirItem.LastWriteTime;
        break;
    }
  }
  else
  {
    if (propID == kpidPath)
    {
      if (updatePair.NewNameIsDefined)
      {
        propVariant = updatePair.NewName;
        propVariant.Detach(value);
        return S_OK;
      }
    }
    if (updatePair.ExistInArchive && Archive)
    {
      UInt32 indexInArchive;
      if (ArchiveItems == 0)
        indexInArchive = updatePair.ArchiveItemIndex;
      else
        indexInArchive = (*ArchiveItems)[updatePair.ArchiveItemIndex].IndexInServer;
      return Archive->GetProperty(indexInArchive, propID, value);
    }
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream)
{
  COM_TRY_BEGIN
  const CUpdatePair2 &updatePair = (*UpdatePairs)[index];
  if(!updatePair.NewData)
    return E_FAIL;
  
  RINOK(Callback->CheckBreak());
  RINOK(Callback->Finilize());

  if(updatePair.IsAnti)
  {
    return Callback->GetStream((*ArchiveItems)[updatePair.ArchiveItemIndex].Name, true);
  }
  const CDirItem &dirItem = (*DirItems)[updatePair.DirItemIndex];
  RINOK(Callback->GetStream(dirItem.Name, false));
 
  if(dirItem.IsDirectory())
    return S_OK;

  if (StdInMode)
  {
    CStdInFileStream *inStreamSpec = new CStdInFileStream;
    CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
    *inStream = inStreamLoc.Detach();
  }
  else
  {
    CInFileStream *inStreamSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
    UString path = DirPrefix + dirItem.FullPath;
    if(!inStreamSpec->OpenShared(path, ShareForWrite))
    {
      return Callback->OpenFileError(path, ::GetLastError());
    }
    *inStream = inStreamLoc.Detach();
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::SetOperationResult(Int32 operationResult)
{
  COM_TRY_BEGIN
  return Callback->SetOperationResult(operationResult);
  COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeSize(UInt32 index, UInt64 *size)
{
  if (VolumesSizes.Size() == 0)
    return S_FALSE;
  if (index >= (UInt32)VolumesSizes.Size())
    index = VolumesSizes.Size() - 1;
  *size = VolumesSizes[index];
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeStream(UInt32 index, ISequentialOutStream **volumeStream)
{
  COM_TRY_BEGIN
  wchar_t temp[32];
  ConvertUInt64ToString(index + 1, temp);
  UString res = temp;
  while (res.Length() < 2)
    res = UString(L'0') + res;
  UString fileName = VolName;
  fileName += L'.';
  fileName += res;
  fileName += VolExt;
  COutFileStream *streamSpec = new COutFileStream;
  CMyComPtr<ISequentialOutStream> streamLoc(streamSpec);
  if(!streamSpec->Create(fileName, false))
    return ::GetLastError();
  *volumeStream = streamLoc.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  COM_TRY_BEGIN
  return Callback->CryptoGetTextPassword2(passwordIsDefined, password);
  COM_TRY_END
}
