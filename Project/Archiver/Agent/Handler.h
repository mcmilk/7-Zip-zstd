// Agent/Handler.h

#pragma once

#ifndef __AGENT_HANDLER_H
#define __AGENT_HANDLER_H


#include "../Common/IArchiveHandler2.h"
#include "../Common/UpdatePairBasic.h"
// #include "../../Compress/Interface/CompressInterface.h"

#include "AgentProxyHandler.h"
#include "../Common/ZipRegistryMain.h"

#include "Windows/PropVariant.h"

#ifdef NEW_FOLDER_INTERFACE
#include "../../FileManager/FolderInterface.h"
#endif

class CAgentFolder;

// {23170F69-40C1-278A-0000-000100050001}
DEFINE_GUID(IID_IArchiveFolderInternal, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100050001")
IArchiveFolderInternal: public IUnknown
{
public:
  STDMETHOD(GetAgentFolder)(CAgentFolder **anAgentFolder) PURE;  
};

class CAgent;

class CAgentFolder: 
  public IFolderFolder,
  public IArchiveFolder,
  public IArchiveFolderInternal,
#ifdef NEW_FOLDER_INTERFACE
  public IEnumProperties,
  public IFolderGetTypeID,
  public IFolderGetPath,
  public IFolderOperations,
#endif
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CAgentFolder)
  COM_INTERFACE_ENTRY(IFolderFolder)
  COM_INTERFACE_ENTRY(IArchiveFolder)
  COM_INTERFACE_ENTRY(IArchiveFolderInternal)
#ifdef NEW_FOLDER_INTERFACE
  COM_INTERFACE_ENTRY(IEnumProperties)
  COM_INTERFACE_ENTRY(IFolderGetTypeID)
  COM_INTERFACE_ENTRY(IFolderGetPath)
  COM_INTERFACE_ENTRY(IFolderOperations)
#endif
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CAgentFolder)

DECLARE_NO_REGISTRY()

  STDMETHOD(LoadItems)();
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 itemIndex, PROPID propID, PROPVARIANT *value);
  STDMETHOD(BindToFolder)(UINT32 index, IFolderFolder **resultFolder);
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder);
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder);
  STDMETHOD(GetName)(BSTR *name);

  STDMETHOD(Extract)(const UINT32 *indices, UINT32 numItems, 
      NExtractionMode::NPath::EEnum aPathMode, 
      NExtractionMode::NOverwrite::EEnum anOverwriteMode, 
      const wchar_t *aPath,
      INT32 aTestMode,
      IExtractCallback2 *anExtractCallBack2);
  
  STDMETHOD(GetAgentFolder)(CAgentFolder **anAgentFolder);

