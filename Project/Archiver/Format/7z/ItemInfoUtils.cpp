// ItemInfoUtils.cpp

#include "StdAfx.h"

#include "ItemInfoUtils.h"
#include "ItemNameUtils.h"

#include "Windows/COM.h"
#include "Windows/Time.h"

#include "Header.h"

using namespace NWindows;

// #define _MULTI_PACK

namespace NArchive {
namespace N7z {


struct CPropMap
{
  UINT32 FilePropID;
  STATPROPSTG StatPROPSTG;
};

CPropMap kPropMap[] = 
{
  { NID::kName, NULL, kaipidPath, VT_BSTR},
  { NID::kSize, NULL, kaipidSize, VT_UI8},
  { NID::kPackInfo, NULL, kaipidPackedSize, VT_UI8},
  
  #ifdef _MULTI_PACK
  { 100, L"Pack0", kaipidPackedSize0, VT_UI8},
  { 101, L"Pack1", kaipidPackedSize1, VT_UI8},
  { 102, L"Pack2", kaipidPackedSize2, VT_UI8},
  { 103, L"Pack3", kaipidPackedSize3, VT_UI8},
  { 104, L"Pack4", kaipidPackedSize4, VT_UI8},
  #endif

  { NID::kCreationTime, NULL, kaipidCreationTime, VT_FILETIME},
  { NID::kLastWriteTime, NULL, kaipidLastWriteTime, VT_FILETIME},
  { NID::kLastAccessTime, NULL, kaipidLastAccessTime, VT_FILETIME},
  { NID::kWinAttributes, NULL, kaipidAttributes, VT_UI4},


  { NID::kCRC, NULL, kaipidCRC, VT_UI4},
  
  { NID::kAnti, L"Anti", kaipidIsAnti, VT_BOOL}
  // { L"ID", kaipidID, VT_BSTR},
  // { L"UnPack Version", kaipidUnPackVersion, VT_UI1},
  // { L"Host OS", kaipidHostOS, VT_BSTR}
};

static const int kPropMapSize = sizeof(kPropMap) / sizeof(kPropMap[0]);

static int FindPropInMap(UINT32 aFilePropID)
{
  for (int i = 0; i < kPropMapSize; i++)
    if (kPropMap[i].FilePropID == aFilePropID)
      return i;
  return -1;
}

void CopyOneItem(CRecordVector<UINT32> &aFrom, CRecordVector<UINT32> &aTo, UINT32 anItem)
{
  for (int i = 0; i < aFrom.Size(); i++)
    if (aFrom[i] == anItem)
    {
      aTo.Add(anItem);
      aFrom.Delete(i);
      return;
    }
}

void RemoveOneItem(CRecordVector<UINT32> &aFrom, UINT32 anItem)
{
  for (int i = 0; i < aFrom.Size(); i++)
    if (aFrom[i] == anItem)
    {
      aFrom.Delete(i);
      return;
    }
}

void CEnumArchiveItemProperty::Init(const CRecordVector<UINT32> &_aFileInfoPopIDs)
{ 
  CRecordVector<UINT32> aFileInfoPopIDs = _aFileInfoPopIDs;

  RemoveOneItem(aFileInfoPopIDs, NID::kEmptyStream);
  RemoveOneItem(aFileInfoPopIDs, NID::kEmptyFile);

  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kName);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kAnti);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kSize);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kPackInfo);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kCreationTime);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kLastWriteTime);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kLastAccessTime);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kWinAttributes);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kCRC);
  CopyOneItem(aFileInfoPopIDs, m_FileInfoPopIDs, NID::kComment);
  m_FileInfoPopIDs += aFileInfoPopIDs; 
 
  #ifdef _MULTI_PACK
  m_FileInfoPopIDs.Add(100);
  m_FileInfoPopIDs.Add(101);
  m_FileInfoPopIDs.Add(102);
  m_FileInfoPopIDs.Add(103);
  m_FileInfoPopIDs.Add(104);
  #endif
}


STDMETHODIMP CEnumArchiveItemProperty::Reset()
{
  m_Index = 0;
  return S_OK;
}

STDMETHODIMP CEnumArchiveItemProperty::Next(ULONG aNumItems, 
    STATPROPSTG *anItems, ULONG *aNumFetched)
{
  HRESULT aResult = S_OK;
  if(aNumItems > 1 && !aNumFetched)
    return E_INVALIDARG;

  for(DWORD anIndex = 0; anIndex < aNumItems; m_Index++)
  {
    if(m_Index >= m_FileInfoPopIDs.Size())
    {
      aResult =  S_FALSE;
      break;
    }
    int anIndexInMap = FindPropInMap(m_FileInfoPopIDs[m_Index]);
    if (anIndexInMap == -1)
      continue;
    const STATPROPSTG &aSrcItem = kPropMap[anIndexInMap].StatPROPSTG;
    STATPROPSTG &aDestItem = anItems[anIndex];
    aDestItem.propid = aSrcItem.propid;
    aDestItem.vt = aSrcItem.vt;
    if(aSrcItem.lpwstrName != NULL)
    {
      aDestItem.lpwstrName = (wchar_t *)CoTaskMemAlloc((wcslen(aSrcItem.lpwstrName) + 1) * sizeof(wchar_t));
      wcscpy(aDestItem.lpwstrName, aSrcItem.lpwstrName);
    }
    else
      aDestItem.lpwstrName = aSrcItem.lpwstrName;
    anIndex++;
  }
  if (aNumFetched)
    *aNumFetched = anIndex;
  return aResult;
}

STDMETHODIMP CEnumArchiveItemProperty::Skip(ULONG aNumSkip)
  {  return E_NOTIMPL; }

STDMETHODIMP CEnumArchiveItemProperty::Clone(IEnumSTATPROPSTG **anEnum)
  {  return E_NOTIMPL; }

}}
