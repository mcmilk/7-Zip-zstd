// Agent/Agent.h

#ifndef __AGENT_AGENT_H
#define __AGENT_AGENT_H

#include "Common/MyCom.h"
#include "Windows/PropVariant.h"

#include "../Common/UpdateAction.h"
#include "../Common/ArchiverInfo.h"
#include "../Common/OpenArchive.h"

#include "IFolderArchive.h"
#include "AgentProxy.h"

#ifndef EXCLUDE_COM
#include "Windows/DLL.h"
#endif
#ifdef NEW_FOLDER_INTERFACE
#include "../../FileManager/IFolder.h"
#endif

class CAgentFolder;

// {23170F69-40C1-278A-0000-000100050001}
DEFINE_GUID(IID_IArchiveFolderInternal, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100050001")
IArchiveFolderInternal: public IUnknown
{
public:
  STDMETHOD(GetAgentFolder)(CAgentFolder **agentFolder) PURE;  
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
  public CMyUnknownImp
{
public:

  MY_QUERYINTERFACE_BEGIN 
    MY_QUERYINTERFACE_ENTRY(IFolderFolder)
    MY_QUERYINTERFACE_ENTRY(IArchiveFolder)
    MY_QUERYINTERFACE_ENTRY(IArchiveFolderInternal)
  #ifdef NEW_FOLDER_INTERFACE
    MY_QUERYINTERFACE_ENTRY(IEnumProperties)
    MY_QUERYINTERFACE_ENTRY(IFolderGetTypeID)
    MY_QUERYINTERFACE_ENTRY(IFolderGetPath)
    MY_QUERYINTERFACE_ENTRY(IFolderOperations)
  #endif
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  // IFolderFolder
  
  STDMETHOD(LoadItems)();
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 itemIndex, PROPID propID, PROPVARIANT *value);
  STDMETHOD(BindToFolder)(UINT32 index, IFolderFolder **resultFolder);
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder);
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder);
  STDMETHOD(GetName)(BSTR *name);

  // IArchiveFolder
  STDMETHOD(Extract)(const UINT32 *indices, UINT32 numItems, 
      NExtract::NPathMode::EEnum pathMode, 
      NExtract::NOverwriteMode::EEnum overwriteMode, 
      const wchar_t *path,
      INT32 testMode,
      IFolderArchiveExtractCallback *extractCallback);
  
  STDMETHOD(GetAgentFolder)(CAgentFolder **agentFolder);

  #ifdef NEW_FOLDER_INTERFACE
  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);
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
  STDMETHOD(SetProperty)(UINT32 index, PROPID propID, const PROPVARIANT *value, IProgress *progress);


#endif

  CAgentFolder(): _proxyFolderItem(NULL) {}

  void Init(CProxyArchive *proxyHandler,
      CProxyFolder *proxyFolderItem,
      IFolderFolder *parentFolder,
      CAgent *agent)
  {
    _proxyArchive = proxyHandler;
    _proxyFolderItem = proxyFolderItem;
    _parentFolder = parentFolder;
    _agent = (IInFolderArchive *)agent;
    _agentSpec = agent;
  }

  void GetPathParts(UStringVector &pathParts);
  HRESULT CommonUpdateOperation(
      bool deleteOperation,
      bool createFolderOperation,
      bool renameOperation,
      const wchar_t *newItemName, 
      const NUpdateArchive::CActionSet *actionSet,
      const UINT32 *indices, UINT32 numItems,
      IFolderArchiveUpdateCallback *updateCallback100);


public:
  CProxyArchive *_proxyArchive;
  CProxyFolder *_proxyFolderItem;
  CMyComPtr<IFolderFolder> _parentFolder;
  CMyComPtr<IInFolderArchive> _agent;
  CAgent *_agentSpec;
private:
};

// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);

