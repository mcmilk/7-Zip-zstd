// ArchiveUpdateCallback.h

#include "StdAfx.h"

#include "ArchiveUpdateCallback.h"

#include "Common/StringConvert.h"
#include "Common/Defs.h"

#include "Windows/FileName.h"
#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

#include "Windows/Defs.h"

using namespace NWindows;

void CArchiveUpdateCallback::Init(const CSysString &baseFolderPrefix,
    const CObjectVector<CDirItem> *dirItems, 
    const CObjectVector<CArchiveItem> *archiveItems, // test CItemInfoExList
    CObjectVector<CUpdatePair2> *updatePairs,
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

STDMETHODIMP CArchiveUpdateCallback::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
}
*/

STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(UINT32 index, 
      INT32 *newData, INT32 *newProperties, UINT32 *indexInArchive)
{
  const CUpdatePair2 &updatePair = (*m_UpdatePairs)[index];
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
  const CUpdatePair2 &updatePair = (*m_UpdatePairs)[index];
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
        propVariant = (UINT64)0;
        propVariant.Detach(value);
        return S_OK;
      default:
        propVariant.Detach(value);
        return S_OK;
    }
  }
 
  if(updatePair.ExistOnDisk)
  {
    const CDirItem &dirItem = (*m_DirItems)[updatePair.DirItemIndex];
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
  const CUpdatePair2 &updatePair = (*m_UpdatePairs)[index];
  if(!updatePair.NewData)
    return E_FAIL;
  const CDirItem &dirItem = (*m_DirItems)[updatePair.DirItemIndex];

  /*
  m_PercentPrinter.PrintString("Compressing  ");
  m_PercentCanBePrint = true;
  m_PercentPrinter.PrintString(UnicodeStringToMultiByte(dirItem.Name, CP_OEMCP));
  m_PercentPrinter.PreparePrint();
  m_PercentPrinter.RePrintRatio();
  */

  if (m_UpdateCallback)
  {
    RINOK(m_UpdateCallback->CompressOperation(
        GetUnicodeString(dirItem.FullPath, m_CodePage)));
  }

  if(dirItem.IsDirectory())
    return S_OK;

  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStreamLoc(inStreamSpec);
  if(!inStreamSpec->Open(m_BaseFolderPrefix + dirItem.FullPath))
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
    HRESULT result = m_UpdateCallback.QueryInterface(
        IID_ICryptoGetTextPassword2, &_cryptoGetTextPassword);
    if (result != S_OK)
      return S_OK;
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword2(passwordIsDefined, password);
}
