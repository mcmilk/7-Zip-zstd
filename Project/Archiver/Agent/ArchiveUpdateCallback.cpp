// ArchiveUpdateCallback.h

#include "StdAfx.h"

#include "ArchiveUpdateCallback.h"

#include "Common/StringConvert.h"
#include "Common/Defs.h"

#include "Windows/FileName.h"

#include "Interface/FileStreams.h"
#include "Interface/EnumStatProp.h"

#include "Windows/Defs.h"

using namespace NWindows;

void CArchiveUpdateCallback::Init(const CSysString &baseFolderPrefix,
    const CArchiveStyleDirItemInfoVector *dirItems, 
    const CArchiveItemInfoVector *archiveItems, // test CItemInfoExList
    CUpdatePairInfo2Vector *updatePairs,
    IInArchive *inArchive,
    IFolderArchiveUpdateCallback *updateCallback)
{
  m_BaseFolderPrefix = baseFolderPrefix;
  NFile::NName::NormalizeDirPathPrefix(m_BaseFolderPrefix);
  m_DirItems = dirItems;
  m_ArchiveItems = archiveItems;
  m_UpdatePairs = updatePairs;
  m_UpdateCallback = updateCallback;
  m_CodePage = ::AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  _inArchive = inArchive;;
}

STDMETHODIMP CArchiveUpdateCallback::SetTotal(UINT64 size)
{
  if (m_UpdateCallback)
    return m_UpdateCallback->SetTotal(size);
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const UINT64 *completeValue)
{
  if (m_UpdateCallback)
    return m_UpdateCallback->SetCompleted(completeValue);
  return S_OK;
}

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidLastAccessTime, VT_FILETIME},
  { NULL, kpidCreationTime, VT_FILETIME},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},

};

STDMETHODIMP CArchiveUpdateCallback::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
}


STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(UINT32 index, 
      INT32 *newData, INT32 *newProperties, UINT32 *indexInArchive)
{
  const CUpdatePairInfo2 &updatePair = (*m_UpdatePairs)[index];
  if(newData != NULL)
    *newData = BoolToInt(updatePair.NewData);
  if(newProperties != NULL)
    *newProperties = BoolToInt(updatePair.NewProperties);
  if(indexInArchive != NULL)
  {
    if (updatePair.ExistInArchive)
    {
      if (m_ArchiveItems == 0)
        *indexInArchive = updatePair.ArchiveItemIndex;
      else
        *indexInArchive = (*m_ArchiveItems)[updatePair.ArchiveItemIndex].IndexInServer;
    }
    else
      *indexInArchive = UINT32(-1);
  }
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  const CUpdatePairInfo2 &updatePair = (*m_UpdatePairs)[index];
  NWindows::NCOM::CPropVariant propVariant;
  if(updatePair.ExistOnDisk)
  {
    const CArchiveStyleDirItemInfo &dirItemInfo = 
        (*m_DirItems)[updatePair.DirItemIndex];
    switch(propID)
    {
      case kpidPath:
        propVariant = dirItemInfo.Name;
        break;
      case kpidIsFolder:
        propVariant = dirItemInfo.IsDirectory();
        break;
      case kpidSize:
        propVariant = dirItemInfo.Size;
        break;
      case kpidAttributes:
        propVariant = dirItemInfo.Attributes;
        break;
      case kpidLastAccessTime:
        propVariant = dirItemInfo.LastAccessTime;
        break;
      case kpidCreationTime:
        propVariant = dirItemInfo.CreationTime;
        break;
      case kpidLastWriteTime:
        propVariant = dirItemInfo.LastWriteTime;
        break;
      /*
      case kpidIsAnti:
        propVariant = updatePair.IsAnti;
        break;
      */
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
    if (updatePair.ExistInArchive && _inArchive)
    {
      UINT32 indexInArchive;
      if (m_ArchiveItems == 0)
        indexInArchive = updatePair.ArchiveItemIndex;
      else
        indexInArchive = (*m_ArchiveItems)[updatePair.ArchiveItemIndex].IndexInServer;
      return _inArchive->GetProperty(indexInArchive, propID, value);
    }
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(UINT32 index,
    IInStream **inStream)
{
  const CUpdatePairInfo2 &updatePair = (*m_UpdatePairs)[index];
  if(!updatePair.NewData)
    return E_FAIL;
  const CArchiveStyleDirItemInfo &dirItemInfo = 
      (*m_DirItems)[updatePair.DirItemIndex];

  /*
  m_PercentPrinter.PrintString("Compressing  ");
  m_PercentCanBePrint = true;
  m_PercentPrinter.PrintString(UnicodeStringToMultiByte(dirItemInfo.Name, CP_OEMCP));
  m_PercentPrinter.PreparePrint();
  m_PercentPrinter.RePrintRatio();
  */

  if (m_UpdateCallback)
  {
    RETURN_IF_NOT_S_OK(m_UpdateCallback->CompressOperation(
        GetUnicodeString(dirItemInfo.FullPathDiskName, m_CodePage)));
  }

  if(dirItemInfo.IsDirectory())
    return S_OK;

  CComObjectNoLock<CInFileStream> *inStreamSpec =
      new CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> inStreamLoc(inStreamSpec);
  if(!inStreamSpec->Open(m_BaseFolderPrefix + dirItemInfo.FullPathDiskName))
    return ::GetLastError();

  *inStream = inStreamLoc.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetOperationResult(INT32 operationResult)
{
  if (m_UpdateCallback)
    return m_UpdateCallback->OperationResult(operationResult);
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword2(INT32 *passwordIsDefined, BSTR *password)
{
  *passwordIsDefined = BoolToInt(false);
  if (!_cryptoGetTextPassword)
  {
    if (!m_UpdateCallback)
      return S_OK;
    HRESULT result = m_UpdateCallback.QueryInterface(&_cryptoGetTextPassword);
    if (result != S_OK)
      return S_OK;
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword2(passwordIsDefined, password);
}
