// Agent.cpp

#include "StdAfx.h"

#include "../../../../C/Sort.h"

#include "Common/ComTry.h"

#include "../Common/ArchiveExtractCallback.h"

#include "Agent.h"

using namespace NWindows;

STDMETHODIMP CAgentFolder::GetAgentFolder(CAgentFolder **agentFolder)
{
  *agentFolder = this;
  return S_OK;
}

void CAgentFolder::LoadFolder(CProxyFolder *folder)
{
  int i;
  CProxyItem item;
  item.Folder = folder;
  for (i = 0; i < folder->Folders.Size(); i++)
  {
    item.Index = i;
    _items.Add(item);
    LoadFolder(&folder->Folders[i]);
  }
  int start = folder->Folders.Size();
  for (i = 0; i < folder->Files.Size(); i++)
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
  else
    *numItems = _proxyFolderItem->Folders.Size() +_proxyFolderItem->Files.Size();
  return S_OK;
}

UString CAgentFolder::GetName(UInt32 index) const
{
  UInt32 realIndex;
  const CProxyFolder *folder;
  if (_flatMode)
  {
    const CProxyItem &item = _items[index];
    folder = item.Folder;
    realIndex = item.Index;
  }
  else
  {
    folder = _proxyFolderItem;
    realIndex = index;
  }

  if (realIndex < (UInt32)folder->Folders.Size())
    return folder->Folders[realIndex].Name;
  return folder->Files[realIndex - folder->Folders.Size()].Name;
}

UString CAgentFolder::GetPrefix(UInt32 index) const
{
  if (!_flatMode)
    return UString();
  const CProxyItem &item = _items[index];
  const CProxyFolder *folder = item.Folder;
  UString path;
  while (folder != _proxyFolderItem)
  {
    path = folder->Name + UString(WCHAR_PATH_SEPARATOR) + path;
    folder = folder->Parent;
  }
  return path;
}

UString CAgentFolder::GetFullPathPrefixPlusPrefix(UInt32 index) const
{
  return _proxyFolderItem->GetFullPathPrefix() + GetPrefix(index);
}

void CAgentFolder::GetPrefixIfAny(UInt32 index, NCOM::CPropVariant &prop) const
{
  if (!_flatMode)
    return;
  prop = GetPrefix(index);
}


