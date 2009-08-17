// AgentOut.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/Time.h"

#include "../../Compress/CopyCoder.h"

#include "../../Common/FileStreams.h"

#include "../Common/EnumDirItems.h"
#include "../Common/OpenArchive.h"
#include "../Common/UpdateCallback.h"
#include "../Common/UpdatePair.h"

#include "Agent.h"
#include "UpdateCallbackAgent.h"

using namespace NWindows;
using namespace NCOM;

STDMETHODIMP CAgent::SetFolder(IFolderFolder *folder)
{
  _archiveNamePrefix.Empty();
  if (folder == NULL)
  {
    _agentFolder = NULL;
    return S_OK;
  }
  else
  {
    CMyComPtr<IFolderFolder> archiveFolder = folder;
    CMyComPtr<IArchiveFolderInternal> archiveFolderInternal;
    RINOK(archiveFolder.QueryInterface(IID_IArchiveFolderInternal, &archiveFolderInternal));
    RINOK(archiveFolderInternal->GetAgentFolder(&_agentFolder));
  }

  UStringVector pathParts;
  pathParts.Clear();
  CMyComPtr<IFolderFolder> folderItem = folder;
  if (folderItem != NULL)
    for (;;)
    {
      CMyComPtr<IFolderFolder> newFolder;
      folderItem->BindToParentFolder(&newFolder);
      if (newFolder == NULL)
        break;

      NCOM::CPropVariant prop;
      if (folderItem->GetFolderProperty(kpidName, &prop) == S_OK)
        if (prop.vt == VT_BSTR)
          pathParts.Insert(0, (const wchar_t *)prop.bstrVal);
      folderItem = newFolder;
    }

  for (int i = 0; i < pathParts.Size(); i++)
  {
    _archiveNamePrefix += pathParts[i];
    _archiveNamePrefix += WCHAR_PATH_SEPARATOR;
  }
  return S_OK;
}

STDMETHODIMP CAgent::SetFiles(const wchar_t *folderPrefix,
    const wchar_t **names, UInt32 numNames)
{
  _folderPrefix = folderPrefix;
  _names.Clear();
  _names.Reserve(numNames);
  for (UInt32 i = 0; i < numNames; i++)
    _names.Add(names[i]);
  return S_OK;
}

static HRESULT EnumerateArchiveItems(CAgent *agent,
    const CProxyFolder &item,
    const UString &prefix,
    CObjectVector<CArcItem> &arcItems)
{
  int i;
  for (i = 0; i < item.Files.Size(); i++)
  {
    const CProxyFile &fileItem = item.Files[i];
    CArcItem ai;
    RINOK(agent->GetArc().GetItemMTime(fileItem.Index, ai.MTime, ai.MTimeDefined));

    CPropVariant property;
    agent->GetArchive()->GetProperty(fileItem.Index, kpidSize, &property);
    ai.SizeDefined = (property.vt != VT_EMPTY);
    if (ai.SizeDefined)
      ai.Size = ConvertPropVariantToUInt64(property);
    ai.IsDir = false;
    ai.Name = prefix + fileItem.Name;
    ai.Censored = true; // test it
    ai.IndexInServer = fileItem.Index;
    arcItems.Add(ai);
  }
  for (i = 0; i < item.Folders.Size(); i++)
  {
    const CProxyFolder &dirItem = item.Folders[i];
    UString fullName = prefix + dirItem.Name;
    if (dirItem.IsLeaf)
    {
      CArcItem ai;
      RINOK(agent->GetArc().GetItemMTime(dirItem.Index, ai.MTime, ai.MTimeDefined));
      ai.IsDir = true;
      ai.SizeDefined = false;
      ai.Name = fullName;
      ai.Censored = true; // test it
      ai.IndexInServer = dirItem.Index;
      arcItems.Add(ai);
    }
    RINOK(EnumerateArchiveItems(agent, dirItem, fullName + UString(WCHAR_PATH_SEPARATOR), arcItems));
  }
  return S_OK;
}

struct CAgUpCallbackImp: public IUpdateProduceCallback
{
  const CObjectVector<CArcItem> *_arcItems;
  IFolderArchiveUpdateCallback *_callback;
  
  CAgUpCallbackImp(const CObjectVector<CArcItem> *a,
      IFolderArchiveUpdateCallback *callback): _arcItems(a), _callback(callback) {}
  HRESULT ShowDeleteFile(int arcIndex);
};

