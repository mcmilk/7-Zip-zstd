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
#include "UpdateCallback.h"

using namespace NWindows;
using namespace NCOM;

static HRESULT CopyBlock(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream)
{
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> aCopyCoder = aCopyCoderSpec;
  return aCopyCoder->Code(anInStream, anOutStream, NULL, NULL, NULL);
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
    CComPtr<IArchiveFolderInternal> anArchiveFolderInternal;
    RETURN_IF_NOT_S_OK(archiveFolder.QueryInterface(&anArchiveFolderInternal));
    CAgentFolder *agentFolder;
    RETURN_IF_NOT_S_OK(anArchiveFolderInternal->GetAgentFolder(&agentFolder));
    _archiveFolderItem = agentFolder->_proxyFolderItem;
  }

  UStringVector aPathParts;
  aPathParts.Clear();
  CComPtr<IFolderFolder> folderItem = folder;
  if (_archiveFolderItem != NULL)
    while (true)
    {
      CComPtr<IFolderFolder> newFolder;
      folderItem->BindToParentFolder(&newFolder);  
      if (newFolder == NULL)
        break;
      CComBSTR aName;
      folderItem->GetName(&aName);
      aPathParts.Insert(0, (const wchar_t *)aName);
      folderItem = newFolder;
    }

  for(int i = 0; i < aPathParts.Size(); i++)
  {
    _archiveNamePrefix += aPathParts[i];
    _archiveNamePrefix += L'\\';
  }
  return S_OK;
}

STDMETHODIMP CAgent::SetFiles(const wchar_t *aFolderPrefix, const wchar_t **aNames, UINT32 aNumNames)
{
  _folderPrefix = aFolderPrefix;
  _names.Clear();
  _names.Reserve(aNumNames);
  for (int i = 0; i < aNumNames; i++)
    _names.Add(aNames[i]);

  return S_OK;
}


static void GetFileTime(CAgent *agent, UINT32 itemIndex, FILETIME &fileTime)
{
  CPropVariant aProperty;
  agent->_archive->GetProperty(itemIndex, kpidLastWriteTime, &aProperty);
  if (aProperty.vt == VT_FILETIME)
    fileTime = aProperty.filetime;
  else if (aProperty.vt == VT_EMPTY)
    fileTime = agent->_defaultTime;
  else
    throw 4190407;
}

static void EnumerateInArchiveItems(CAgent *agent,
    const CFolderItem &anItem, 
    const UString &aPrefix,
    CArchiveItemInfoVector &anArchiveItems)
{
  for(int i = 0; i < anItem.m_FileSubItems.Size(); i++)
  {
    const CFileItem &aFileItem = anItem.m_FileSubItems[i];
    CArchiveItemInfo anItemInfo;

    GetFileTime(agent, aFileItem.m_Index, anItemInfo.LastWriteTime);

    CPropVariant aProperty;
    agent->_archive->GetProperty(aFileItem.m_Index, kpidSize, &aProperty);
    if (anItemInfo.SizeIsDefined = (aProperty.vt != VT_EMPTY))
      anItemInfo.Size = ConvertPropVariantToUINT64(aProperty);
    anItemInfo.IsDirectory = false;
    anItemInfo.Name = aPrefix + aFileItem.m_Name;
    anItemInfo.Censored = true; // test it
    anItemInfo.IndexInServer = aFileItem.m_Index;
    anArchiveItems.Add(anItemInfo);
  }
  for(i = 0; i < anItem.m_FolderSubItems.Size(); i++)
  {
    const CFolderItem &aDirItem = anItem.m_FolderSubItems[i];
    UString fullName = aPrefix + aDirItem.m_Name;
    if(aDirItem.m_IsLeaf)
    {
      CArchiveItemInfo anItemInfo;
      GetFileTime(agent, aDirItem.m_Index, anItemInfo.LastWriteTime);
      anItemInfo.IsDirectory = true;
      anItemInfo.SizeIsDefined = false;
      anItemInfo.Name = fullName;
      anItemInfo.Censored = true; // test it
      anItemInfo.IndexInServer = aDirItem.m_Index;
      anArchiveItems.Add(anItemInfo);
    }
    EnumerateInArchiveItems(agent, aDirItem, fullName + wchar_t(L'\\'), anArchiveItems);
  }
}

