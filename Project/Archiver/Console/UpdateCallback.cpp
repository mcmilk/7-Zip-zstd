// UpdateCallback.cpp

#include "StdAfx.h"

#include "UpdateCallback.h"

#include "Common/StdInStream.h"
#include "Common/StdOutStream.h"
#include "Common/StringConvert.h"
#include "Common/Defs.h"

#include "Interface/FileStreams.h"

#include "ConsoleCloseUtils.h"

CUpdateCallBackImp::CUpdateCallBackImp():
  m_PercentPrinter(1 << 16) {}

void CUpdateCallBackImp::Init(const CArchiveStyleDirItemInfoVector *aDirItems, 
    const CArchiveItemInfoVector *anArchiveItems, // test CItemInfoExList
    CUpdatePairInfo2Vector *anUpdatePairs, bool anEnablePercents,
    bool passwordIsDefined, const UString &password, bool askPassword)
{
  _passwordIsDefined = passwordIsDefined;
  _password = password;
  _askPassword = askPassword;

  m_EnablePercents = anEnablePercents;
  m_DirItems = aDirItems;
  m_ArchiveItems = anArchiveItems;
  m_UpdatePairs = anUpdatePairs;
  m_PercentCanBePrint = false;
  m_NeedBeClosed = false;
}

void CUpdateCallBackImp::Finilize()
{
  if (m_NeedBeClosed)
  {
    if (m_EnablePercents)
    {
      m_PercentPrinter.ClosePrint();
      m_PercentCanBePrint = false;
      m_NeedBeClosed = false;
    }
    m_PercentPrinter.PrintNewLine();
  }
}

STDMETHODIMP CUpdateCallBackImp::SetTotal(UINT64 aSize)
{
  if (m_EnablePercents)
    m_PercentPrinter.SetTotal(aSize);
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::SetCompleted(const UINT64 *aCompleteValue)
{
  if (aCompleteValue != NULL)
  {
    if (m_EnablePercents)
    {
      m_PercentPrinter.SetRatio(*aCompleteValue);
      if (m_PercentCanBePrint)
        m_PercentPrinter.PrintRatio();
    }
  }

  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::GetUpdateItemInfo2(INT32 anIndex, 
      INT32 *anCompress, // 1 - compress 0 - copy
      INT32 *anExistInArchive, // 1 - exist, 0 - not exist
      INT32 *anIndexInServer,
      UINT32 *anAttributes,
      FILETIME *aCreationTime, 
      FILETIME *aLastAccessTime, 
      FILETIME *aLastWriteTime, 
      UINT64 *aSize, 
      BSTR *aName, 
      INT32 *anIsAnti)
{
  if (anIsAnti != NULL)
  {
    const CUpdatePairInfo2 &anUpdatePair = (*m_UpdatePairs)[anIndex];
    *anIsAnti = BoolToMyBool(anUpdatePair.IsAnti);
  }
  return GetUpdateItemInfo(anIndex, anCompress, anExistInArchive, 
      anIndexInServer, anAttributes, aCreationTime, aLastAccessTime, 
      aLastWriteTime, aSize, aName);
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
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  const CUpdatePairInfo2 &anUpdatePair = (*m_UpdatePairs)[anIndex];
  if(anCompress != NULL)
    *anCompress = BoolToMyBool(anUpdatePair.OperationIsCompress);
  if(anExistInArchive != NULL)
    *anExistInArchive = BoolToMyBool(anUpdatePair.ExistInArchive);
  if(anIndexInServer != NULL && anUpdatePair.ExistInArchive)
    *anIndexInServer = (*m_ArchiveItems)[anUpdatePair.ArchiveItemIndex].IndexInServer;
  if(anUpdatePair.OperationIsCompress && anUpdatePair.ExistOnDisk)
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
      CComBSTR tempName = aDirItemInfo.Name;
      *aName = tempName.Detach();
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
  
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;

  Finilize();

  if(anUpdatePair.IsAnti)
  {
    m_PercentPrinter.PrintString("Anti item    ");
    m_PercentPrinter.PrintString(UnicodeStringToMultiByte(
      (*m_ArchiveItems)[anUpdatePair.ArchiveItemIndex].Name, CP_OEMCP));
  }
  else
  {
    const CArchiveStyleDirItemInfo &aDirItemInfo = 
      (*m_DirItems)[anUpdatePair.DirItemIndex];
  
    m_PercentPrinter.PrintString("Compressing  ");
    m_PercentPrinter.PrintString(UnicodeStringToMultiByte(aDirItemInfo.Name, CP_OEMCP));
  }
  if (m_EnablePercents)
  {
    m_PercentCanBePrint = true;
    m_PercentPrinter.PreparePrint();
    m_PercentPrinter.RePrintRatio();
  }
  
  if(anUpdatePair.IsAnti)
    return S_OK;
 
  const CArchiveStyleDirItemInfo &aDirItemInfo = 
      (*m_DirItems)[anUpdatePair.DirItemIndex];
 
  if(aDirItemInfo.IsDirectory())
    return S_OK;

  CComObjectNoLock<CInFileStream> *anInStreamSpec =
      new CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> anInStreamLoc(anInStreamSpec);
  if(!anInStreamSpec->Open(aDirItemInfo.FullPathDiskName))
    return E_FAIL;

  *anInStream = anInStreamLoc.Detach();
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::DeleteOperation(LPITEMIDLIST anItemIDList)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallBackImp::OperationResult(INT32 aOperationResult)
{
  m_NeedBeClosed = true;
  return S_OK;
}


STDMETHODIMP CUpdateCallBackImp::CryptoGetTextPassword2(INT32 *passwordIsDefined, BSTR *password)
{
  if (!_passwordIsDefined) 
  {
    if (_askPassword)
    {
      g_StdOut << "\nEnter password:";
      AString oemPassword = g_StdIn.ScanStringUntilNewLine();
      _password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
      _passwordIsDefined = true;
    }
  }
  *passwordIsDefined = BoolToInt(_passwordIsDefined);
  CComBSTR tempName = _password;
  *password = tempName.Detach();
  return S_OK;
}