STDMETHODIMP CAgentFolder::GetProperty(UInt32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  const CProxyFolder *folder;
  UInt32 realIndex;
  if (_flatMode)
  {
    const CProxyItem &item = _items[itemIndex];
    folder = item.Folder;
    realIndex = item.Index;
  }
  else
  {
    folder = _proxyFolderItem;
    realIndex = itemIndex;
  }

  if (realIndex < (UInt32)folder->Folders.Size())
  {
    const CProxyFolder &item = folder->Folders[realIndex];
    if (!_flatMode && propID == kpidSize)
      prop = item.Size;
    else if (!_flatMode && propID == kpidPackSize)
      prop = item.PackSize;
    else
    switch(propID)
    {
      case kpidIsDir:      prop = true; break;
      case kpidNumSubDirs: prop = item.NumSubFolders; break;
      case kpidNumSubFiles:   prop = item.NumSubFiles; break;
      case kpidName:          prop = item.Name; break;
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
      case kpidPrefix: GetPrefixIfAny(itemIndex, prop); break;

      default:
        if (item.IsLeaf)
          return _agentSpec->GetArchive()->GetProperty(item.Index, propID, value);
    }
  }
  else
  {
    realIndex -= folder->Folders.Size();
    const CProxyFile &item = folder->Files[realIndex];
    switch(propID)
    {
      case kpidIsDir: prop = false; break;
      case kpidName: prop = item.Name; break;
      case kpidPrefix: GetPrefixIfAny(itemIndex, prop); break;
      default:
        return _agentSpec->GetArchive()->GetProperty(item.Index, propID, value);
    }
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT CAgentFolder::BindToFolder(CProxyFolder *folder, IFolderFolder **resultFolder)
{
  CMyComPtr<IFolderFolder> parentFolder;
  if (folder->Parent != _proxyFolderItem)
  {
    RINOK(BindToFolder(folder->Parent, &parentFolder));
  }
  else
    parentFolder = this;
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> agentFolder = folderSpec;
  folderSpec->Init(_proxyArchive, folder, parentFolder, _agentSpec);
  *resultFolder = agentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN

  CProxyFolder *folder;
  UInt32 realIndex;
  if (_flatMode)
  {
    const CProxyItem &item = _items[index];
    folder = item.Folder;
    realIndex = item.Index;
  }
  else
  {
    folder = _proxyFolderItem;
    realIndex = index;
  }
  if (realIndex >= (UInt32)folder->Folders.Size())
    return E_INVALIDARG;
  return BindToFolder(&folder->Folders[realIndex], resultFolder);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  int index = _proxyFolderItem->FindDirSubItemIndex(name);
  if (index < 0)
    return E_INVALIDARG;
  return BindToFolder(index, resultFolder);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  CMyComPtr<IFolderFolder> parentFolder = _parentFolder;
  *resultFolder = parentFolder.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetStream(UInt32 index, ISequentialInStream **stream)
{
  CMyComPtr<IInArchiveGetStream> getStream;
  _agentSpec->GetArchive()->QueryInterface(IID_IInArchiveGetStream, (void **)&getStream);
  if (!getStream)
    return S_OK;

  const CProxyFolder *folder;
  UInt32 realIndex;
  if (_flatMode)
  {
    const CProxyItem &item = _items[index];
    folder = item.Folder;
    realIndex = item.Index;
  }
  else
  {
    folder = _proxyFolderItem;
    realIndex = index;
  }

  UInt32 indexInArchive;
  if (realIndex < (UInt32)folder->Folders.Size())
  {
    const CProxyFolder &item = folder->Folders[realIndex];
    if (!item.IsLeaf)
      return S_OK;
    indexInArchive = item.Index;
  }
  else
    indexInArchive = folder->Files[realIndex - folder->Folders.Size()].Index;
  return getStream->GetStream(indexInArchive, stream);
}

STATPROPSTG kProperties[] =
{
  { NULL, kpidNumSubDirs, VT_UI4},
  { NULL, kpidNumSubFiles, VT_UI4},
  { NULL, kpidPrefix, VT_BSTR}
};

static const UInt32 kNumProperties = sizeof(kProperties) / sizeof(kProperties[0]);

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
  *numProps += kNumProperties;
  if (!_flatMode)
    (*numProps)--;
  if (!_agentSpec->_proxyArchive->ThereIsPathProp)
    (*numProps)++;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  UInt32 numProps;
  _agentSpec->GetArchive()->GetNumberOfProperties(&numProps);
  if (!_agentSpec->_proxyArchive->ThereIsPathProp)
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
    const STATPROPSTG &srcItem = kProperties[index - numProps];
    *propID = srcItem.propid;
    *varType = srcItem.vt;
    *name = 0;
  }
  return S_OK;
  COM_TRY_END
}

STATPROPSTG kFolderProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidNumSubDirs, VT_UI4},
  { NULL, kpidNumSubFiles, VT_UI4},
  { NULL, kpidCRC, VT_UI4}
};

static const UInt32 kNumFolderProps = sizeof(kFolderProps) / sizeof(kFolderProps[0]);

