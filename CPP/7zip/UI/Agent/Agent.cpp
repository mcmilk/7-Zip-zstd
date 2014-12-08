// Agent.cpp

#include "StdAfx.h"

#include "../../../../C/Sort.h"

#include "../../../Common/ComTry.h"

#include "../../../Windows/PropVariantConv.h"

#include "../Common/ArchiveExtractCallback.h"
#include "../FileManager/RegistryUtils.h"

#include "Agent.h"

using namespace NWindows;

STDMETHODIMP CAgentFolder::GetAgentFolder(CAgentFolder **agentFolder)
{
  *agentFolder = this;
  return S_OK;
}

void CAgentFolder::LoadFolder(unsigned proxyFolderIndex)
{
  unsigned i;
  CProxyItem item;
  item.ProxyFolderIndex = proxyFolderIndex;
  if (_proxyArchive2)
  {
    const CProxyFolder2 &folder = _proxyArchive2->Folders[proxyFolderIndex];
    for (i = 0; i < folder.SubFiles.Size(); i++)
    {
      item.Index = i;
      _items.Add(item);
      const CProxyFile2 &file = _proxyArchive2->Files[folder.SubFiles[i]];
      int subFolderIndex = file.FolderIndex;
      if (subFolderIndex >= 0)
        LoadFolder(subFolderIndex);
      subFolderIndex = file.AltStreamsFolderIndex;
      if (subFolderIndex >= 0)
        LoadFolder(subFolderIndex);
    }
    return;
  }
  const CProxyFolder &folder = _proxyArchive->Folders[proxyFolderIndex];
  for (i = 0; i < folder.Folders.Size(); i++)
  {
    item.Index = i;
    _items.Add(item);
    LoadFolder(folder.Folders[i]);
  }
  unsigned start = folder.Folders.Size();
  for (i = 0; i < folder.Files.Size(); i++)
  {
    item.Index = start + i;
    _items.Add(item);
  }
}

