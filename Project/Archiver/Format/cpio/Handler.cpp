// cpio/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"
#include "Interface/EnumStatProp.h"
#include "Interface/StreamObjects.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"
#include "Archive/Common/ItemNameUtils.h"
#include "Archive/cpio/InEngine.h"


#include "../Common/DummyOutStream.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace Ncpio {

enum // PropID
{
  kpidUserName = kpidUserDefined,
  kpidGroupName, 
  kpidinode,
  kpidiChkSum
};

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  // { L"User Name", kpidUserName, VT_BSTR},
  // { L"Group Name", kpidGroupName, VT_BSTR},
  { L"inode", kpidinode, VT_UI4}
  // { L"CheckSum", kpidiChkSum, VT_UI4}
};


STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  bool aMustBeClosed = true;
  // try
  {
    CInArchive anArchive;

    if(anArchive.Open(aStream) != S_OK)
      return S_FALSE;

    m_Items.Clear();

    if (anOpenArchiveCallBack != NULL)
    {
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetTotal(NULL, NULL));
      UINT64 aNumFiles = m_Items.Size();
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
    }

    while(true)
    {
      CItemInfoEx anItemInfo;
      bool aFilled;
      HRESULT aResult = anArchive.GetNextItem(aFilled, anItemInfo);
      if (aResult == S_FALSE)
        return S_FALSE;
      if (aResult != S_OK)
        return S_FALSE;
      if (!aFilled)
        break;
      m_Items.Add(anItemInfo);
      anArchive.SkeepDataRecords(anItemInfo.Size);
      if (anOpenArchiveCallBack != NULL)
      {
        UINT64 aNumFiles = m_Items.Size();
        RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
      }
    }
    if (m_Items.Size() == 0)
      return S_FALSE;

    m_InStream = aStream;
  }
  /*
  catch(...)
  {
    return S_FALSE;
  }
  */
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  m_InStream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = m_Items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(
    UINT32 anIndex, 
    PROPID aPropID,  
    PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;
  const NArchive::Ncpio::CItemInfoEx &anItem = m_Items[anIndex];

  switch(aPropID)
  {
    case kpidPath:
      aPropVariant = (const wchar_t *)NItemName::GetOSName(
          MultiByteToUnicodeString(anItem.Name, CP_OEMCP));
      break;
    case kpidIsFolder:
      aPropVariant = anItem.IsDirectory();
      break;
    case kpidSize:
    case kpidPackedSize:
      aPropVariant = anItem.Size;
      break;
    case kpidLastWriteTime:
    {
      FILETIME anUTCFileTime;
      if (anItem.ModificationTime != 0)
        NTime::UnixTimeToFileTime(anItem.ModificationTime, anUTCFileTime);
      else
      {
        anUTCFileTime.dwLowDateTime = 0;
        anUTCFileTime.dwHighDateTime = 0;
      }
      aPropVariant = anUTCFileTime;
      break;
    }
    case kpidinode:
      aPropVariant = anItem.inode;
      break;
    /*
    case kpidiChkSum:
      aPropVariant = anItem.ChkSum;
      break;
    */
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
  UINT64 aTotalSize = 0;
  if(aNumItems == 0)
    return S_OK;
  for(UINT32 i = 0; i < aNumItems; i++)
    aTotalSize += m_Items[anIndexes[i]].Size;
  anExtractCallBack->SetTotal(aTotalSize);

  UINT64 aCurrentTotalSize = 0;
  UINT64 aCurrentItemSize;
  
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = NULL;
  CComPtr<ICompressCoder> aCopyCoder;

  for(i = 0; i < aNumItems; i++, aCurrentTotalSize += aCurrentItemSize)
  {
    RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalSize));
    CComPtr<ISequentialOutStream> aRealOutStream;
    INT32 anAskMode;
    anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
        NArchiveHandler::NExtract::NAskMode::kExtract;
    INT32 anIndex = anIndexes[i];
    const CItemInfoEx &anItemInfo = m_Items[anIndex];
    
    RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(anIndex, &aRealOutStream, anAskMode));

    aCurrentItemSize = anItemInfo.Size;

    if(anItemInfo.IsDirectory())
    {
      RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
      continue;
    }
    if(!aTestMode && (!aRealOutStream))
    {
      continue;
    }
    RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
    {
      if (aTestMode)
      {
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
        continue;
      }

      RETURN_IF_NOT_S_OK(m_InStream->Seek(anItemInfo.GetDataPosition(), STREAM_SEEK_SET, NULL));
      CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
          CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<ISequentialInStream> anInStream(aStreamSpec);
      aStreamSpec->Init(m_InStream, anItemInfo.Size);

      CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
      aLocalProgressSpec->Init(anExtractCallBack, false);


      CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
      aLocalCompressProgressSpec->Init(aProgress, 
          &aCurrentTotalSize, &aCurrentTotalSize);

      if(aCopyCoderSpec == NULL)
      {
        aCopyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
        aCopyCoder = aCopyCoderSpec;
      }
      try
      {
        RETURN_IF_NOT_S_OK(aCopyCoder->Code(anInStream, aRealOutStream,
            NULL, NULL, aCompressProgress));
      }
      catch(...)
      {
        aRealOutStream.Release();
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kDataError));
        continue;
      }
      aRealOutStream.Release();
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 aTestMode,
      IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(m_Items.Size());
  for(int i = 0; i < m_Items.Size(); i++)
    anIndexes.Add(i);
  return Extract(&anIndexes.Front(), m_Items.Size(), aTestMode,
      anExtractCallBack);
  COM_TRY_END
}

}}
