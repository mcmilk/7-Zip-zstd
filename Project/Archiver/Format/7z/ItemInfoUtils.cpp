// ItemInfoUtils.cpp

#include "StdAfx.h"

#include "ItemInfoUtils.h"

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
  { NID::kName, NULL, kpidPath, VT_BSTR},
  { NID::kSize, NULL, kpidSize, VT_UI8},
  { NID::kPackInfo, NULL, kpidPackedSize, VT_UI8},
  
  #ifdef _MULTI_PACK
  { 100, L"Pack0", kpidPackedSize0, VT_UI8},
  { 101, L"Pack1", kpidPackedSize1, VT_UI8},
  { 102, L"Pack2", kpidPackedSize2, VT_UI8},
  { 103, L"Pack3", kpidPackedSize3, VT_UI8},
  { 104, L"Pack4", kpidPackedSize4, VT_UI8},
  #endif

  { NID::kCreationTime, NULL, kpidCreationTime, VT_FILETIME},
  { NID::kLastWriteTime, NULL, kpidLastWriteTime, VT_FILETIME},
  { NID::kLastAccessTime, NULL, kpidLastAccessTime, VT_FILETIME},
  { NID::kWinAttributes, NULL, kpidAttributes, VT_UI4},


  { NID::kCRC, NULL, kpidCRC, VT_UI4},
  
  { NID::kAnti, L"Anti", kpidIsAnti, VT_BOOL},
  // { 97, NULL, kpidSolid, VT_BOOL},
  #ifndef _SFX
  { 98, NULL, kpidMethod, VT_BSTR},
  { 99, L"Block", kpidBlock, VT_UI4}
  #endif
  // { L"ID", kpidID, VT_BSTR},
  // { L"UnPack Version", kpidUnPackVersion, VT_UI1},
  // { L"Host OS", kpidHostOS, VT_BSTR}
};

static const int kPropMapSize = sizeof(kPropMap) / sizeof(kPropMap[0]);

static int FindPropInMap(UINT32 filePropID)
{
  for (int i = 0; i < kPropMapSize; i++)
    if (kPropMap[i].FilePropID == filePropID)
      return i;
  return -1;
}

static void CopyOneItem(CRecordVector<UINT32> &src, 
    CRecordVector<UINT32> &dest, UINT32 item)
{
  for (int i = 0; i < src.Size(); i++)
    if (src[i] == item)
    {
      dest.Add(item);
      src.Delete(i);
      return;
    }
}

static void RemoveOneItem(CRecordVector<UINT32> &src, UINT32 item)
{
  for (int i = 0; i < src.Size(); i++)
    if (src[i] == item)
    {
      src.Delete(i);
      return;
    }
}

static void InsertToHead(CRecordVector<UINT32> &dest, UINT32 item)
{
  for (int i = 0; i < dest.Size(); i++)
    if (dest[i] == item)
    {
      dest.Delete(i);
      break;
    }
  dest.Insert(0, item);
}

void CEnumArchiveItemProperty::Init(const CRecordVector<UINT32> &fileInfoPopIDsSpec)
{ 
  CRecordVector<UINT32> fileInfoPopIDs = fileInfoPopIDsSpec;

  RemoveOneItem(fileInfoPopIDs, NID::kEmptyStream);
  RemoveOneItem(fileInfoPopIDs, NID::kEmptyFile);

  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kName);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kAnti);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kSize);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kPackInfo);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kCreationTime);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kLastWriteTime);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kLastAccessTime);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kWinAttributes);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kCRC);
  CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::kComment);
  _fileInfoPopIDs += fileInfoPopIDs; 
 
  #ifndef _SFX
  _fileInfoPopIDs.Add(98);
  _fileInfoPopIDs.Add(99);
  #endif
  #ifdef _MULTI_PACK
  _fileInfoPopIDs.Add(100);
  _fileInfoPopIDs.Add(101);
  _fileInfoPopIDs.Add(102);
  _fileInfoPopIDs.Add(103);
  _fileInfoPopIDs.Add(104);
  #endif

  #ifndef _SFX
  InsertToHead(_fileInfoPopIDs, NID::kLastWriteTime);
  InsertToHead(_fileInfoPopIDs, NID::kPackInfo);
  InsertToHead(_fileInfoPopIDs, NID::kSize);
  InsertToHead(_fileInfoPopIDs, NID::kName);
  #endif
}


STDMETHODIMP CEnumArchiveItemProperty::Reset()
{
  _index = 0;
  return S_OK;
}

STDMETHODIMP CEnumArchiveItemProperty::Next(ULONG numItems, 
    STATPROPSTG *items, ULONG *numFetched)
{
  HRESULT result = S_OK;
  if(numItems > 1 && !numFetched)
    return E_INVALIDARG;

  for(DWORD index = 0; index < numItems; _index++)
  {
    if(_index >= _fileInfoPopIDs.Size())
    {
      result =  S_FALSE;
      break;
    }
    int indexInMap = FindPropInMap(_fileInfoPopIDs[_index]);
    if (indexInMap == -1)
      continue;
    const STATPROPSTG &srcItem = kPropMap[indexInMap].StatPROPSTG;
    STATPROPSTG &destItem = items[index];
    destItem.propid = srcItem.propid;
    destItem.vt = srcItem.vt;
    if(srcItem.lpwstrName != NULL)
    {
      destItem.lpwstrName = (wchar_t *)CoTaskMemAlloc((wcslen(srcItem.lpwstrName) + 1) * sizeof(wchar_t));
      wcscpy(destItem.lpwstrName, srcItem.lpwstrName);
    }
    else
      destItem.lpwstrName = srcItem.lpwstrName;
    index++;
  }
  if (numFetched)
    *numFetched = index;
  return result;
}

STDMETHODIMP CEnumArchiveItemProperty::Skip(ULONG numSkip)
  {  return E_NOTIMPL; }

STDMETHODIMP CEnumArchiveItemProperty::Clone(IEnumSTATPROPSTG **enumerator)
  {  return E_NOTIMPL; }

}}