STDMETHODIMP CAgentFolder::LoadItems()
{
  if (!_agentSpec->_archiveLink.IsOpen)
    return E_FAIL;
  _items.Clear();
  if (_flatMode)
    LoadFolder(_proxyFolderItem);
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetNumberOfItems(UInt32 *numItems)
{
  if (_flatMode)
    *numItems = _items.Size();
  else if (_proxyArchive2)
    *numItems = _proxyArchive2->Folders[_proxyFolderItem].SubFiles.Size();
  else
  {
    const CProxyFolder *folder = &_proxyArchive->Folders[_proxyFolderItem];
    *numItems = folder->Folders.Size() + folder ->Files.Size();
  }
  return S_OK;
}

#define SET_realIndex_AND_folder \
  UInt32 realIndex; const CProxyFolder *folder; \
  if (_flatMode) { const CProxyItem &item = _items[index]; folder = &_proxyArchive->Folders[item.ProxyFolderIndex]; realIndex = item.Index; } \
  else { folder = &_proxyArchive->Folders[_proxyFolderItem]; realIndex = index; }

#define SET_realIndex_AND_folder_2 \
  UInt32 realIndex; const CProxyFolder2 *folder; \
  if (_flatMode) { const CProxyItem &item = _items[index]; folder = &_proxyArchive2->Folders[item.ProxyFolderIndex]; realIndex = item.Index; } \
  else { folder = &_proxyArchive2->Folders[_proxyFolderItem]; realIndex = index; }

UString CAgentFolder::GetName(UInt32 index) const
{
  if (_proxyArchive2)
  {
    SET_realIndex_AND_folder_2
    return _proxyArchive2->Files[folder->SubFiles[realIndex]].Name;
  }
  SET_realIndex_AND_folder
  if (realIndex < (UInt32)folder->Folders.Size())
    return _proxyArchive->Folders[folder->Folders[realIndex]].Name;
  return folder->Files[realIndex - folder->Folders.Size()].Name;
}

void CAgentFolder::GetPrefix(UInt32 index, UString &prefix) const
{
  if (!_flatMode)
  {
    prefix.Empty();
    return;
  }
  const CProxyItem &item = _items[index];
  unsigned proxyIndex = item.ProxyFolderIndex;
  unsigned totalLen;
  if (_proxyArchive2)
  {
    unsigned len = 0;
    while (proxyIndex != _proxyFolderItem)
    {
      const CProxyFile2 &file = _proxyArchive2->Files[_proxyArchive2->Folders[proxyIndex].ArcIndex];
      len += file.NameSize + 1;
      proxyIndex = (file.Parent < 0) ? 0 : _proxyArchive2->Files[file.Parent].GetFolderIndex(file.IsAltStream);
    }
    totalLen = len;
    
    wchar_t *p = prefix.GetBuffer(len);
    proxyIndex = item.ProxyFolderIndex;
    while (proxyIndex != _proxyFolderItem)
    {
      const CProxyFile2 &file = _proxyArchive2->Files[_proxyArchive2->Folders[proxyIndex].ArcIndex];
      MyStringCopy(p + len - file.NameSize - 1, file.Name);
      p[--len] = WCHAR_PATH_SEPARATOR;
      len -= file.NameSize;
      proxyIndex = (file.Parent < 0) ? 0 : _proxyArchive2->Files[file.Parent].GetFolderIndex(file.IsAltStream);
    }
  }
  else
  {
    unsigned len = 0;
    while (proxyIndex != _proxyFolderItem)
    {
      const CProxyFolder *folder = &_proxyArchive->Folders[proxyIndex];
      len += folder->Name.Len() + 1;
      proxyIndex = folder->Parent;
    }
    totalLen = len;
    
    wchar_t *p = prefix.GetBuffer(len);
    proxyIndex = item.ProxyFolderIndex;
    while (proxyIndex != _proxyFolderItem)
    {
      const CProxyFolder *folder = &_proxyArchive->Folders[proxyIndex];
      MyStringCopy(p + len - folder->Name.Len() - 1, (const wchar_t *)folder->Name);
      p[--len] = WCHAR_PATH_SEPARATOR;
      len -= folder->Name.Len();
      proxyIndex = folder->Parent;
    }
  }
  prefix.ReleaseBuffer(totalLen);
}

UString CAgentFolder::GetFullPathPrefixPlusPrefix(UInt32 index) const
{
  UString prefix;
  GetPrefix(index, prefix);
  if (_proxyArchive2)
    return _proxyArchive2->GetFullPathPrefix(_proxyFolderItem) + prefix;
  else
    return _proxyArchive->GetFullPathPrefix(_proxyFolderItem) + prefix;
}

STDMETHODIMP_(UInt64) CAgentFolder::GetItemSize(UInt32 index)
{
  unsigned arcIndex;
  if (_proxyArchive2)
  {
    SET_realIndex_AND_folder_2
    arcIndex = folder->SubFiles[realIndex];
    const CProxyFile2 &item = _proxyArchive2->Files[arcIndex];
    if (item.IsDir())
    {
      const CProxyFolder2 &itemFolder = _proxyArchive2->Folders[item.FolderIndex];
      if (!_flatMode)
        return itemFolder.Size;
    }
  }
  else
  {
    SET_realIndex_AND_folder
    if (realIndex < (UInt32)folder->Folders.Size())
    {
      const CProxyFolder &item = _proxyArchive->Folders[folder->Folders[realIndex]];
      if (!_flatMode)
        return item.Size;
      if (!item.IsLeaf)
        return 0;
      arcIndex = item.Index;
    }
    else
    {
      const CProxyFile &item = folder->Files[realIndex - folder->Folders.Size()];
      arcIndex = item.Index;
    }
  }
  NCOM::CPropVariant prop;
  _agentSpec->GetArchive()->GetProperty(arcIndex, kpidSize, &prop);
  if (prop.vt == VT_UI8)
    return prop.uhVal.QuadPart;
  else
    return 0;
}

STDMETHODIMP CAgentFolder::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  if (propID == kpidPrefix)
  {
    if (_flatMode)
    {
      UString prefix;
      GetPrefix(index, prefix);
      prop = prefix;
    }
  }
  else if (_proxyArchive2)
  {
    SET_realIndex_AND_folder_2
    unsigned arcIndex = folder->SubFiles[realIndex];
    const CProxyFile2 &item = _proxyArchive2->Files[arcIndex];
    if (!item.IsDir())
    {
      switch (propID)
      {
        case kpidIsDir: prop = false; break;
        case kpidName: prop = item.Name; break;
        default: return _agentSpec->GetArchive()->GetProperty(arcIndex, propID, value);
      }
    }
    else
    {
      const CProxyFolder2 &itemFolder = _proxyArchive2->Folders[item.FolderIndex];
      if (!_flatMode && propID == kpidSize)
        prop = itemFolder.Size;
      else if (!_flatMode && propID == kpidPackSize)
        prop = itemFolder.PackSize;
      else switch (propID)
      {
        case kpidIsDir: prop = true; break;
        case kpidNumSubDirs: prop = itemFolder.NumSubFolders; break;
        case kpidNumSubFiles: prop = itemFolder.NumSubFiles; break;
        case kpidName: prop = item.Name; break;
        case kpidCRC:
        {
          // if (itemFolder.IsLeaf)
          if (!item.Ignore)
          {
            RINOK(_agentSpec->GetArchive()->GetProperty(arcIndex, propID, value));
          }
          if (itemFolder.CrcIsDefined && value->vt == VT_EMPTY)
            prop = itemFolder.Crc;
          break;
        }
        default:
          // if (itemFolder.IsLeaf)
          if (!item.Ignore)
            return _agentSpec->GetArchive()->GetProperty(arcIndex, propID, value);
      }
    }
  }
  else
  {
  SET_realIndex_AND_folder
  if (realIndex < (UInt32)folder->Folders.Size())
  {
    const CProxyFolder &item = _proxyArchive->Folders[folder->Folders[realIndex]];
    if (!_flatMode && propID == kpidSize)
      prop = item.Size;
    else if (!_flatMode && propID == kpidPackSize)
      prop = item.PackSize;
    else
    switch (propID)
    {
      case kpidIsDir: prop = true; break;
      case kpidNumSubDirs: prop = item.NumSubFolders; break;
      case kpidNumSubFiles: prop = item.NumSubFiles; break;
      case kpidName: prop = item.Name; break;
      case kpidCRC:
      {
        if (item.IsLeaf)
        {
          RINOK(_agentSpec->GetArchive()->GetProperty(item.Index, propID, value));
        }
        if (item.CrcIsDefined && value->vt == VT_EMPTY)
          prop = item.Crc;
        break;
      }
      default:
        if (item.IsLeaf)
          return _agentSpec->GetArchive()->GetProperty(item.Index, propID, value);
    }
  }
  else
  {
    const CProxyFile &item = folder->Files[realIndex - folder->Folders.Size()];
    switch (propID)
    {
      case kpidIsDir: prop = false; break;
      case kpidName: prop = item.Name; break;
      default:
        return _agentSpec->GetArchive()->GetProperty(item.Index, propID, value);
    }
  }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

static UInt64 GetUInt64Prop(IInArchive *archive, UInt32 index, PROPID propID)
{
  NCOM::CPropVariant prop;
  if (archive->GetProperty(index, propID, &prop) != S_OK)
    throw 111233443;
  UInt64 v = 0;
  if (ConvertPropVariantToUInt64(prop, v))
    return v;
  return 0;
}

STDMETHODIMP CAgentFolder::GetItemName(UInt32 index, const wchar_t **name, unsigned *len)
{
  if (_proxyArchive2)
  {
    SET_realIndex_AND_folder_2
    unsigned arcIndex = folder->SubFiles[realIndex];
    const CProxyFile2 &item = _proxyArchive2->Files[arcIndex];
    *name = item.Name;
    *len = item.NameSize;
    return S_OK;
  }
  else
  {
    SET_realIndex_AND_folder
    if (realIndex < (UInt32)folder->Folders.Size())
    {
      const CProxyFolder &item = _proxyArchive->Folders[folder->Folders[realIndex]];
      *name = item.Name;
      *len = item.Name.Len();
      return S_OK;
    }
    else
    {
      const CProxyFile &item = folder->Files[realIndex - folder->Folders.Size()];
      *name = item.Name;
      *len = item.Name.Len();
      return S_OK;
    }
  }
}

STDMETHODIMP CAgentFolder::GetItemPrefix(UInt32 index, const wchar_t **name, unsigned *len)
{
  *name = 0;
  *len = 0;
  if (!_flatMode)
    return S_OK;
  if (_proxyArchive2)
  {
    SET_realIndex_AND_folder_2

    unsigned arcIndex = folder->SubFiles[realIndex];
    const CProxyFile2 &item = _proxyArchive2->Files[arcIndex];
    if (item.Parent >= 0)
    {
      const CProxyFile2 &item2 = _proxyArchive2->Files[item.Parent];
      int foldInd = item2.GetFolderIndex(item.IsAltStream);
      if (foldInd >= 0)
      {
        const UString &s = _proxyArchive2->Folders[foldInd].PathPrefix;
        unsigned baseLen = _proxyArchive2->Folders[_proxyFolderItem].PathPrefix.Len();
        if (baseLen <= s.Len())
        {
          *name = (const wchar_t *)s + baseLen;
          *len = s.Len() - baseLen;
        }
      }
    }
  }
  return S_OK;
}

static int CompareRawProps(IArchiveGetRawProps *rawProps, int arcIndex1, int arcIndex2, PROPID propID)
{
  // if (propID == kpidSha1)
  if (rawProps)
  {
    const void *p1, *p2;
    UInt32 size1, size2;
    UInt32 propType1, propType2;
    HRESULT res1 = rawProps->GetRawProp(arcIndex1, propID, &p1, &size1, &propType1);
    HRESULT res2 = rawProps->GetRawProp(arcIndex2, propID, &p2, &size2, &propType2);
    if (res1 == S_OK && res2 == S_OK)
    {
      for (UInt32 i = 0; i < size1 && i < size2; i++)
      {
        Byte b1 = ((const Byte *)p1)[i];
        Byte b2 = ((const Byte *)p2)[i];
        if (b1 < b2) return -1;
        if (b1 > b2) return 1;
      }
      if (size1 < size2) return -1;
      if (size1 > size2) return 1;
      return 0;
    }
  }
  return 0;
}

// returns pointer to extension including '.'

static const wchar_t *GetExtension(const wchar_t *name)
{
  for (const wchar_t *dotPtr = NULL;; name++)
  {
    wchar_t c = *name;
    if (c == 0)
      return dotPtr ? dotPtr : name;
    if (c == '.')
      dotPtr = name;
  }
}

int CAgentFolder::CompareItems2(UInt32 index1, UInt32 index2, PROPID propID, Int32 propIsRaw)
{
  UInt32 realIndex1, realIndex2;
  const CProxyFolder2 *folder1, *folder2;
  
  if (_flatMode)
  {
    const CProxyItem &item1 = _items[index1];
    const CProxyItem &item2 = _items[index2];
    folder1 = &_proxyArchive2->Folders[item1.ProxyFolderIndex];
    folder2 = &_proxyArchive2->Folders[item2.ProxyFolderIndex];
    realIndex1 = item1.Index;
    realIndex2 = item2.Index;
  }
  else
  {
    folder2 = folder1 = &_proxyArchive2->Folders[_proxyFolderItem];
    realIndex1 = index1;
    realIndex2 = index2;
  }

  UInt32 arcIndex1;
  UInt32 arcIndex2;
  bool isDir1, isDir2;
  arcIndex1 = folder1->SubFiles[realIndex1];
  arcIndex2 = folder2->SubFiles[realIndex2];
  const CProxyFile2 &prox1 = _proxyArchive2->Files[arcIndex1];
  const CProxyFile2 &prox2 = _proxyArchive2->Files[arcIndex2];

  if (propID == kpidName)
  {
    return CompareFileNames_ForFolderList(prox1.Name, prox2.Name);
  }
  
  if (propID == kpidPrefix)
  {
    if (!_flatMode)
      return 0;
    if (prox1.Parent < 0) return prox2.Parent < 0 ? 0 : -1;
    if (prox2.Parent < 0) return 1;

    const CProxyFile2 &proxPar1 = _proxyArchive2->Files[prox1.Parent];
    const CProxyFile2 &proxPar2 = _proxyArchive2->Files[prox2.Parent];
    return CompareFileNames_ForFolderList(
        _proxyArchive2->Folders[proxPar1.GetFolderIndex(prox1.IsAltStream)].PathPrefix,
        _proxyArchive2->Folders[proxPar2.GetFolderIndex(prox2.IsAltStream)].PathPrefix);
  }
  
  if (propID == kpidExtension)
  {
     return CompareFileNames_ForFolderList(
         GetExtension(prox1.Name),
         GetExtension(prox2.Name));
  }

  isDir1 = prox1.IsDir();
  isDir2 = prox2.IsDir();

  if (propID == kpidIsDir)
  {
    if (isDir1 == isDir2)
      return 0;
    return isDir1 ? -1 : 1;
  }

  const CProxyFolder2 *proxFolder1 = NULL;
  const CProxyFolder2 *proxFolder2 = NULL;
  if (isDir1) proxFolder1 = &_proxyArchive2->Folders[prox1.FolderIndex];
  if (isDir2) proxFolder2 = &_proxyArchive2->Folders[prox2.FolderIndex];

  if (propID == kpidNumSubDirs)
  {
    UInt32 n1 = 0;
    UInt32 n2 = 0;
    if (isDir1) n1 = proxFolder1->NumSubFolders;
    if (isDir2) n2 = proxFolder2->NumSubFolders;
    return MyCompare(n1, n2);
  }
  
  if (propID == kpidNumSubFiles)
  {
    UInt32 n1 = 0;
    UInt32 n2 = 0;
    if (isDir1) n1 = proxFolder1->NumSubFiles;
    if (isDir2) n2 = proxFolder2->NumSubFiles;
    return MyCompare(n1, n2);
  }
  
  if (propID == kpidSize)
  {
    UInt64 n1, n2;
    if (isDir1)
      n1 = _flatMode ? 0 : proxFolder1->Size;
    else
      n1 = GetUInt64Prop(_agentSpec->GetArchive(), arcIndex1, kpidSize);
    if (isDir2)
      n2 = _flatMode ? 0 : proxFolder2->Size;
    else
      n2 = GetUInt64Prop(_agentSpec->GetArchive(), arcIndex2, kpidSize);
    return MyCompare(n1, n2);
  }
  
  if (propID == kpidPackSize)
  {
    UInt64 n1, n2;
    if (isDir1)
      n1 = _flatMode ? 0 : proxFolder1->PackSize;
    else
      n1 = GetUInt64Prop(_agentSpec->GetArchive(), arcIndex1, kpidPackSize);
    if (isDir2)
      n2 = _flatMode ? 0 : proxFolder2->PackSize;
    else
      n2 = GetUInt64Prop(_agentSpec->GetArchive(), arcIndex2, kpidPackSize);
    return MyCompare(n1, n2);
  }

  if (propID == kpidCRC)
  {
    UInt64 n1, n2;
    if (!isDir1 || !prox1.Ignore)
      n1 = GetUInt64Prop(_agentSpec->GetArchive(), arcIndex1, kpidCRC);
    else
      n1 = proxFolder1->Crc;
    if (!isDir2 || !prox2.Ignore)
      n2 = GetUInt64Prop(_agentSpec->GetArchive(), arcIndex2, kpidCRC);
    else
      n2 = proxFolder2->Crc;
    return MyCompare(n1, n2);
  }

  if (propIsRaw)
    return CompareRawProps(_agentSpec->_archiveLink.GetArchiveGetRawProps(), arcIndex1, arcIndex2, propID);

  NCOM::CPropVariant prop1, prop2;
  // Name must be first property
  GetProperty(index1, propID, &prop1);
  GetProperty(index2, propID, &prop2);
  if (prop1.vt != prop2.vt)
  {
    return MyCompare(prop1.vt, prop2.vt);
  }
  if (prop1.vt == VT_BSTR)
  {
    return _wcsicmp(prop1.bstrVal, prop2.bstrVal);
  }
  return prop1.Compare(prop2);
}

STDMETHODIMP_(Int32) CAgentFolder::CompareItems(UInt32 index1, UInt32 index2, PROPID propID, Int32 propIsRaw)
{
  COM_TRY_BEGIN
  if (_proxyArchive2)
    return CompareItems2(index1, index2, propID, propIsRaw);
  UInt32 realIndex1, realIndex2;
  const CProxyFolder *folder1, *folder2;
  
  if (_flatMode)
  {
    const CProxyItem &item1 = _items[index1];
    const CProxyItem &item2 = _items[index2];
    folder1 = &_proxyArchive->Folders[item1.ProxyFolderIndex];
    folder2 = &_proxyArchive->Folders[item2.ProxyFolderIndex];
    realIndex1 = item1.Index;
    realIndex2 = item2.Index;
  }
  else
  {
    folder2 = folder1 = &_proxyArchive->Folders[_proxyFolderItem];
    realIndex1 = index1;
    realIndex2 = index2;
  }
  if (propID == kpidPrefix)
  {
    if (!_flatMode)
      return 0;
    UString prefix1, prefix2;
    GetPrefix(index1, prefix1);
    GetPrefix(index2, prefix2);
    return CompareFileNames_ForFolderList(prefix1, prefix2);
  }
  const CProxyFile *prox1;
  const CProxyFile *prox2;
  const CProxyFolder *proxFolder1 = NULL;
  const CProxyFolder *proxFolder2 = NULL;
  bool isDir1, isDir2;
  if (realIndex1 < (UInt32)folder1->Folders.Size())
  {
    isDir1 = true;
    prox1 = proxFolder1 = &_proxyArchive->Folders[folder1->Folders[realIndex1]];
  }
  else
  {
    isDir1 = false;
    prox1 = &folder1->Files[realIndex1 - folder1->Folders.Size()];
  }

  if (realIndex2 < (UInt32)folder2->Folders.Size())
  {
    isDir2 = true;
    prox2 = proxFolder2 = &_proxyArchive->Folders[folder2->Folders[realIndex2]];
  }
  else
  {
    isDir2 = false;
    prox2 = &folder2->Files[realIndex2 - folder2->Folders.Size()];
  }

  if (propID == kpidName)
  {
    return CompareFileNames_ForFolderList(prox1->Name, prox2->Name);
  }
  if (propID == kpidExtension)
  {
     return CompareFileNames_ForFolderList(
         GetExtension(prox1->Name),
         GetExtension(prox2->Name));
  }

  if (propID == kpidIsDir)
  {
    if (isDir1 == isDir2)
      return 0;
    return isDir1 ? -1 : 1;
  }
  if (propID == kpidNumSubDirs)
  {
    UInt32 n1 = 0;
    UInt32 n2 = 0;
    if (isDir1) n1 = proxFolder1->NumSubFolders;
    if (isDir2) n2 = proxFolder2->NumSubFolders;
    return MyCompare(n1, n2);
  }
  if (propID == kpidNumSubFiles)
  {
    UInt32 n1 = 0;
    UInt32 n2 = 0;
    if (isDir1) n1 = proxFolder1->NumSubFiles;
    if (isDir2) n2 = proxFolder2->NumSubFiles;
    return MyCompare(n1, n2);
  }
  if (propID == kpidSize)
  {
    UInt64 n1, n2;
    if (isDir1)
      n1 = _flatMode ? 0 : proxFolder1->Size;
    else
      n1 = GetUInt64Prop(_agentSpec->GetArchive(), prox1->Index, kpidSize);
    if (isDir2)
      n2 = _flatMode ? 0 : proxFolder2->Size;
    else
      n2 = GetUInt64Prop(_agentSpec->GetArchive(), prox2->Index, kpidSize);
    return MyCompare(n1, n2);
  }
  if (propID == kpidPackSize)
  {
    UInt64 n1, n2;
    if (isDir1)
      n1 = _flatMode ? 0 : proxFolder1->PackSize;
    else
      n1 = GetUInt64Prop(_agentSpec->GetArchive(), prox1->Index, kpidPackSize);
    if (isDir2)
      n2 = _flatMode ? 0 : proxFolder2->PackSize;
    else
      n2 = GetUInt64Prop(_agentSpec->GetArchive(), prox2->Index, kpidPackSize);
    return MyCompare(n1, n2);
  }

  if (propID == kpidCRC)
  {
    UInt64 n1, n2;
    if (!isDir1 || proxFolder1->IsLeaf)
      n1 = GetUInt64Prop(_agentSpec->GetArchive(), prox1->Index, kpidCRC);
    else
      n1 = proxFolder1->Crc;
    if (!isDir2 || proxFolder2->IsLeaf)
      n2 = GetUInt64Prop(_agentSpec->GetArchive(), prox2->Index, kpidCRC);
    else
      n2 = proxFolder2->Crc;
    return MyCompare(n1, n2);
  }

  if (propIsRaw)
    return CompareRawProps(_agentSpec->_archiveLink.GetArchiveGetRawProps(), prox1->Index, prox2->Index, propID);

  NCOM::CPropVariant prop1, prop2;
  // Name must be first property
  GetProperty(index1, propID, &prop1);
  GetProperty(index2, propID, &prop2);
  if (prop1.vt != prop2.vt)
  {
    return MyCompare(prop1.vt, prop2.vt);
  }
  if (prop1.vt == VT_BSTR)
  {
    return _wcsicmp(prop1.bstrVal, prop2.bstrVal);
  }
  return prop1.Compare(prop2);

  COM_TRY_END
}

HRESULT CAgentFolder::BindToFolder_Internal(unsigned proxyFolderIndex, IFolderFolder **resultFolder)
{
  CMyComPtr<IFolderFolder> parentFolder;

  if (_proxyArchive2)
  {
    parentFolder = NULL; // Change it;
    const CProxyFolder2 &folder = _proxyArchive2->Folders[proxyFolderIndex];
    int par = _proxyArchive2->GetParentFolderOfFile(folder.ArcIndex);
    if (par != (int)_proxyFolderItem)
    {
      // return E_FAIL;
      RINOK(BindToFolder_Internal(par, &parentFolder));
    }
    else
      parentFolder = this;
  }
  else
  {
    const CProxyFolder &folder = _proxyArchive->Folders[proxyFolderIndex];
    if (folder.Parent != (int)_proxyFolderItem)
    {
      RINOK(BindToFolder_Internal(folder.Parent, &parentFolder));
    }
    else
      parentFolder = this;
  }
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> agentFolder = folderSpec;
  folderSpec->Init(_proxyArchive, _proxyArchive2, proxyFolderIndex, parentFolder, _agentSpec);
  *resultFolder = agentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  if (_proxyArchive2)
  {
    SET_realIndex_AND_folder_2
    unsigned arcIndex = folder->SubFiles[realIndex];
    const CProxyFile2 &item = _proxyArchive2->Files[arcIndex];
    if (!item.IsDir())
      return E_INVALIDARG;
    return BindToFolder_Internal(item.FolderIndex, resultFolder);
  }
  SET_realIndex_AND_folder
  if (realIndex >= (UInt32)folder->Folders.Size())
    return E_INVALIDARG;
  return BindToFolder_Internal(folder->Folders[realIndex], resultFolder);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  if (_proxyArchive2)
  {
    const CProxyFolder2 &folder = _proxyArchive2->Folders[_proxyFolderItem];
    FOR_VECTOR (i, folder.SubFiles)
    {
      const CProxyFile2 &file = _proxyArchive2->Files[folder.SubFiles[i]];
      if (file.FolderIndex >= 0)
        if (StringsAreEqualNoCase(file.Name, name))
          return BindToFolder_Internal(file.FolderIndex, resultFolder);
    }
    return E_INVALIDARG;
  }
  int index = _proxyArchive->FindDirSubItemIndex(_proxyFolderItem, name);
  if (index < 0)
    return E_INVALIDARG;
  return BindToFolder_Internal(index, resultFolder);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  // COM_TRY_BEGIN
  CMyComPtr<IFolderFolder> parentFolder = _parentFolder;
  *resultFolder = parentFolder.Detach();
  return S_OK;
  // COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetStream(UInt32 index, ISequentialInStream **stream)
{
  CMyComPtr<IInArchiveGetStream> getStream;
  _agentSpec->GetArchive()->QueryInterface(IID_IInArchiveGetStream, (void **)&getStream);
  if (!getStream)
    return S_OK;

  UInt32 indexInArchive;
  if (_proxyArchive2)
  {
    SET_realIndex_AND_folder_2
    indexInArchive = folder->SubFiles[realIndex];
  }
  else
  {
  SET_realIndex_AND_folder

  if (realIndex < (UInt32)folder->Folders.Size())
  {
    const CProxyFolder &item = _proxyArchive->Folders[folder->Folders[realIndex]];
    if (!item.IsLeaf)
      return S_OK;
    indexInArchive = item.Index;
  }
  else
    indexInArchive = folder->Files[realIndex - folder->Folders.Size()].Index;
  }
  return getStream->GetStream(indexInArchive, stream);
}

static const PROPID kProps[] =
{
  kpidNumSubDirs,
  kpidNumSubFiles,
  kpidPrefix
};

struct CArchiveItemPropertyTemp
{
  UString Name;
  PROPID ID;
  VARTYPE Type;
};

STDMETHODIMP CAgentFolder::GetNumberOfProperties(UInt32 *numProps)
{
  COM_TRY_BEGIN
  RINOK(_agentSpec->GetArchive()->GetNumberOfProperties(numProps));
  *numProps += ARRAY_SIZE(kProps);
  if (!_flatMode)
    (*numProps)--;
  /*
  bool thereIsPathProp = _proxyArchive2 ?
    _agentSpec->_proxyArchive2->ThereIsPathProp :
    _agentSpec->_proxyArchive->ThereIsPathProp;
  */

  // if there is kpidPath, we change kpidPath to kpidName
  // if there is no kpidPath, we add kpidName.
  if (!_agentSpec->ThereIsPathProp)
    (*numProps)++;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  UInt32 numProps;
  _agentSpec->GetArchive()->GetNumberOfProperties(&numProps);

  /*
  bool thereIsPathProp = _proxyArchive2 ?
    _agentSpec->_proxyArchive2->ThereIsPathProp :
    _agentSpec->_proxyArchive->ThereIsPathProp;
  */

  if (!_agentSpec->ThereIsPathProp)
  {
    if (index == 0)
    {
      *propID = kpidName;
      *varType = VT_BSTR;
      *name = 0;
      return S_OK;
    }
    index--;
  }

  if (index < numProps)
  {
    RINOK(_agentSpec->GetArchive()->GetPropertyInfo(index, name, propID, varType));
    if (*propID == kpidPath)
      *propID = kpidName;
  }
  else
  {
    *propID = kProps[index - numProps];
    *varType = k7z_PROPID_To_VARTYPE[(unsigned)*propID];
    *name = 0;
  }
  return S_OK;
  COM_TRY_END
}

static const PROPID kFolderProps[] =
{
  kpidSize,
  kpidPackSize,
  kpidNumSubDirs,
  kpidNumSubFiles,
  kpidCRC
};

STDMETHODIMP CAgentFolder::GetFolderProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN

  NWindows::NCOM::CPropVariant prop;

  if (_proxyArchive2)
  {
    const CProxyFolder2 &folder = _proxyArchive2->Folders[_proxyFolderItem];
    if (propID == kpidName)
    {
      if (folder.ArcIndex >= 0)
        prop = _proxyArchive2->Files[folder.ArcIndex].Name;
    }
    else if (propID == kpidPath)
    {
      prop = _proxyArchive2->GetFullPathPrefix(_proxyFolderItem);
    }
    else switch (propID)
    {
      case kpidSize:         prop = folder.Size; break;
      case kpidPackSize:     prop = folder.PackSize; break;
      case kpidNumSubDirs:   prop = folder.NumSubFolders; break;
      case kpidNumSubFiles:  prop = folder.NumSubFiles; break;
        // case kpidName:         prop = folder.Name; break;
      // case kpidPath:         prop = _proxyArchive2->GetFullPathPrefix(_proxyFolderItem); break;
      case kpidType: prop = UString(L"7-Zip.") + _agentSpec->ArchiveType; break;
      case kpidCRC: if (folder.CrcIsDefined) prop = folder.Crc; break;
    }
    
  }
  else
  {
  const CProxyFolder &folder = _proxyArchive->Folders[_proxyFolderItem];
  switch (propID)
  {
    case kpidSize:         prop = folder.Size; break;
    case kpidPackSize:     prop = folder.PackSize; break;
    case kpidNumSubDirs:   prop = folder.NumSubFolders; break;
    case kpidNumSubFiles:  prop = folder.NumSubFiles; break;
    case kpidName:         prop = folder.Name; break;
    case kpidPath:         prop = _proxyArchive->GetFullPathPrefix(_proxyFolderItem); break;
    case kpidType: prop = UString(L"7-Zip.") + _agentSpec->ArchiveType; break;
    case kpidCRC: if (folder.CrcIsDefined) prop = folder.Crc; break;
  }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetNumberOfFolderProperties(UInt32 *numProps)
{
  *numProps = ARRAY_SIZE(kFolderProps);
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetFolderPropertyInfo IMP_IFolderFolder_GetProp(kFolderProps)

STDMETHODIMP CAgentFolder::GetParent(UInt32 /* index */, UInt32 * /* parent */, UInt32 * /* parentType */)
{
  return E_FAIL;
}


STDMETHODIMP CAgentFolder::GetNumRawProps(UInt32 *numProps)
{
  IArchiveGetRawProps *rawProps = _agentSpec->_archiveLink.GetArchiveGetRawProps();
  if (rawProps)
    return rawProps->GetNumRawProps(numProps);
  *numProps = 0;
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *propID)
{
  IArchiveGetRawProps *rawProps = _agentSpec->_archiveLink.GetArchiveGetRawProps();
  if (rawProps)
    return rawProps->GetRawPropInfo(index, name, propID);
  return E_FAIL;
}

STDMETHODIMP CAgentFolder::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType)
{
  IArchiveGetRawProps *rawProps = _agentSpec->_archiveLink.GetArchiveGetRawProps();
  if (rawProps)
  {
    unsigned arcIndex;
    if (_proxyArchive2)
    {
      SET_realIndex_AND_folder_2
      arcIndex = folder->SubFiles[realIndex];
    }
    else
    {
      SET_realIndex_AND_folder
      if (realIndex < (UInt32)folder->Folders.Size())
      {
        const CProxyFolder &item = _proxyArchive->Folders[folder->Folders[realIndex]];
        if (!item.IsLeaf)
        {
          *data = NULL;
          *dataSize = 0;
          *propType = 0;
          return S_OK;
        }
        arcIndex = item.Index;
      }
      else
      {
        const CProxyFile &item = folder->Files[realIndex - folder->Folders.Size()];
        arcIndex = item.Index;
      }
    }
    return rawProps->GetRawProp(arcIndex, propID, data, dataSize, propType);
  }
  *data = NULL;
  *dataSize = 0;
  *propType = 0;
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetFolderArcProps(IFolderArcProps **object)
{
  CMyComPtr<IFolderArcProps> temp = _agentSpec;
  *object = temp.Detach();
  return S_OK;
}

#ifdef NEW_FOLDER_INTERFACE

STDMETHODIMP CAgentFolder::SetFlatMode(Int32 flatMode)
{
  _flatMode = IntToBool(flatMode);
  return S_OK;
}

#endif

int CAgentFolder::GetRealIndex(unsigned index) const
{
  if (!_flatMode)
  {
    if (_proxyArchive2)
      return _proxyArchive2->GetRealIndex(_proxyFolderItem, index);
    else
      return _proxyArchive->GetRealIndex(_proxyFolderItem, index);
  }
  {
    const CProxyItem &item = _items[index];
    if (_proxyArchive2)
    {
      const CProxyFolder2 *folder = &_proxyArchive2->Folders[item.ProxyFolderIndex];
      return folder->SubFiles[item.Index];
    }
    else
    {
      const CProxyFolder *folder = &_proxyArchive->Folders[item.ProxyFolderIndex];
      unsigned realIndex = item.Index;
      if (realIndex < folder->Folders.Size())
      {
        const CProxyFolder &f = _proxyArchive->Folders[folder->Folders[realIndex]];
        if (!f.IsLeaf)
          return -1;
        return f.Index;
      }
      return folder->Files[realIndex - folder->Folders.Size()].Index;
    }
  }
}

void CAgentFolder::GetRealIndices(const UInt32 *indices, UInt32 numItems, bool includeAltStreams, bool includeFolderSubItemsInFlatMode, CUIntVector &realIndices) const
{
  if (!_flatMode)
  {
    if (_proxyArchive2)
      _proxyArchive2->GetRealIndices(_proxyFolderItem, indices, numItems, includeAltStreams, realIndices);
    else
      _proxyArchive->GetRealIndices(_proxyFolderItem, indices, numItems, realIndices);
    return;
  }
  realIndices.Clear();
  for (UInt32 i = 0; i < numItems; i++)
  {
    const CProxyItem &item = _items[indices[i]];
    if (_proxyArchive2)
    {
      const CProxyFolder2 *folder = &_proxyArchive2->Folders[item.ProxyFolderIndex];
      _proxyArchive2->AddRealIndices_of_ArcItem(folder->SubFiles[item.Index], includeAltStreams, realIndices);
      continue;
    }
    UInt32 arcIndex;
    {
      const CProxyFolder *folder = &_proxyArchive->Folders[item.ProxyFolderIndex];
      unsigned realIndex = item.Index;
      if (realIndex < folder->Folders.Size())
      {
        if (includeFolderSubItemsInFlatMode)
        {
          _proxyArchive->AddRealIndices(folder->Folders[realIndex], realIndices);
          continue;
        }
        const CProxyFolder &f = _proxyArchive->Folders[folder->Folders[realIndex]];
        if (!f.IsLeaf)
          continue;
        arcIndex = f.Index;
      }
      else
        arcIndex = folder->Files[realIndex - folder->Folders.Size()].Index;
    }
    realIndices.Add(arcIndex);
  }
  HeapSort(&realIndices.Front(), realIndices.Size());
}

STDMETHODIMP CAgentFolder::Extract(const UInt32 *indices,
    UInt32 numItems,
    Int32 includeAltStreams,
    Int32 replaceAltStreamColon,
    NExtract::NPathMode::EEnum pathMode,
    NExtract::NOverwriteMode::EEnum overwriteMode,
    const wchar_t *path,
    Int32 testMode,
    IFolderArchiveExtractCallback *extractCallback2)
{
  COM_TRY_BEGIN
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UStringVector pathParts;
  if (_proxyArchive2)
    _proxyArchive2->GetPathParts(_proxyFolderItem, pathParts);
  else
    _proxyArchive->GetPathParts(_proxyFolderItem, pathParts);

  /*
  if (_flatMode)
    pathMode = NExtract::NPathMode::kNoPathnames;
  */

  extractCallbackSpec->InitForMulti(false, pathMode, overwriteMode);

  FString pathU;
  if (path)
    pathU = us2fs(path);

  CExtractNtOptions extractNtOptions;
  extractNtOptions.AltStreams.Val = IntToBool(includeAltStreams); // change it!!!
  extractNtOptions.AltStreams.Def = true;

  extractNtOptions.ReplaceColonForAltStream = IntToBool(replaceAltStreamColon);
  
  extractCallbackSpec->Init(
      extractNtOptions,
      NULL, &_agentSpec->GetArc(),
      extractCallback2,
      false, // stdOutMode
      IntToBool(testMode),
      path ? pathU : FTEXT(""),
      pathParts,
      (UInt64)(Int64)-1);
  
  if (_proxyArchive2)
    extractCallbackSpec->SetBaseParentFolderIndex(_proxyArchive2->Folders[_proxyFolderItem].ArcIndex);

  CUIntVector realIndices;
  GetRealIndices(indices, numItems, IntToBool(includeAltStreams),
      false, // includeFolderSubItemsInFlatMode
      realIndices); //

  #ifdef SUPPORT_LINKS

  if (!testMode)
  {
    RINOK(extractCallbackSpec->PrepareHardLinks(&realIndices));
  }
    
  #endif

  HRESULT result = _agentSpec->GetArchive()->Extract(&realIndices.Front(),
      realIndices.Size(), testMode, extractCallback);
  if (result == S_OK)
    result = extractCallbackSpec->SetDirsTimes();
  return result;
  COM_TRY_END
}

/////////////////////////////////////////
// CAgent

CAgent::CAgent():
  _proxyArchive(NULL),
  _proxyArchive2(NULL),
  _isDeviceFile(false),
  _codecs(0)
{
}

CAgent::~CAgent()
{
  if (_proxyArchive)
    delete _proxyArchive;
  if (_proxyArchive2)
    delete _proxyArchive2;
}

bool CAgent::CanUpdate() const
{
  // FAR plugin uses empty agent to create new archive !!!
  if (_archiveLink.Arcs.Size() == 0)
    return true;
  if (_isDeviceFile)
    return false;
  if (_archiveLink.Arcs.Size() != 1)
    return false;
  if (_archiveLink.Arcs[0].ErrorInfo.ThereIsTail)
    return false;
  return true;
}

STDMETHODIMP CAgent::Open(
    IInStream *inStream,
    const wchar_t *filePath,
    const wchar_t *arcFormat,
    BSTR *archiveType,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  _archiveFilePath = filePath;
  NFile::NFind::CFileInfo fi;
  _isDeviceFile = false;
  if (!inStream)
  {
    if (!fi.Find(us2fs(_archiveFilePath)))
      return ::GetLastError();
    if (fi.IsDir())
      return E_FAIL;
    _isDeviceFile = fi.IsDevice;
  }
  CArcInfoEx archiverInfo0, archiverInfo1;

  _compressCodecsInfo.Release();
  _codecs = new CCodecs;
  _compressCodecsInfo = _codecs;
  RINOK(_codecs->Load());

  CObjectVector<COpenType> types;
  if (!ParseOpenTypes(*_codecs, arcFormat, types))
    return S_FALSE;

  /*
  CObjectVector<COptionalOpenProperties> optProps;
  if (Read_ShowDeleted())
  {
    COptionalOpenProperties &optPair = optProps.AddNew();
    optPair.FormatName = L"ntfs";
    // optPair.Props.AddNew().Name = L"LS";
    optPair.Props.AddNew().Name = L"LD";
  }
  */

  COpenOptions options;
  options.props = NULL;
  options.codecs = _codecs;
  options.types = &types;
  CIntVector exl;
  options.excludedFormats = &exl;
  options.stdInMode = false;
  options.stream = inStream;
  options.filePath = _archiveFilePath;
  options.callback = openArchiveCallback;

  RINOK(_archiveLink.Open(options));

  CArc &arc = _archiveLink.Arcs.Back();
  if (!inStream)
  {
    arc.MTimeDefined = !fi.IsDevice;
    arc.MTime = fi.MTime;
  }

  ArchiveType = GetTypeOfArc(arc);
  if (archiveType)
  {
    RINOK(StringToBstr(ArchiveType, archiveType));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::ReOpen(IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  if (_proxyArchive2)
  {
    delete _proxyArchive2;
    _proxyArchive2 = NULL;
  }
  if (_proxyArchive)
  {
    delete _proxyArchive;
    _proxyArchive = NULL;
  }

  CObjectVector<COpenType> incl;
  CIntVector exl;

  COpenOptions options;
  options.props = NULL;
  options.codecs = _codecs;
  options.types = &incl;
  options.excludedFormats = &exl;
  options.stdInMode = false;
  options.filePath = _archiveFilePath;
  options.callback = openArchiveCallback;

  RINOK(_archiveLink.ReOpen(options));
  return ReadItems();
  COM_TRY_END
}

STDMETHODIMP CAgent::Close()
{
  COM_TRY_BEGIN
  return _archiveLink.Close();
  COM_TRY_END
}

/*
STDMETHODIMP CAgent::EnumProperties(IEnumSTATPROPSTG **EnumProperties)
{
  return _archive->EnumProperties(EnumProperties);
}
*/

HRESULT CAgent::ReadItems()
{
  if (_proxyArchive || _proxyArchive2)
    return S_OK;
  
  const CArc &arc = GetArc();
  bool useProxy2 = (arc.GetRawProps && arc.IsTree);
  // useProxy2 = false;

  if (useProxy2)
    _proxyArchive2 = new CProxyArchive2();
  else
    _proxyArchive = new CProxyArchive();

  {
    ThereIsPathProp = false;
    UInt32 numProps;
    arc.Archive->GetNumberOfProperties(&numProps);
    for (UInt32 i = 0; i < numProps; i++)
    {
      CMyComBSTR name;
      PROPID propID;
      VARTYPE varType;
      RINOK(arc.Archive->GetPropertyInfo(i, &name, &propID, &varType));
      if (propID == kpidPath)
      {
        ThereIsPathProp = true;
        break;
      }
    }
  }

  if (_proxyArchive2)
    return _proxyArchive2->Load(GetArc(), NULL);
  return _proxyArchive->Load(GetArc(), NULL);
}

STDMETHODIMP CAgent::BindToRootFolder(IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  RINOK(ReadItems());
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> rootFolder = folderSpec;
  folderSpec->Init(_proxyArchive, _proxyArchive2, 0, NULL, this);
  *resultFolder = rootFolder.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::Extract(
    NExtract::NPathMode::EEnum pathMode,
    NExtract::NOverwriteMode::EEnum overwriteMode,
    const wchar_t *path,
    Int32 testMode,
    IFolderArchiveExtractCallback *extractCallback2)
{
  COM_TRY_BEGIN
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  extractCallbackSpec->InitForMulti(false, pathMode, overwriteMode);

  CExtractNtOptions extractNtOptions;
  extractNtOptions.AltStreams.Val = true; // change it!!!
  extractNtOptions.AltStreams.Def = true; // change it!!!
  extractNtOptions.ReplaceColonForAltStream = false; // change it!!!

  extractCallbackSpec->Init(
      extractNtOptions,
      NULL, &GetArc(),
      extractCallback2,
      false, // stdOutMode
      IntToBool(testMode),
      us2fs(path),
      UStringVector(),
      (UInt64)(Int64)-1);

  #ifdef SUPPORT_LINKS

  if (!testMode)
  {
    RINOK(extractCallbackSpec->PrepareHardLinks(NULL)); // NULL means all items
  }
    
  #endif

  return GetArchive()->Extract(0, (UInt32)(Int32)-1, testMode, extractCallback);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetNumberOfProperties(UInt32 *numProps)
{
  COM_TRY_BEGIN
  return GetArchive()->GetNumberOfProperties(numProps);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetPropertyInfo(UInt32 index,
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  RINOK(GetArchive()->GetPropertyInfo(index, name, propID, varType));
  if (*propID == kpidPath)
    *propID = kpidName;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::GetArcNumLevels(UInt32 *numLevels)
{
  *numLevels = _archiveLink.Arcs.Size();
  return S_OK;
}

STDMETHODIMP CAgent::GetArcProp(UInt32 level, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  if (level > (UInt32)_archiveLink.Arcs.Size())
    return E_INVALIDARG;
  if (level == (UInt32)_archiveLink.Arcs.Size())
  {
    switch (propID)
    {
      case kpidPath:
        if (!_archiveLink.NonOpen_ArcPath.IsEmpty())
          prop = _archiveLink.NonOpen_ArcPath;
        break;
      case kpidErrorType:
        if (_archiveLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
          prop = _codecs->Formats[_archiveLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name;
        break;
    }
  }
  else
  {
    const CArc &arc = _archiveLink.Arcs[level];
    switch (propID)
    {
      case kpidType: prop = GetTypeOfArc(arc); break;
      case kpidPath: prop = arc.Path; break;
      case kpidErrorType: if (arc.ErrorInfo.ErrorFormatIndex >= 0) prop = _codecs->Formats[arc.ErrorInfo.ErrorFormatIndex].Name; break;
      case kpidErrorFlags:
      {
        UInt32 flags = arc.ErrorInfo.GetErrorFlags();
        if (flags != 0)
          prop = flags;
        break;
      }
      case kpidWarningFlags:
      {
        UInt32 flags = arc.ErrorInfo.GetWarningFlags();
        if (flags != 0)
          prop = flags;
        break;
      }
      case kpidOffset:
      {
        Int64 v = arc.GetGlobalOffset();
        if (v != 0)
          prop = v;
        break;
      }
      case kpidTailSize:
      {
        if (arc.ErrorInfo.TailSize != 0)
          prop = arc.ErrorInfo.TailSize;
        break;
      }
      default: return arc.Archive->GetArchiveProperty(propID, value);
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::GetArcNumProps(UInt32 level, UInt32 *numProps)
{
  return _archiveLink.Arcs[level].Archive->GetNumberOfArchiveProperties(numProps);
}

STDMETHODIMP CAgent::GetArcPropInfo(UInt32 level, UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return _archiveLink.Arcs[level].Archive->GetArchivePropertyInfo(index, name, propID, varType);
}

// MainItemProperty
STDMETHODIMP CAgent::GetArcProp2(UInt32 level, PROPID propID, PROPVARIANT *value)
{
  return _archiveLink.Arcs[level - 1].Archive->GetProperty(_archiveLink.Arcs[level].SubfileIndex, propID, value);
}

STDMETHODIMP CAgent::GetArcNumProps2(UInt32 level, UInt32 *numProps)
{
  return _archiveLink.Arcs[level - 1].Archive->GetNumberOfProperties(numProps);
}

STDMETHODIMP CAgent::GetArcPropInfo2(UInt32 level, UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return _archiveLink.Arcs[level - 1].Archive->GetPropertyInfo(index, name, propID, varType);
}