HRESULT CAgUpCallbackImp::ShowDeleteFile(int arcIndex)
{
  return _callback->DeleteOperation((*_arcItems)[arcIndex].Name);
}

STDMETHODIMP CAgent::DoOperation(
    CCodecs *codecs,
    int formatIndex,
    const wchar_t *newArchiveName,
    const Byte *stateActions,
    const wchar_t *sfxModule,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  if (!CanUpdate())
    return E_NOTIMPL;
  NUpdateArchive::CActionSet actionSet;
  int i;
  for (i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    actionSet.StateActions[i] = (NUpdateArchive::NPairAction::EEnum)stateActions[i];

  CDirItems dirItems;

  {
    UString folderPrefix = _folderPrefix;
    NFile::NName::NormalizeDirPathPrefix(folderPrefix);
    UStringVector errorPaths;
    CRecordVector<DWORD> errorCodes;
    dirItems.EnumerateDirItems2(folderPrefix, _archiveNamePrefix, _names, errorPaths, errorCodes);
    if (errorCodes.Size() > 0)
      return errorCodes.Front();
  }

  CMyComPtr<IOutArchive> outArchive;
  if (GetArchive())
  {
    RINOK(GetArchive()->QueryInterface(IID_IOutArchive, (void **)&outArchive));
  }
  else
  {
    if (formatIndex < 0)
      return E_FAIL;
    RINOK(codecs->CreateOutArchive(formatIndex, outArchive));
    #ifdef EXTERNAL_CODECS
    {
      CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
      outArchive.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
      if (setCompressCodecsInfo)
      {
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(codecs));
      }
    }
    #endif

  }

  NFileTimeType::EEnum fileTimeType;
  UInt32 value;
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


  CObjectVector<CArcItem> arcItems;
  if (GetArchive())
  {
    RINOK(ReadItems());
    EnumerateArchiveItems(this, _proxyArchive->RootFolder, L"", arcItems);
  }

  CRecordVector<CUpdatePair2> updatePairs2;

  {
    CRecordVector<CUpdatePair> updatePairs;
    GetUpdatePairInfoList(dirItems, arcItems, fileTimeType, updatePairs);
    CAgUpCallbackImp upCallback(&arcItems, updateCallback100);
    UpdateProduce(updatePairs, actionSet, updatePairs2, &upCallback);
  }

  UInt32 numFiles = 0;
  for (i = 0; i < updatePairs2.Size(); i++)
    if (updatePairs2[i].NewData)
      numFiles++;
  
  if (updateCallback100)
  {
    RINOK(updateCallback100->SetNumFiles(numFiles));
  }
  
  CUpdateCallbackAgent updateCallbackAgent;
  updateCallbackAgent.SetCallback(updateCallback100);
  CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
  CMyComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec );

  updateCallbackSpec->DirItems = &dirItems;
  updateCallbackSpec->ArcItems = &arcItems;
  updateCallbackSpec->UpdatePairs = &updatePairs2;
  updateCallbackSpec->Archive = GetArchive();
  updateCallbackSpec->Callback = &updateCallbackAgent;

  COutFileStream *outStreamSpec = new COutFileStream;
  CMyComPtr<IOutStream> outStream(outStreamSpec);
  UString archiveName = newArchiveName;
  {
    UString resultPath;
    int pos;
    if (!NFile::NDirectory::MyGetFullPathName(archiveName, resultPath, pos))
      return E_FAIL;
    NFile::NDirectory::CreateComplexDirectory(resultPath.Left(pos));
  }
  if (!outStreamSpec->Create(archiveName, true))
  {
    // ShowLastErrorMessage();
    return E_FAIL;
  }
  
  CMyComPtr<ISetProperties> setProperties;
  if (outArchive->QueryInterface(IID_ISetProperties, (void **)&setProperties) == S_OK)
  {
    if (m_PropNames.Size() == 0)
    {
      RINOK(setProperties->SetProperties(0, 0, 0));
    }
    else
    {
      CRecordVector<const wchar_t *> names;
      for(i = 0; i < m_PropNames.Size(); i++)
        names.Add((const wchar_t *)m_PropNames[i]);

      NWindows::NCOM::CPropVariant *propValues = new NWindows::NCOM::CPropVariant[m_PropValues.Size()];
      try
      {
        for (int i = 0; i < m_PropValues.Size(); i++)
          propValues[i] = m_PropValues[i];
        RINOK(setProperties->SetProperties(&names.Front(), propValues, names.Size()));
      }
      catch(...)
      {
        delete []propValues;
        return E_FAIL;
      }
      delete []propValues;
    }
  }
  m_PropNames.Clear();
  m_PropValues.Clear();

  if (sfxModule != NULL)
  {
    CInFileStream *sfxStreamSpec = new CInFileStream;
    CMyComPtr<IInStream> sfxStream(sfxStreamSpec);
    if (!sfxStreamSpec->Open(sfxModule))
      return E_FAIL;
      // throw "Can't open sfx module";
    RINOK(NCompress::CopyStream(sfxStream, outStream, NULL));
  }

  RINOK(outArchive->UpdateItems(outStream, updatePairs2.Size(),updateCallback));
  return outStreamSpec->Close();
}

