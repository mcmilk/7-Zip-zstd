// Tar/OutHandler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "UpdateEngine.h"


using namespace NArchive;
using namespace NTar;

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

STDMETHODIMP CTarHandler::GetFileTimeType(UINT32 *aType)
{
  *aType = NFileTimeType::kUnix;
  return S_OK;
}

STDMETHODIMP CTarHandler::DeleteItems(IOutStream *anOutStream, 
    const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<bool> aCompressStatuses;
  CRecordVector<UINT32> aCopyIndexes;
  UINT32 anIndex = 0;
  for(UINT32 i = 0; i < (UINT32)m_Items.Size(); i++)
  {
    if(anIndex < aNumItems && i == anIndexes[anIndex])
      anIndex++;
    else
    {
      aCompressStatuses.Add(false);
      aCopyIndexes.Add(i);
    }
  }
  return UpdateArchive(m_InStream, anOutStream, m_Items, aCompressStatuses,
      CObjectVector<CUpdateItemInfo>(), aCopyIndexes, anUpdateCallBack);
  COM_TRY_END
}

STDMETHODIMP CTarHandler::UpdateItems(IOutStream *anOutStream, UINT32 aNumItems,
    IUpdateCallBack *anUpdateCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<bool> aCompressStatuses;
  CObjectVector<CUpdateItemInfo> anUpdateItems;
  CRecordVector<UINT32> aCopyIndexes;
  int anIndex = 0;
  for(UINT32 i = 0; i < aNumItems; i++)
  {
    CUpdateItemInfo anUpdateItemInfo;
    INT32 anCompress;
    INT32 anExistInArchive;
    INT32 anIndexInServer;
    FILETIME aTime;
    UINT64 aSize;
    CComBSTR aName;
    UINT32 anAttributes;
    HRESULT aResult = anUpdateCallBack->GetUpdateItemInfo(i,
        &anCompress, // 1 - compress 0 - copy
        &anExistInArchive,
        &anIndexInServer,
        &anAttributes,
        NULL,
        NULL,
        &aTime, 
        &aSize, 
        &aName);

    if (aResult != S_OK)
      return aResult;
    if (MyBoolToBool(anCompress))
    {
      if(!FileTimeToUnixTime(aTime, anUpdateItemInfo.Time))
        return E_FAIL;

      anUpdateItemInfo.IsDirectory = ((anAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
      anUpdateItemInfo.Size = aSize;
      anUpdateItemInfo.Name = UnicodeStringToMultiByte((BSTR)aName, CP_OEMCP);
      anUpdateItemInfo.IndexInClient = i;
      if(MyBoolToBool(anExistInArchive))
      {
        // const NArchive::NTar::CItemInfoEx &anItemInfo = m_Items[anIndexInServer];
      }
      aCompressStatuses.Add(true);
      anUpdateItems.Add(anUpdateItemInfo);
    }
    else
    {
      aCompressStatuses.Add(false);
      aCopyIndexes.Add(anIndexInServer);
    }
  }
  return UpdateArchive(m_InStream, anOutStream, m_Items, aCompressStatuses,
      anUpdateItems, aCopyIndexes, anUpdateCallBack);
  COM_TRY_END
}
