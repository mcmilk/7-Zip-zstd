// UpdateCallback.cpp

#include "StdAfx.h"

#include "UpdateCallback.h"

#include "Common/StdInStream.h"
#include "Common/StdOutStream.h"
#include "Common/StringConvert.h"
#include "Common/Defs.h"

#include "Interface/FileStreams.h"
#include "Interface/EnumStatProp.h"

#include "ConsoleCloseUtils.h"

CUpdateCallbackImp::CUpdateCallbackImp():
  m_PercentPrinter(1 << 16) {}

void CUpdateCallbackImp::Init(const CArchiveStyleDirItemInfoVector *dirItems, 
    const CArchiveItemInfoVector *anArchiveItems, // test CItemInfoExList
    CUpdatePairInfo2Vector *anUpdatePairs, bool anEnablePercents,
    bool passwordIsDefined, const UString &password, bool askPassword)
{
  _passwordIsDefined = passwordIsDefined;
  _password = password;
  _askPassword = askPassword;

  m_EnablePercents = anEnablePercents;
  m_DirItems = dirItems;
  m_ArchiveItems = anArchiveItems;
  m_UpdatePairs = anUpdatePairs;
  m_PercentCanBePrint = false;
  m_NeedBeClosed = false;
}

void CUpdateCallbackImp::Finilize()
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

STDMETHODIMP CUpdateCallbackImp::SetTotal(UINT64 size)
{
  if (m_EnablePercents)
    m_PercentPrinter.SetTotal(size);
  return S_OK;
}

STDMETHODIMP CUpdateCallbackImp::SetCompleted(const UINT64 *completeValue)
{
  if (completeValue != NULL)
  {
    if (m_EnablePercents)
    {
      m_PercentPrinter.SetRatio(*completeValue);
      if (m_PercentCanBePrint)
        m_PercentPrinter.PrintRatio();
    }
  }

  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
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
  { NULL, kpidIsAnti, VT_BOOL}
};

STDMETHODIMP CUpdateCallbackImp::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
}

STDMETHODIMP CUpdateCallbackImp::GetUpdateItemInfo(UINT32 index, 
      INT32 *newData, INT32 *newProperties, UINT32 *indexInArchive)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  const CUpdatePairInfo2 &updatePair = (*m_UpdatePairs)[index];
  if(newData != NULL)
    *newData = BoolToInt(updatePair.NewData);
  if(newProperties != NULL)
    *newProperties = BoolToInt(updatePair.NewProperties);
  if(indexInArchive != NULL)
  {
    if (updatePair.ExistInArchive)
      *indexInArchive = (*m_ArchiveItems)[updatePair.ArchiveItemIndex].IndexInServer;
    else
      *indexInArchive = UINT32(-1);
  }
  return S_OK;
}

STDMETHODIMP CUpdateCallbackImp::GetProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  const CUpdatePairInfo2 &updatePair = (*m_UpdatePairs)[index];
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
    }
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CUpdateCallbackImp::GetStream(UINT32 index,
    IInStream **inStream)
{
  const CUpdatePairInfo2 &updatePair = (*m_UpdatePairs)[index];
  if(!updatePair.NewData)
    return E_FAIL;
  
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;

  Finilize();

  if(updatePair.IsAnti)
  {
    m_PercentPrinter.PrintString("Anti item    ");
    m_PercentPrinter.PrintString(UnicodeStringToMultiByte(
      (*m_ArchiveItems)[updatePair.ArchiveItemIndex].Name, CP_OEMCP));
  }
  else
  {
    const CArchiveStyleDirItemInfo &dirItemInfo = 
      (*m_DirItems)[updatePair.DirItemIndex];
  
    m_PercentPrinter.PrintString("Compressing  ");
    m_PercentPrinter.PrintString(UnicodeStringToMultiByte(dirItemInfo.Name, CP_OEMCP));
  }
  if (m_EnablePercents)
  {
    m_PercentCanBePrint = true;
    m_PercentPrinter.PreparePrint();
    m_PercentPrinter.RePrintRatio();
  }
  
  if(updatePair.IsAnti)
    return S_OK;
 
  const CArchiveStyleDirItemInfo &dirItemInfo = 
      (*m_DirItems)[updatePair.DirItemIndex];
 
  if(dirItemInfo.IsDirectory())
    return S_OK;

  CComObjectNoLock<CInFileStream> *anInStreamSpec =
      new CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> anInStreamLoc(anInStreamSpec);
  if(!anInStreamSpec->Open(dirItemInfo.FullPathDiskName))
    return ::GetLastError();
  *inStream = anInStreamLoc.Detach();
  return S_OK;
}

STDMETHODIMP CUpdateCallbackImp::SetOperationResult(INT32 operationResult)
{
  m_NeedBeClosed = true;
  return S_OK;
}


STDMETHODIMP CUpdateCallbackImp::CryptoGetTextPassword2(INT32 *passwordIsDefined, BSTR *password)
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