STDMETHODIMP CAgent::DoOperation2(
    const wchar_t *newArchiveName,
    const Byte *stateActions,
    const wchar_t *sfxModule,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  return DoOperation(_codecs, -1, newArchiveName,
    stateActions, sfxModule, updateCallback100);
}

HRESULT CAgent::CommonUpdate(
    const wchar_t *newArchiveName,
    int numUpdateItems,
    IArchiveUpdateCallback *updateCallback)
{
  if (!CanUpdate())
    return E_NOTIMPL;
  CMyComPtr<IOutArchive> outArchive;
  RINOK(GetArchive()->QueryInterface(IID_IOutArchive, (void **)&outArchive));

  COutFileStream *outStreamSpec = new COutFileStream;
  CMyComPtr<IOutStream> outStream(outStreamSpec);

  UString archiveName = newArchiveName;
  {
    UString resultPath;
    int pos;
    if (!NFile::NDirectory::MyGetFullPathName(archiveName, resultPath, pos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(resultPath.Left(pos));
  }

  /*
  bool isOK = false;
  for (int i = 0; i < (1 << 16); i++)
  {
    resultName = newArchiveName;
    if (i > 0)
    {
      wchar_t s[16];
      ConvertUInt32ToString(i, s);
      resultName += s;
    }
    if (outStreamSpec->Open(realPath))
    {
      isOK = true;
      break;
    }
    if (::GetLastError() != ERROR_FILE_EXISTS)
      return ::GetLastError();
  }
  if (!isOK)
    return ::GetLastError();
  */
  if (!outStreamSpec->Create(archiveName, true))
  {
    // ShowLastErrorMessage();
    return E_FAIL;
  }
  
  RINOK(outArchive->UpdateItems(outStream, numUpdateItems, updateCallback));
  return outStreamSpec->Close();
}


STDMETHODIMP CAgent::DeleteItems(
    const wchar_t *newArchiveName,
    const UInt32 *indices, UInt32 numItems,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  if (!CanUpdate())
    return E_NOTIMPL;
  CUpdateCallbackAgent updateCallbackAgent;
  updateCallbackAgent.SetCallback(updateCallback100);
  CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
  CMyComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec);
  
  CUIntVector realIndices;
  _agentFolder->GetRealIndices(indices, numItems, realIndices);
  CRecordVector<CUpdatePair2> updatePairs;
  int curIndex = 0;
  UInt32 numItemsInArchive;
  RINOK(GetArchive()->GetNumberOfItems(&numItemsInArchive));
  for (UInt32 i = 0; i < numItemsInArchive; i++)
  {
    if (curIndex < realIndices.Size())
      if (realIndices[curIndex] == i)
      {
        curIndex++;
        continue;
      }
    CUpdatePair2 up2;
    up2.NewData = up2.NewProps = false;
    up2.IsAnti = false; // check it. Maybe it can be undefined
    up2.ArcIndex = i;
    updatePairs.Add(up2);
  }
  updateCallbackSpec->UpdatePairs = &updatePairs;
  updateCallbackSpec->Archive = GetArchive();
  updateCallbackSpec->Callback = &updateCallbackAgent;
  return CommonUpdate(newArchiveName, updatePairs.Size(), updateCallback);
}

HRESULT CAgent::CreateFolder(
    const wchar_t *newArchiveName,
    const wchar_t *folderName,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  if (!CanUpdate())
    return E_NOTIMPL;
  CUpdateCallbackAgent updateCallbackAgent;
  updateCallbackAgent.SetCallback(updateCallback100);
  CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
  CMyComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec);

  CRecordVector<CUpdatePair2> updatePairs;
  UInt32 numItemsInArchive;
  RINOK(GetArchive()->GetNumberOfItems(&numItemsInArchive));
  for (UInt32 i = 0; i < numItemsInArchive; i++)
  {
    CUpdatePair2 up2;
    up2.NewData = up2.NewProps = false;
    up2.IsAnti = false;  // check it.
    up2.ArcIndex = i;
    updatePairs.Add(up2);
  }
  CUpdatePair2 up2;
  up2.NewData = up2.NewProps = true;
  up2.IsAnti = false;
  up2.DirIndex = 0;

  updatePairs.Add(up2);

  updatePairs.ReserveDown();

  CDirItems dirItems;
  CDirItem di;

  di.Attrib = FILE_ATTRIBUTE_DIRECTORY;
  di.Size = 0;
  di.Name = _agentFolder->_proxyFolderItem->GetFullPathPrefix() + folderName;

  FILETIME ft;
  NTime::GetCurUtcFileTime(ft);
  di.CTime = di.ATime = di.MTime = ft;

  dirItems.Items.Add(di);

  updateCallbackSpec->Callback = &updateCallbackAgent;
  updateCallbackSpec->DirItems = &dirItems;
  updateCallbackSpec->UpdatePairs = &updatePairs;
  updateCallbackSpec->Archive = GetArchive();
  return CommonUpdate(newArchiveName, updatePairs.Size(), updateCallback);
}


