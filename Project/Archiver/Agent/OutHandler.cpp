// Zip/Handler.cpp

#include "StdAfx.h"

#include "../Common/UpdatePairInfo.h"
#include "../Common/UpdatePairBasic.h"
#include "../Common/CompressEngineCommon.h"
#include "../Common/UpdateProducer.h"

#include "Compression/CopyCoder.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/FileDir.h"

#include "Interface/FileStreams.h"

#include "Handler.h"
#include "ArchiveUpdateCallback.h"

using namespace NWindows;
using namespace NCOM;

static HRESULT CopyBlock(ISequentialInStream *inStream, ISequentialOutStream *outStream)
{
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

STDMETHODIMP CAgent::SetFolder(IFolderFolder *folder)
{
  _archiveNamePrefix.Empty();
  if (folder == NULL)
  {
    _archiveFolderItem = NULL;
    return S_OK;
    // folder = m_RootFolder;
  }
  else
  {
    CComPtr<IFolderFolder> archiveFolder = folder;
    CComPtr<IArchiveFolderInternal> archiveFolderInternal;
    RINOK(archiveFolder.QueryInterface(&archiveFolderInternal));
    CAgentFolder *agentFolder;
    RINOK(archiveFolderInternal->GetAgentFolder(&agentFolder));
    _archiveFolderItem = agentFolder->_proxyFolderItem;
  }

  UStringVector pathParts;
  pathParts.Clear();
  CComPtr<IFolderFolder> folderItem = folder;
  if (_archiveFolderItem != NULL)
    while (true)
    {
      CComPtr<IFolderFolder> newFolder;
      folderItem->BindToParentFolder(&newFolder);  
      if (newFolder == NULL)
        break;
      CComBSTR name;
      folderItem->GetName(&name);
      pathParts.Insert(0, (const wchar_t *)name);
      folderItem = newFolder;
    }

  for(int i = 0; i < pathParts.Size(); i++)
  {
    _archiveNamePrefix += pathParts[i];
    _archiveNamePrefix += L'\\';
  }
  return S_OK;
}

STDMETHODIMP CAgent::SetFiles(const wchar_t *folderPrefix, 
    const wchar_t **names, UINT32 numNames)
{
  _folderPrefix = folderPrefix;
  _names.Clear();
  _names.Reserve(numNames);
  for (int i = 0; i < numNames; i++)
    _names.Add(names[i]);
  return S_OK;
}


static HRESULT GetFileTime(CAgent *agent, UINT32 itemIndex, FILETIME &fileTime)
{
  CPropVariant property;
  RINOK(agent->_archive->GetProperty(itemIndex, kpidLastWriteTime, &property));
  if (property.vt == VT_FILETIME)
    fileTime = property.filetime;
  else if (property.vt == VT_EMPTY)
    fileTime = agent->_defaultTime;
  else
    throw 4190407;
  return S_OK;
}

static HRESULT EnumerateArchiveItems(CAgent *agent,
    const CFolderItem &item, 
    const UString &prefix,
    CArchiveItemInfoVector &archiveItems)
{
  int i;
  for(i = 0; i < item.FileSubItems.Size(); i++)
  {
    const CFileItem &fileItem = item.FileSubItems[i];
    CArchiveItemInfo itemInfo;

    RINOK(::GetFileTime(agent, fileItem.Index, itemInfo.LastWriteTime));

    CPropVariant property;
    agent->_archive->GetProperty(fileItem.Index, kpidSize, &property);
    if (itemInfo.SizeIsDefined = (property.vt != VT_EMPTY))
      itemInfo.Size = ConvertPropVariantToUINT64(property);
    itemInfo.IsDirectory = false;
    itemInfo.Name = prefix + fileItem.Name;
    itemInfo.Censored = true; // test it
    itemInfo.IndexInServer = fileItem.Index;
    archiveItems.Add(itemInfo);
  }
  for(i = 0; i < item.FolderSubItems.Size(); i++)
  {
    const CFolderItem &dirItem = item.FolderSubItems[i];
    UString fullName = prefix + dirItem.Name;
    if(dirItem.IsLeaf)
    {
      CArchiveItemInfo itemInfo;
      RINOK(::GetFileTime(agent, dirItem.Index, itemInfo.LastWriteTime));
      itemInfo.IsDirectory = true;
      itemInfo.SizeIsDefined = false;
      itemInfo.Name = fullName;
      itemInfo.Censored = true; // test it
      itemInfo.IndexInServer = dirItem.Index;
      archiveItems.Add(itemInfo);
    }
    RINOK(EnumerateArchiveItems(agent, dirItem, fullName + UString(L'\\'), archiveItems));
  }
  return S_OK;
}

STDMETHODIMP CAgent::DoOperation(const CLSID *clsID, 
    const wchar_t *newArchiveName, 
    const BYTE *stateActions, 
    const wchar_t *sfxModule,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  UINT codePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  NUpdateArchive::CActionSet actionSet;
  for (int i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    actionSet.StateActions[i] = (NUpdateArchive::NPairAction::EEnum)stateActions[i];

  CSysStringVector systemFileNames;
  systemFileNames.Reserve(_names.Size());
  for (i = 0; i < _names.Size(); i++)
    systemFileNames.Add(GetSystemString(_names[i], codePage));
  CArchiveStyleDirItemInfoVector dirItems;

  CSysString folderPrefix = GetSystemString(_folderPrefix, codePage);
  NFile::NName::NormalizeDirPathPrefix(folderPrefix);
  ::EnumerateItems(folderPrefix, systemFileNames, _archiveNamePrefix, dirItems, codePage);

  CComPtr<IOutArchive> outArchive;
  if (_archive)
  {
    RINOK(_archive.QueryInterface(&outArchive));
  }
  else
  {
    RINOK(outArchive.CoCreateInstance(*clsID));
  }

  NFileTimeType::EEnum fileTimeType;
  UINT32 value;
  RINOK(outArchive->GetFileTimeType(&value));

  switch(value)
  {
    case NFileTimeType::kWindows:
    case NFileTimeType::kDOS:
    case NFileTimeType::kUnix:
      fileTimeType = NFileTimeType::EEnum(value);
      break;
    default:
      return E_FAIL;
  }

  CUpdatePairInfoVector updatePairs;

  CArchiveItemInfoVector archiveItems;
  if (_archive)
  {
    RINOK(ReadItems());
    EnumerateArchiveItems(this, _proxyHandler->FolderItemHead,  L"", archiveItems);
  }

  GetUpdatePairInfoList(dirItems, archiveItems, fileTimeType, updatePairs);
  
  CUpdatePairInfo2Vector operationChain;
  UpdateProduce(dirItems, archiveItems, updatePairs, actionSet,
      operationChain);
  
  CComObjectNoLock<CArchiveUpdateCallback> *updateCallbackSpec =
    new CComObjectNoLock<CArchiveUpdateCallback>;
  CComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  updateCallbackSpec->Init(folderPrefix,&dirItems, &archiveItems, 
      &operationChain, NULL, updateCallback100);
  
  CComObjectNoLock<COutFileStream> *outStreamSpec =
    new CComObjectNoLock<COutFileStream>;
  CComPtr<IOutStream> outStream(outStreamSpec);
  CSysString archiveName = GetSystemString(newArchiveName, codePage);
  {
    CSysString resultPath;
    int pos;
    if(!NFile::NDirectory::MyGetFullPathName(archiveName, resultPath, pos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(resultPath.Left(pos));
  }
  if (!outStreamSpec->Open(archiveName))
  {
    // ShowLastErrorMessage();
    return E_FAIL;
  }
  
  CComPtr<ISetProperties> setProperties;
  if (outArchive->QueryInterface(&setProperties) == S_OK)
  {
    if (m_PropNames.Size() == 0)
    {
      RINOK(setProperties->SetProperties(0, 0, 0));
    }
    else
    {
      std::vector<BSTR> names;
      for(i = 0; i < m_PropNames.Size(); i++)
        names.push_back(m_PropNames[i]);
      RINOK(setProperties->SetProperties(&names.front(), 
          &m_PropValues.front(), names.size()));
    }
  }
  m_PropNames.Clear();
  m_PropValues.clear();

  if (sfxModule != NULL)
  {
    CComObjectNoLock<CInFileStream> *sfxStreamSpec = new CComObjectNoLock<CInFileStream>;
    CComPtr<IInStream> sfxStream(sfxStreamSpec);
    if (!sfxStreamSpec->Open(GetSystemString(sfxModule, codePage)))
      throw "Can't open sfx module";
    RINOK(CopyBlock(sfxStream, outStream));
  }

  return outArchive->UpdateItems(outStream, operationChain.Size(),
      updateCallback);
}


HRESULT CAgent::CommonUpdate(const wchar_t *newArchiveName,
    int numUpdateItems,
    IArchiveUpdateCallback *updateCallback)
{
  UINT codePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  CComPtr<IOutArchive> outArchive;
  RINOK(_archive.QueryInterface(&outArchive));

  CComObjectNoLock<COutFileStream> *outStreamSpec =
    new CComObjectNoLock<COutFileStream>;
  CComPtr<IOutStream> outStream(outStreamSpec);
  CSysString archiveName = GetSystemString(newArchiveName, codePage);
  {
    CSysString resultPath;
    int pos;
    if(!NFile::NDirectory::MyGetFullPathName(archiveName, resultPath, pos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(resultPath.Left(pos));
  }
  if (!outStreamSpec->Open(archiveName))
  {
    // ShowLastErrorMessage();
    return E_FAIL;
  }
  
  return outArchive->UpdateItems(outStream, numUpdateItems, updateCallback);
}


STDMETHODIMP CAgent::DeleteItems(
    const wchar_t *newArchiveName, 
    const UINT32 *indices, UINT32 numItems, 
    IFolderArchiveUpdateCallback *updateCallback100)
{
  CComObjectNoLock<CArchiveUpdateCallback> *updateCallbackSpec =
    new CComObjectNoLock<CArchiveUpdateCallback>;
  CComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec);
  
  CUIntVector realIndices;
  _proxyHandler->GetRealIndices(*_archiveFolderItem, indices, numItems, 
      realIndices);
  CUpdatePairInfo2Vector updatePairs;
  int curIndex = 0;
  UINT32 numItemsInArchive;
  RINOK(_archive->GetNumberOfItems(&numItemsInArchive));
  for (int i = 0; i < numItemsInArchive; i++)
  {
    if (curIndex < realIndices.Size())
      if (realIndices[curIndex] == i)
      {
        curIndex++;
        continue;
      }
    CUpdatePairInfo2 updatePair;
    updatePair.NewData = updatePair.NewProperties = false;
    updatePair.ExistInArchive = true;
    updatePair.ExistOnDisk = false;
    updatePair.IsAnti = false;
    updatePair.ArchiveItemIndex = i;
    updatePairs.Add(updatePair);
  }
  updateCallbackSpec->Init(TEXT(""), NULL, NULL, &updatePairs, NULL, updateCallback100);
  return CommonUpdate(newArchiveName, updatePairs.Size(), updateCallback);
}

HRESULT CAgent::CreateFolder(
    const wchar_t *newArchiveName, 
    const wchar_t *folderName, 
    IFolderArchiveUpdateCallback *updateCallback100)
{
  CComObjectNoLock<CArchiveUpdateCallback> *updateCallbackSpec =
    new CComObjectNoLock<CArchiveUpdateCallback>;
  CComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec);

  CUpdatePairInfo2Vector updatePairs;
  UINT32 numItemsInArchive;
  RINOK(_archive->GetNumberOfItems(&numItemsInArchive));
  for (int i = 0; i < numItemsInArchive; i++)
  {
    CUpdatePairInfo2 updatePair;
    updatePair.NewData = updatePair.NewProperties = false;
    updatePair.ExistInArchive = true;
    updatePair.ExistOnDisk = false;
    updatePair.IsAnti = false;
    updatePair.ArchiveItemIndex = i;
    updatePairs.Add(updatePair);
  }
  CUpdatePairInfo2 updatePair;
  updatePair.NewData = updatePair.NewProperties = true;
  updatePair.ExistInArchive = false;
  updatePair.ExistOnDisk = true;
  updatePair.IsAnti = false;
  updatePair.ArchiveItemIndex = -1;
  updatePair.DirItemIndex = 0;

  updatePairs.Add(updatePair);

  CArchiveStyleDirItemInfoVector dirItems;
  CArchiveStyleDirItemInfo dirItem;

  dirItem.Attributes = FILE_ATTRIBUTE_DIRECTORY;
  dirItem.Size = 0;
  dirItem.Name = _archiveFolderItem->GetFullPathPrefix() + folderName;

  SYSTEMTIME systemTime;
  FILETIME fileTime;
  ::GetSystemTime(&systemTime);
  ::SystemTimeToFileTime(&systemTime, &fileTime);
  dirItem.LastAccessTime = dirItem.LastWriteTime = 
      dirItem.CreationTime = fileTime;

  dirItems.Add(dirItem);

  updateCallbackSpec->Init(TEXT(""), &dirItems, NULL, &updatePairs, NULL, updateCallback100);
  return CommonUpdate(newArchiveName, updatePairs.Size(), updateCallback);
}


HRESULT CAgent::RenameItem(
    const wchar_t *newArchiveName, 
    const UINT32 *indices, UINT32 numItems, 
    const wchar_t *newItemName, 
    IFolderArchiveUpdateCallback *updateCallback100)
{
  if (numItems != 1)
    return E_INVALIDARG;
  CComObjectNoLock<CArchiveUpdateCallback> *updateCallbackSpec =
    new CComObjectNoLock<CArchiveUpdateCallback>;
  CComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec);
  
  CUIntVector realIndices;
  _proxyHandler->GetRealIndices(*_archiveFolderItem, indices, numItems, 
      realIndices);

  UString fullPrefix = _archiveFolderItem->GetFullPathPrefix();
  UString oldItemPath = fullPrefix + 
      _archiveFolderItem->GetItemName(indices[0]);
  UString newItemPath = fullPrefix + newItemName;

  CUpdatePairInfo2Vector updatePairs;
  int curIndex = 0;
  UINT32 numItemsInArchive;
  RINOK(_archive->GetNumberOfItems(&numItemsInArchive));
  for (int i = 0; i < numItemsInArchive; i++)
  {
    if (curIndex < realIndices.Size())
      if (realIndices[curIndex] == i)
      {
        CUpdatePairInfo2 updatePair;
        updatePair.NewData = false;
        updatePair.NewProperties = true;
        updatePair.ExistInArchive = true;
        updatePair.ExistOnDisk = false;
        updatePair.IsAnti = false; // ?
        updatePair.ArchiveItemIndex = i;
        updatePair.NewNameIsDefined = true;

        updatePair.NewName = newItemName;

        NCOM::CPropVariant propVariant;
        RETURN_IF_NOT_S_OK(_archive->GetProperty(
            updatePair.ArchiveItemIndex, kpidPath, &propVariant));
        if (propVariant.vt != VT_BSTR)
          return E_INVALIDARG;

        UString oldFullPath = propVariant.bstrVal;
        if (oldItemPath.CollateNoCase(oldFullPath.Left(oldItemPath.Length())) != 0)
          return E_INVALIDARG;

        updatePair.NewName = newItemPath + oldFullPath.Mid(oldItemPath.Length());

        updatePairs.Add(updatePair);
        curIndex++;
        continue;
      }
    CUpdatePairInfo2 updatePair;
    updatePair.NewData = updatePair.NewProperties = false;
    updatePair.ExistInArchive = true;
    updatePair.ExistOnDisk = false;
    updatePair.IsAnti = false;
    updatePair.ArchiveItemIndex = i;
    updatePairs.Add(updatePair);
  }
  updateCallbackSpec->Init(TEXT(""), NULL, NULL, &updatePairs, _archive, updateCallback100);
  return CommonUpdate(newArchiveName, updatePairs.Size(), updateCallback);
}



STDMETHODIMP CAgent::SetProperties(const BSTR *names, 
    const PROPVARIANT *values, INT32 numProperties)
{
  m_PropNames.Clear();
  m_PropValues.clear();
  for (int i = 0; i < numProperties; i++)
  {
    m_PropNames.Add(names[i]);
    m_PropValues.push_back(values[i]);
  }
  return S_OK;
}