#ifdef NEW_FOLDER_INTERFACE
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetTypeID)(BSTR *name);
  STDMETHOD(GetPath)(BSTR *path);


  // IFolderOperations

  STDMETHOD(CreateFolder)(const wchar_t *name, IProgress *progress);
  STDMETHOD(CreateFile)(const wchar_t *name, IProgress *progress);
  STDMETHOD(Rename)(UINT32 index, const wchar_t *newName, IProgress *progress);
  STDMETHOD(Delete)(const UINT32 *indices, UINT32 numItems, IProgress *progress);
  STDMETHOD(CopyTo)(const UINT32 *indices, UINT32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(MoveTo)(const UINT32 *indices, UINT32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(CopyFrom)(const wchar_t *fromFolderPath,
      const wchar_t **itemsPaths, UINT32 numItems, IProgress *progress);

#endif

  CAgentFolder(): _proxyFolderItem(NULL) {}

  void Init(CAgentProxyHandler *aProxyHandler,
      CFolderItem *aProxyFolderItem,
      IFolderFolder *aParentFolder,
      CAgent *agent)
  {
    _proxyHandler = aProxyHandler;
    _proxyFolderItem = aProxyFolderItem;
    _parentFolder = aParentFolder;
    _agent = (IArchiveHandler100 *)agent;
    _agentSpec = agent;
  }

  void GetPathParts(UStringVector &pathParts);
  HRESULT CommonUpdateOperation(
      bool deleteOperation,
      const NUpdateArchive::CActionSet *actionSet,
      const UINT32 *indices, UINT32 numItems,
      IUpdateCallback100 *updateCallback100);


public:
  CAgentProxyHandler *_proxyHandler;
  CFolderItem *_proxyFolderItem;
  CComPtr<IFolderFolder> _parentFolder;
  CComPtr<IArchiveHandler100> _agent;
  CAgent *_agentSpec;
};

// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
class CAgent: 
  public IArchiveHandler100,
#ifdef NEW_FOLDER_INTERFACE
  public IFolderManager,
  public IFolderManagerGetIconPath,
#endif
  #ifndef EXTRACT_ONLY
  public IOutArchiveHandler100,
  public ISetProperties,
  #endif
  public CComObjectRoot,
  public CComCoClass<CAgent,&CLSID_CAgentArchiveHandler>
{
public:
BEGIN_COM_MAP(CAgent)
  COM_INTERFACE_ENTRY(IArchiveHandler100)
  #ifndef EXTRACT_ONLY
  COM_INTERFACE_ENTRY(IOutArchiveHandler100)
  COM_INTERFACE_ENTRY(ISetProperties)
#ifdef NEW_FOLDER_INTERFACE
  COM_INTERFACE_ENTRY(IFolderManager)
  COM_INTERFACE_ENTRY(IFolderManagerGetIconPath)
#endif
  #endif
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CAgent)

// DECLARE_NO_REGISTRY();
DECLARE_REGISTRY(CAgent, TEXT("SevenZip.Plugin7zip.1"), TEXT("SevenZip.Plugin7zip"), 
    UINT(0), THREADFLAGS_APARTMENT)


  STDMETHOD(Open)(IInStream *stream, 
      const wchar_t *defaultName,
      const FILETIME *defaultTime,
      UINT32 defaultAttributes,
      const UINT64 *maxCheckStartPosition,
      const CLSID *aCLSID, 
      IOpenArchive2CallBack *openArchiveCallback);  

  STDMETHOD(ReOpen)(IInStream *stream, 
      const wchar_t *defaultName,
      const FILETIME *defaultTime,
      UINT32 defaultAttributes,
      const UINT64 *maxCheckStartPosition,
      IOpenArchive2CallBack *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(BindToRootFolder)(IFolderFolder **resultFolder);  
  STDMETHOD(Extract)(
      NExtractionMode::NPath::EEnum pathMode, 
      NExtractionMode::NOverwrite::EEnum overwriteMode, 
      const wchar_t *path, 
      INT32 testMode,
      IExtractCallback2 *extractCallback2);

  #ifndef EXTRACT_ONLY
  STDMETHOD(SetFolder)(IFolderFolder *folder);
  STDMETHOD(SetFiles)(const wchar_t *folderPrefix, const wchar_t **names, UINT32 numNames);
  STDMETHOD(DeleteItems)(const wchar_t *newArchiveName, const UINT32 *indices, 
      UINT32 numItems, IUpdateCallback100 *updateCallback);
  STDMETHOD(DoOperation)(const CLSID *clsID, 
      const wchar_t *newArchiveName, 
      const BYTE *stateActions, 
      const wchar_t *sfxModule,
      IUpdateCallback100 *updateCallback);

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *names, const PROPVARIANT *values, INT32 numProperties);
  #endif

#ifdef NEW_FOLDER_INTERFACE
  // IFolderManager
  STDMETHOD(OpenFolderFile)(const wchar_t *filePath, IFolderFolder **resultFolder, IProgress *progress);
  STDMETHOD(GetTypes)(BSTR *types);
  STDMETHOD(GetExtension)(const wchar_t *type, BSTR *extension);
  STDMETHOD(CreateFolderFile)(const wchar_t *type, const wchar_t *filePath, IProgress *progress);
  STDMETHOD(GetIconPath)(const wchar_t *type, BSTR *iconPath);
  /*
  STDMETHOD(FolderOpen)(
      const wchar_t *aFileName, 
      IOpenArchive2CallBack *anOpenArchive2CallBack);
  */
  HRESULT FolderReOpen(IOpenArchive2CallBack *openArchiveCallback);  
#endif

  CAgent();
  ~CAgent();
private:
  HRESULT ReadItems();
  void LoadFormats();
  int FindFormat(const UString &type); 
public:
  CAgentProxyHandler *_proxyHandler;
  CComPtr<IArchiveHandler200> _archive;
  CLSID _CLSID;
  // CComPtr<IArchiveFolder> m_RootFolder;
  UString _defaultName;
  FILETIME _defaultTime;
  UINT32 _defaultAttributes;

  UStringVector _names;
  UString _folderPrefix;

  UString _archiveNamePrefix;
  CFolderItem *_archiveFolderItem;

  CSysString _archiveFilePath;

  bool _formatsLoaded;
  CObjectVector<NZipRootRegistry::CArchiverInfo> _formats;

  CObjectVector<CComBSTR> m_PropNames;
  std::vector<NWindows::NCOM::CPropVariant> m_PropValues;
};

#endif