HRESULT CAgent::RenameItem(
    const wchar_t *newArchiveName,
    const UInt32 *indices, UInt32 numItems,
    const wchar_t *newItemName,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  if (!CanUpdate())
    return E_NOTIMPL;
  if (numItems != 1)
    return E_INVALIDARG;
  CUpdateCallbackAgent updateCallbackAgent;
  updateCallbackAgent.SetCallback(updateCallback100);
  CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
  CMyComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec);
  
  CUIntVector realIndices;
  _agentFolder->GetRealIndices(indices, numItems, realIndices);

  UString fullPrefix = _agentFolder->GetFullPathPrefixPlusPrefix(indices[0]);
  UString oldItemPath = fullPrefix + _agentFolder->GetName(indices[0]);
  UString newItemPath = fullPrefix + newItemName;

  CRecordVector<CUpdatePair2> updatePairs;
  UStringVector newNames;

  int curIndex = 0;
  UInt32 numItemsInArchive;
  RINOK(GetArchive()->GetNumberOfItems(&numItemsInArchive));
  for (UInt32 i = 0; i < numItemsInArchive; i++)
  {
    if (curIndex < realIndices.Size())
      if (realIndices[curIndex] == i)
      {
        CUpdatePair2 up2;
        up2.NewData = false;
        up2.NewProps = true;
        RINOK(GetArc().IsItemAnti(i, up2.IsAnti));
        up2.ArcIndex = i;

        UString oldFullPath;
        RINOK(GetArc().GetItemPath(i, oldFullPath));

        if (oldItemPath.CompareNoCase(oldFullPath.Left(oldItemPath.Length())) != 0)
          return E_INVALIDARG;

        up2.NewNameIndex = newNames.Add(newItemPath + oldFullPath.Mid(oldItemPath.Length()));
        updatePairs.Add(up2);
        curIndex++;
        continue;
      }
    CUpdatePair2 up2;
    up2.NewData = up2.NewProps = false;
    up2.IsAnti = false;
    up2.ArcIndex = i;
    updatePairs.Add(up2);
  }
  updateCallbackSpec->Callback = &updateCallbackAgent;
  updateCallbackSpec->UpdatePairs = &updatePairs;
  updateCallbackSpec->NewNames = &newNames;
  updateCallbackSpec->Archive = GetArchive();
  return CommonUpdate(newArchiveName, updatePairs.Size(), updateCallback);
}

STDMETHODIMP CAgent::SetProperties(const wchar_t **names,
    const PROPVARIANT *values, Int32 numProperties)
{
  m_PropNames.Clear();
  m_PropValues.Clear();
  for (int i = 0; i < numProperties; i++)
  {
    m_PropNames.Add(names[i]);
    m_PropValues.Add(values[i]);
  }
  return S_OK;
}
