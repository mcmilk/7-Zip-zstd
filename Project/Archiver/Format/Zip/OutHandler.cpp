// Zip/OutHandler.cpp

#include "StdAfx.h"

// #include "../../Handler/FileTimeType.h"
#include "Handler.h"
#include "Archive/Zip/OutEngine.h"
#include "Common/StringConvert.h"
#include "UpdateMain.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

using namespace NArchive;
using namespace NZip;

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

STDMETHODIMP CZipHandler::GetFileTimeType(UINT32 *aType)
{
  *aType = NFileTimeType::kDOS;
  return S_OK;
}

STDMETHODIMP CZipHandler::DeleteItems(IOutStream *anOutStream, 
    const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<bool> aCompressStatuses;
  CRecordVector<UINT32> aCopyIndexes;
  int anIndex = 0;
  for(int i = 0; i < m_Items.Size(); i++)
  {
    if(anIndex < aNumItems && i == anIndexes[anIndex])
      anIndex++;
    else
    {
      aCompressStatuses.Add(false);
      aCopyIndexes.Add(i);
    }
  }
  UpdateMain(m_Items, aCompressStatuses,
      CObjectVector<CUpdateItemInfo>(), aCopyIndexes, 
      anOutStream, m_ArchiveIsOpen ? &m_Archive : NULL, NULL, anUpdateCallBack);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CZipHandler::UpdateItems(IOutStream *anOutStream, UINT32 aNumItems,
    IUpdateCallBack *anUpdateCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<bool> aCompressStatuses;
  CObjectVector<CUpdateItemInfo> anUpdateItems;
  CRecordVector<UINT32> aCopyIndexes;
  int anIndex = 0;
  for(int i = 0; i < aNumItems; i++)
  {
    CUpdateItemInfo anUpdateItemInfo;
    INT32 anCompress;
    INT32 anExistInArchive;
    INT32 anIndexInServer;
    FILETIME anUTCFileTime;
    UINT64 aSize;
    CComBSTR aName;
    HRESULT aResult = anUpdateCallBack->GetUpdateItemInfo(i,
        &anCompress, // 1 - compress 0 - copy
        &anExistInArchive,
        &anIndexInServer,
        &anUpdateItemInfo.Attributes,
        NULL,
        NULL,
        &anUTCFileTime,
        &aSize, 
        &aName);
    if (aResult != S_OK)
      return aResult;
    if (MyBoolToBool(anCompress))
    {
      FILETIME aLocalFileTime;
      if(!FileTimeToLocalFileTime(&anUTCFileTime, &aLocalFileTime))
        return E_FAIL;
      if(!FileTimeToDosTime(aLocalFileTime, anUpdateItemInfo.Time))
        return E_FAIL;
      if(aSize > _UI32_MAX)
        return E_FAIL;
      anUpdateItemInfo.Size = aSize;
      anUpdateItemInfo.Name = UnicodeStringToMultiByte((BSTR)aName, CP_OEMCP);
      anUpdateItemInfo.IndexInClient = i;
      if(MyBoolToBool(anExistInArchive))
      {
        const NArchive::NZip::CItemInfoEx &anItemInfo = m_Items[anIndexInServer];
        anUpdateItemInfo.Commented = anItemInfo.IsCommented();
        if(anUpdateItemInfo.Commented)
        {
          anUpdateItemInfo.CommentRange.Position = anItemInfo.GetCommentPosition();
          anUpdateItemInfo.CommentRange.Size  = anItemInfo.CommentSize;
        }
      }
      else
        anUpdateItemInfo.Commented = false;
      aCompressStatuses.Add(true);
      anUpdateItems.Add(anUpdateItemInfo);
    }
    else
    {
      aCompressStatuses.Add(false);
      aCopyIndexes.Add(anIndexInServer);
    }
  }
  return UpdateMain(m_Items, aCompressStatuses,
      anUpdateItems, aCopyIndexes, anOutStream, m_ArchiveIsOpen ? &m_Archive : NULL, 
      &m_Method, anUpdateCallBack);
  COM_TRY_END
}

STDMETHODIMP CZipHandler::SetProperties(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties)
{
  m_Method.MaximizeRatio = false;
  bool aM0 = false;
  for (int i = 0; i < aNumProperties; i++)
  {
    UString aString = UString(aNames[i]);
    aString.MakeUpper();
    if (aString == L"X")
      m_Method.MaximizeRatio = true;
    else if (aString == L"0")
      aM0 = true;
  }
  m_Method.MethodSequence.Clear();
  if (!aM0)
    m_Method.MethodSequence.Add(NFileHeader::NCompressionMethod::kDeflated);
  m_Method.MethodSequence.Add(NFileHeader::NCompressionMethod::kStored);
  return S_OK;
}  