STDMETHODIMP CAgentFolder::GetFolderProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidSize:         prop = _proxyFolderItem->Size; break;
    case kpidPackSize:     prop = _proxyFolderItem->PackSize; break;
    case kpidNumSubDirs:   prop = _proxyFolderItem->NumSubFolders; break;
    case kpidNumSubFiles:  prop = _proxyFolderItem->NumSubFiles; break;
    case kpidName:         prop = _proxyFolderItem->Name; break;
    case kpidPath:         prop = _proxyFolderItem->GetFullPathPrefix(); break;
    case kpidType: prop = UString(L"7-Zip.") + _agentSpec->ArchiveType; break;
    case kpidCRC: if (_proxyFolderItem->CrcIsDefined) prop = _proxyFolderItem->Crc; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetNumberOfFolderProperties(UInt32 *numProps)
{
  *numProps = kNumFolderProps;
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetFolderPropertyInfo(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType)
{
  // if (index < kNumFolderProps)
  {
    const STATPROPSTG &srcItem = kFolderProps[index];
    *propID = srcItem.propid;
    *varType = srcItem.vt;
    *name = 0;
    return S_OK;
  }
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

void CAgentFolder::GetRealIndices(const UInt32 *indices, UInt32 numItems, CUIntVector &realIndices) const
{
  if (!_flatMode)
  {
    _proxyFolderItem->GetRealIndices(indices, numItems, realIndices);
    return;
  }
  realIndices.Clear();
  for(UInt32 i = 0; i < numItems; i++)
  {
    const CProxyItem &item = _items[indices[i]];
    const CProxyFolder *folder = item.Folder;
    UInt32 realIndex = item.Index;
    if (realIndex < (UInt32)folder->Folders.Size())
      continue;
    realIndices.Add(folder->Files[realIndex - folder->Folders.Size()].Index);
  }
  HeapSort(&realIndices.Front(), realIndices.Size());
}

STDMETHODIMP CAgentFolder::Extract(const UInt32 *indices,
    UInt32 numItems,
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
  CProxyFolder *currentProxyFolder = _proxyFolderItem;
  while (currentProxyFolder->Parent)
  {
    pathParts.Insert(0, currentProxyFolder->Name);
    currentProxyFolder = currentProxyFolder->Parent;
  }
  
  /*
  if (_flatMode)
    pathMode = NExtract::NPathMode::kNoPathnames;
  */

  extractCallbackSpec->InitForMulti(false, pathMode, overwriteMode);

  extractCallbackSpec->Init(NULL, &_agentSpec->GetArc(),
      extractCallback2,
      false, testMode ? true : false, false,
      (path ? path : L""),
      pathParts,
      (UInt64)(Int64)-1);
  CUIntVector realIndices;
  GetRealIndices(indices, numItems, realIndices);
  return _agentSpec->GetArchive()->Extract(&realIndices.Front(),
      realIndices.Size(), testMode, extractCallback);
  COM_TRY_END
}

/////////////////////////////////////////
// CAgent

CAgent::CAgent():
  _proxyArchive(NULL),
  _codecs(0)
{
}

CAgent::~CAgent()
{
  if (_proxyArchive != NULL)
    delete _proxyArchive;
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
  NFile::NFind::CFileInfoW fi;
  if (!inStream)
  {
    if (!fi.Find(_archiveFilePath))
      return ::GetLastError();
    if (fi.IsDir())
      return E_FAIL;
  }
  CArcInfoEx archiverInfo0, archiverInfo1;

  _compressCodecsInfo.Release();
  _codecs = new CCodecs;
  _compressCodecsInfo = _codecs;
  RINOK(_codecs->Load());

  CIntVector formatIndices;
  if (!_codecs->FindFormatForArchiveType(arcFormat, formatIndices))
    return S_FALSE;

  RINOK(_archiveLink.Open(_codecs, formatIndices, false, inStream, _archiveFilePath, openArchiveCallback));

  CArc &arc = _archiveLink.Arcs.Back();
  if (!inStream)
  {
    arc.MTimeDefined = !fi.IsDevice;
    arc.MTime = fi.MTime;
  }

  ArchiveType = _codecs->Formats[arc.FormatIndex].Name;
  if (archiveType == 0)
    return S_OK;
  return StringToBstr(ArchiveType, archiveType);
  COM_TRY_END
}

STDMETHODIMP CAgent::ReOpen(IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  if (_proxyArchive != NULL)
  {
    delete _proxyArchive;
    _proxyArchive = NULL;
  }
  RINOK(_archiveLink.ReOpen(_codecs, _archiveFilePath, openArchiveCallback));
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
  if (_proxyArchive != NULL)
    return S_OK;
  _proxyArchive = new CProxyArchive();
  return _proxyArchive->Load(GetArc(), NULL);
}

STDMETHODIMP CAgent::BindToRootFolder(IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  RINOK(ReadItems());
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> rootFolder = folderSpec;
  folderSpec->Init(_proxyArchive, &_proxyArchive->RootFolder, NULL, this);
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
  extractCallbackSpec->Init(NULL, &GetArc(),
      extractCallback2,
      false, testMode ? true : false, false,
      path,
      UStringVector(),
      (UInt64)(Int64)-1);
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
  CArc &arc = _archiveLink.Arcs[level];
  switch(propID)
  {
    case kpidType: prop = GetTypeOfArc(arc); break;
    case kpidPath: prop = arc.Path; break;
    default: return arc.Archive->GetArchiveProperty(propID, value);
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