class CAgent: 
  public IInFolderArchive,
  #ifndef EXTRACT_ONLY
  public IOutFolderArchive,
  public ISetProperties,
  #endif
  public CMyUnknownImp
{
public:

  MY_QUERYINTERFACE_BEGIN 
    MY_QUERYINTERFACE_ENTRY(IInFolderArchive)
  #ifndef EXTRACT_ONLY
    MY_QUERYINTERFACE_ENTRY(IOutFolderArchive)
    MY_QUERYINTERFACE_ENTRY(ISetProperties)
  #endif
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  STDMETHOD(Open)(
      const wchar_t *filePath, 
      // CLSID *clsIDResult,
      BSTR *archiveType,
      IArchiveOpenCallback *openArchiveCallback);  

  STDMETHOD(ReOpen)(
      // const wchar_t *filePath, 
      IArchiveOpenCallback *openArchiveCallback);  
  /*
  STDMETHOD(ReOpen)(IInStream *stream, 
      const wchar_t *defaultName,
      const FILETIME *defaultTime,
      UINT32 defaultAttributes,
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  */
  STDMETHOD(Close)();  
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);
  STDMETHOD(GetNumberOfArchiveProperties)(UINT32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);
  STDMETHOD(BindToRootFolder)(IFolderFolder **resultFolder);  
  STDMETHOD(Extract)(
      NExtract::NPathMode::EEnum pathMode, 
      NExtract::NOverwriteMode::EEnum overwriteMode, 
      const wchar_t *path, 
      INT32 testMode,
      IFolderArchiveExtractCallback *extractCallback2);

  #ifndef EXTRACT_ONLY
  STDMETHOD(SetFolder)(IFolderFolder *folder);
  STDMETHOD(SetFiles)(const wchar_t *folderPrefix, const wchar_t **names, UINT32 numNames);
  STDMETHOD(DeleteItems)(const wchar_t *newArchiveName, const UINT32 *indices, 
      UINT32 numItems, IFolderArchiveUpdateCallback *updateCallback);
  STDMETHOD(DoOperation)(
      const wchar_t *filePath, 
      const CLSID *clsID, 
      const wchar_t *newArchiveName, 
      const Byte *stateActions, 
      const wchar_t *sfxModule,
      IFolderArchiveUpdateCallback *updateCallback);

  HRESULT CommonUpdate(
      const wchar_t *newArchiveName, 
      int numUpdateItems,
      IArchiveUpdateCallback *updateCallback);
  
  HRESULT CreateFolder(
    const wchar_t *newArchiveName, 
    const wchar_t *folderName, 
    IFolderArchiveUpdateCallback *updateCallback100);

  HRESULT RenameItem(
    const wchar_t *newArchiveName, 
    const UINT32 *indices, UINT32 numItems, 
    const wchar_t *newItemName, 
    IFolderArchiveUpdateCallback *updateCallback100);

  // ISetProperties
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, INT32 numProperties);
  #endif

  CAgent();
  ~CAgent();
private:
  HRESULT ReadItems();
public:
  CProxyArchive *_proxyArchive;

  CArchiveLink _archiveLink;
  // IInArchive *_archive2;
  
  // CLSID _CLSID;
  // CMyComPtr<IArchiveFolder> m_RootFolder;
  
  UString DefaultName;

  FILETIME DefaultTime;
  UINT32 DefaultAttributes;

  UString ArchiveType;

  UStringVector _names;
  UString _folderPrefix;

  UString _archiveNamePrefix;
  CProxyFolder *_archiveFolderItem;

  UString _archiveFilePath;

  #ifndef EXTRACT_ONLY
  CObjectVector<UString> m_PropNames;
  CObjectVector<NWindows::NCOM::CPropVariant> m_PropValues;
  #endif

  IInArchive *GetArchive() { return _archiveLink.GetArchive(); }
  bool CanUpdate() const { return _archiveLink.GetNumLevels() <= 1; }
};

#ifdef NEW_FOLDER_INTERFACE
class CArchiveFolderManager: 
  public IFolderManager,
  public IFolderManagerGetIconPath,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(
    IFolderManager,
    IFolderManagerGetIconPath
    )
  // IFolderManager
  STDMETHOD(OpenFolderFile)(const wchar_t *filePath, IFolderFolder **resultFolder, IProgress *progress);
  STDMETHOD(GetTypes)(BSTR *types);
  STDMETHOD(GetExtension)(const wchar_t *type, BSTR *extension);
  STDMETHOD(CreateFolderFile)(const wchar_t *type, const wchar_t *filePath, IProgress *progress);
  STDMETHOD(GetIconPath)(const wchar_t *type, BSTR *iconPath);
  CArchiveFolderManager(): _formatsLoaded(false) {}
private:
  void LoadFormats();
  int FindFormat(const UString &type); 
  bool _formatsLoaded;
  CObjectVector<CArchiverInfo> _formats;
};
#endif

#endif