STDMETHODIMP CAgent::DoOperation(const CLSID *aCLSID, 
    const wchar_t *aNewArchiveName, 
    const BYTE *aStateActions, 
    const wchar_t *aSfxModule,
    IUpdateCallback100 *anUpdateCallback100)
{
  UINT aCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  NUpdateArchive::CActionSet anActionSet;
  for (int i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    anActionSet.StateActions[i] = (NUpdateArchive::NPairAction::EEnum)aStateActions[i];

  CSysStringVector aSystemFileNames;
  aSystemFileNames.Reserve(_names.Size());
  for (i = 0; i < _names.Size(); i++)
    aSystemFileNames.Add(GetSystemString(_names[i], aCodePage));
  CArchiveStyleDirItemInfoVector aDirItems;

  CSysString aFolderPrefix = GetSystemString(_folderPrefix, aCodePage);
  NFile::NName::NormalizeDirPathPrefix(aFolderPrefix);
  EnumerateItems(aFolderPrefix, aSystemFileNames, _archiveNamePrefix, aDirItems, aCodePage);

  CComPtr<IOutArchiveHandler200> anOutArchive;
  if (_archive)
  {
    RETURN_IF_NOT_S_OK(_archive.QueryInterface(&anOutArchive));
  }
  else
  {
    RETURN_IF_NOT_S_OK(anOutArchive.CoCreateInstance(*aCLSID));
  }

  NFileTimeType::EEnum aFileTimeType;
  UINT32 aValue;
  RETURN_IF_NOT_S_OK(anOutArchive->GetFileTimeType(&aValue));

  switch(aValue)
  {
    case NFileTimeType::kWindows:
    case NFileTimeType::kDOS:
    case NFileTimeType::kUnix:
      aFileTimeType = NFileTimeType::EEnum(aValue);
      break;
    default:
      return E_FAIL;
  }

  CUpdatePairInfoVector anUpdatePairs;

  CArchiveItemInfoVector anArchiveItems;
  if (_archive)
  {
    RETURN_IF_NOT_S_OK(ReadItems());
    EnumerateInArchiveItems(this, _proxyHandler->_folderItemHead,  L"", anArchiveItems);
  }

  GetUpdatePairInfoList(aDirItems, anArchiveItems, aFileTimeType, anUpdatePairs);
  
  CUpdatePairInfo2Vector anOperationChain;
  UpdateProduce(aDirItems, anArchiveItems, anUpdatePairs, anActionSet,
      anOperationChain);
  
  CComObjectNoLock<CUpdateCallBackImp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBackImp>;
  CComPtr<IUpdateCallBack> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(aFolderPrefix,&aDirItems, &anArchiveItems, 
      &anOperationChain, aCodePage, anUpdateCallback100);
  
  CComObjectNoLock<COutFileStream> *anOutStreamSpec =
    new CComObjectNoLock<COutFileStream>;
  CComPtr<IOutStream> anOutStream(anOutStreamSpec);
  CSysString anArchiveName = GetSystemString(aNewArchiveName, aCodePage);
  {
    CSysString aResultPath;
    int aPos;
    if(!NFile::NDirectory::MyGetFullPathName(anArchiveName, aResultPath, aPos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(aResultPath.Left(aPos));
  }
  if (!anOutStreamSpec->Open(anArchiveName))
  {
    // ShowLastErrorMessage();
    return E_FAIL;
  }
  
  CComPtr<ISetProperties> aSetProperties;
  if (anOutArchive->QueryInterface(&aSetProperties) == S_OK)
  {
    if (m_PropNames.Size() == 0)
    {
      RETURN_IF_NOT_S_OK(aSetProperties->SetProperties(0, 0, 0));
    }
    else
    {
      std::vector<BSTR> aNames;
      for(i = 0; i < m_PropNames.Size(); i++)
        aNames.push_back(m_PropNames[i]);
      RETURN_IF_NOT_S_OK(aSetProperties->SetProperties(&aNames.front(), 
          &m_PropValues.front(), aNames.size()));
    }
  }
  m_PropNames.Clear();
  m_PropValues.clear();

  if (aSfxModule != NULL)
  {
    CComObjectNoLock<CInFileStream> *aSFXStreamSpec = new CComObjectNoLock<CInFileStream>;
    CComPtr<IInStream> aSFXStream(aSFXStreamSpec);
    if (!aSFXStreamSpec->Open(GetSystemString(aSfxModule, aCodePage)))
      throw "Can't open sfx module";
    RETURN_IF_NOT_S_OK(CopyBlock(aSFXStream, anOutStream));
  }

  return anOutArchive->UpdateItems(anOutStream, anOperationChain.Size(),
      anUpdateCallBack);
}

STDMETHODIMP CAgent::DeleteItems(
    const wchar_t *aNewArchiveName, 
    const UINT32 *anIndexes, UINT32 aNumItems, 
    IUpdateCallback100 *anUpdateCallback100)
{
  UINT aCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  CComPtr<IOutArchiveHandler200> anOutArchive;
  RETURN_IF_NOT_S_OK(_archive.QueryInterface(&anOutArchive));

  CComObjectNoLock<CUpdateCallBackImp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBackImp>;
  CComPtr<IUpdateCallBack> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(TEXT(""), NULL, NULL, NULL, aCodePage, anUpdateCallback100);
  
  CComObjectNoLock<COutFileStream> *anOutStreamSpec =
    new CComObjectNoLock<COutFileStream>;
  CComPtr<IOutStream> anOutStream(anOutStreamSpec);
  CSysString anArchiveName = GetSystemString(aNewArchiveName, aCodePage);
  {
    CSysString aResultPath;
    int aPos;
    if(!NFile::NDirectory::MyGetFullPathName(anArchiveName, aResultPath, aPos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(aResultPath.Left(aPos));
  }
  if (!anOutStreamSpec->Open(anArchiveName))
  {
    // ShowLastErrorMessage();
    return E_FAIL;
  }
  
  std::vector<UINT32> aRealIndexes;
  _proxyHandler->GetRealIndexes(*_archiveFolderItem, anIndexes, aNumItems, 
      aRealIndexes);
  RETURN_IF_NOT_S_OK(anOutArchive->DeleteItems(anOutStream, 
      &aRealIndexes.front(), aRealIndexes.size(), anUpdateCallBack));
  return S_OK;
}

STDMETHODIMP CAgent::SetProperties(const BSTR *aNames, 
    const PROPVARIANT *aValues, INT32 aNumProperties)
{
  m_PropNames.Clear();
  m_PropValues.clear();
  for (int i = 0; i < aNumProperties; i++)
  {
    m_PropNames.Add(aNames[i]);
    m_PropValues.push_back(aValues[i]);
  }
  return S_OK;
}


