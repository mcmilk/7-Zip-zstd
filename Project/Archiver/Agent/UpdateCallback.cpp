// UpdateEngine.h

#include "StdAfx.h"

#include "UpdateCallback.h"

#include "Common/StringConvert.h"
#include "Common/Defs.h"

#include "Windows/FileName.h"

#include "Interface/FileStreams.h"

#include "Windows/Defs.h"

using namespace NWindows;

void CUpdateCallBackImp::Init(const CSysString &aBaseFolderPrefix,
    const CArchiveStyleDirItemInfoVector *aDirItems, 
    const CArchiveItemInfoVector *anArchiveItems, // test CItemInfoExList
    CUpdatePairInfo2Vector *anUpdatePairs,
    UINT aCodePage,
    IUpdateCallback100 *anUpdateCallback)
{
  m_BaseFolderPrefix = aBaseFolderPrefix;
  NFile::NName::NormalizeDirPathPrefix(m_BaseFolderPrefix);
  m_DirItems = aDirItems;
  m_ArchiveItems = anArchiveItems;
  m_UpdatePairs = anUpdatePairs;
  m_UpdateCallback = anUpdateCallback;
  m_CodePage = aCodePage;
}

STDMETHODIMP CUpdateCallBackImp::SetTotal(UINT64 aSize)
{
  if (m_UpdateCallback)
    return m_UpdateCallback->SetTotal(aSize);
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::SetCompleted(const UINT64 *aCompleteValue)
{
  if (m_UpdateCallback)
    return m_UpdateCallback->SetCompleted(aCompleteValue);
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::GetUpdateItemInfo(INT32 anIndex, 
      INT32 *anCompress, // 1 - compress 0 - copy
      INT32 *anExistInArchive, // 1 - exist, 0 - not exist
      INT32 *anIndexInServer,
      UINT32 *anAttributes,
      FILETIME *aCreationTime, 
      FILETIME *aLastAccessTime, 
      FILETIME *aLastWriteTime, 
      UINT64 *aSize, 
      BSTR *aName)
{
  const CUpdatePairInfo2 &anUpdatePair = (*m_UpdatePairs)[anIndex];
  if(anCompress != NULL)
    *anCompress = BoolToMyBool(anUpdatePair.OperationIsCompress);
  if(anExistInArchive != NULL)
    *anExistInArchive = BoolToMyBool(anUpdatePair.ExistInArchive);
  if(anIndexInServer != NULL && anUpdatePair.ExistInArchive)
    *anIndexInServer = (*m_ArchiveItems)[anUpdatePair.ArchiveItemIndex].IndexInServer;
  if(anUpdatePair.OperationIsCompress)
  {
    const CArchiveStyleDirItemInfo &aDirItemInfo = 
        (*m_DirItems)[anUpdatePair.DirItemIndex];
    if(anAttributes != NULL)
      *anAttributes = aDirItemInfo.Attributes;

    if(aCreationTime != NULL)
      *aCreationTime = aDirItemInfo.CreationTime;
    if(aLastAccessTime != NULL)
      *aLastAccessTime = aDirItemInfo.LastAccessTime;
    if(aLastWriteTime != NULL)
      *aLastWriteTime = aDirItemInfo.LastWriteTime;

    if(aSize != NULL)
      *aSize = aDirItemInfo.Size;
    if(aName != NULL)
    {
      CComBSTR aTempName = aDirItemInfo.Name;
      *aName = aTempName.Detach();
    }
  }
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::CompressOperation(INT32 anIndex,
    IInStream **anInStream)
{
  const CUpdatePairInfo2 &anUpdatePair = (*m_UpdatePairs)[anIndex];
  if(!anUpdatePair.OperationIsCompress)
    return E_FAIL;
  const CArchiveStyleDirItemInfo &aDirItemInfo = 
      (*m_DirItems)[anUpdatePair.DirItemIndex];

  /*
  m_PercentPrinter.PrintString("Compressing  ");
  m_PercentCanBePrint = true;
  m_PercentPrinter.PrintString(UnicodeStringToMultiByte(aDirItemInfo.Name, CP_OEMCP));
  m_PercentPrinter.PreparePrint();
  m_PercentPrinter.RePrintRatio();
  */

  if (m_UpdateCallback)
  {
    RETURN_IF_NOT_S_OK(m_UpdateCallback->CompressOperation(
        GetUnicodeString(aDirItemInfo.FullPathDiskName, m_CodePage)));
  }

  if(aDirItemInfo.IsDirectory())
    return S_OK;

  CComObjectNoLock<CInFileStream> *anInStreamSpec =
      new CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> anInStreamLoc(anInStreamSpec);
  if(!anInStreamSpec->Open(m_BaseFolderPrefix + aDirItemInfo.FullPathDiskName))
    return E_FAIL;

  *anInStream = anInStreamLoc.Detach();
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::DeleteOperation(LPITEMIDLIST anItemIDList)
{
  return S_OK;
  /*
  RETURN_IF_NOT_S_OK(m_UpdateCallback->CompressOperation(
      GetUnicodeString(aDirItemInfo.FullPathDiskName, m_CodePage)));
  */
}

STDMETHODIMP CUpdateCallBackImp::OperationResult(INT32 aOperationResult)
{
  if (m_UpdateCallback)
    return m_UpdateCallback->OperationResult(aOperationResult);
  return S_OK;
}
