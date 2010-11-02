// Agent/Agent.h

#ifndef __AGENT_AGENT_H
#define __AGENT_AGENT_H

#include "Common/MyCom.h"

#include "Windows/PropVariant.h"

#include "../Common/OpenArchive.h"
#include "../Common/UpdateAction.h"

#ifdef NEW_FOLDER_INTERFACE
#include "../FileManager/IFolder.h"
#include "../Common/LoadCodecs.h"
#endif

#include "AgentProxy.h"
#include "IFolderArchive.h"

class CAgentFolder;

DECL_INTERFACE(IArchiveFolderInternal, 0x01, 0xC)
{
  STDMETHOD(GetAgentFolder)(CAgentFolder **agentFolder) PURE;
};

struct CProxyItem
{
  CProxyFolder *Folder;
  UInt32 Index;
};

class CAgent;

class CAgentFolder:
  public IFolderFolder,
  public IFolderProperties,
  public IGetFolderArcProps,
  public IArchiveFolder,
  public IArchiveFolderInternal,
  public IInArchiveGetStream,
#ifdef NEW_FOLDER_INTERFACE
  public IFolderOperations,
  public IFolderSetFlatMode,
#endif
  public CMyUnknownImp
{
public:

  MY_QUERYINTERFACE_BEGIN2(IFolderFolder)
    MY_QUERYINTERFACE_ENTRY(IFolderProperties)
    MY_QUERYINTERFACE_ENTRY(IGetFolderArcProps)
    MY_QUERYINTERFACE_ENTRY(IArchiveFolder)
    MY_QUERYINTERFACE_ENTRY(IArchiveFolderInternal)
    MY_QUERYINTERFACE_ENTRY(IInArchiveGetStream)
  #ifdef NEW_FOLDER_INTERFACE
    MY_QUERYINTERFACE_ENTRY(IFolderOperations)
    MY_QUERYINTERFACE_ENTRY(IFolderSetFlatMode)
  #endif
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  void LoadFolder(CProxyFolder *folder);
  HRESULT BindToFolder(CProxyFolder *folder, IFolderFolder **resultFolder);
  void GetRealIndices(const UINT32 *indices, UINT32 numItems, CUIntVector &realIndices) const;

  INTERFACE_FolderFolder(;)
  INTERFACE_FolderProperties(;)

  STDMETHOD(GetFolderArcProps)(IFolderArcProps **object);

  // IArchiveFolder
  STDMETHOD(Extract)(const UINT32 *indices, UINT32 numItems,
      NExtract::NPathMode::EEnum pathMode,
      NExtract::NOverwriteMode::EEnum overwriteMode,
      const wchar_t *path,
      Int32 testMode,
      IFolderArchiveExtractCallback *extractCallback);
  
  STDMETHOD(GetAgentFolder)(CAgentFolder **agentFolder);

  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);

  #ifdef NEW_FOLDER_INTERFACE
  INTERFACE_FolderOperations(;)

  STDMETHOD(SetFlatMode)(Int32 flatMode);
  #endif

  CAgentFolder(): _proxyFolderItem(NULL), _flatMode(0) {}

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


  UString GetPrefix(UInt32 index) const;
  UString GetName(UInt32 index) const;
  UString GetFullPathPrefixPlusPrefix(UInt32 index) const;
  void GetPrefixIfAny(UInt32 index, NWindows::NCOM::CPropVariant &propVariant) const;

public:
  CProxyArchive *_proxyArchive;
  CProxyFolder *_proxyFolderItem;
  CMyComPtr<IFolderFolder> _parentFolder;
  CMyComPtr<IInFolderArchive> _agent;
  CAgent *_agentSpec;

  CRecordVector<CProxyItem> _items;
  bool _flatMode;
private:
};

class CAgent:
  public IInFolderArchive,
  public IFolderArcProps,
  #ifndef EXTRACT_ONLY
  public IOutFolderArchive,
  public ISetProperties,
  #endif
  public CMyUnknownImp
{
public:

  MY_QUERYINTERFACE_BEGIN2(IInFolderArchive)
    MY_QUERYINTERFACE_ENTRY(IFolderArcProps)
  #ifndef EXTRACT_ONLY
    MY_QUERYINTERFACE_ENTRY(IOutFolderArchive)
    MY_QUERYINTERFACE_ENTRY(ISetProperties)
  #endif
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  INTERFACE_IInFolderArchive(;)
  INTERFACE_IFolderArcProps(;)

  #ifndef EXTRACT_ONLY
  INTERFACE_IOutFolderArchive(;)

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
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties);
  #endif

  CCodecs *_codecs;
  CMyComPtr<ICompressCodecsInfo> _compressCodecsInfo;

  CAgent();
  ~CAgent();
private:
  HRESULT ReadItems();
public:
  CProxyArchive *_proxyArchive;
  CArchiveLink _archiveLink;


  UString ArchiveType;

  UStringVector _names;
  UString _folderPrefix;

  UString _archiveNamePrefix;
  CAgentFolder *_agentFolder;

  UString _archiveFilePath;

  #ifndef EXTRACT_ONLY
  CObjectVector<UString> m_PropNames;
  CObjectVector<NWindows::NCOM::CPropVariant> m_PropValues;
  #endif

  const CArc &GetArc() { return _archiveLink.Arcs.Back(); }
  IInArchive *GetArchive() { if ( _archiveLink.Arcs.IsEmpty()) return 0; return GetArc().Archive; }
  bool CanUpdate() const { return _archiveLink.Arcs.Size() <= 1; }

  UString GetTypeOfArc(const CArc &arc) const { return  _codecs->Formats[arc.FormatIndex].Name; }
  UString GetErrorMessage() const
  {
    UString s;
    for (int i = _archiveLink.Arcs.Size() - 1; i >= 0; i--)
    {
      const CArc &arc = _archiveLink.Arcs[i];
      if (arc.ErrorMessage.IsEmpty())
        continue;
      if (!s.IsEmpty())
        s += L"--------------------\n";
      s += arc.ErrorMessage;
      s += L"\n\n[";
      s += GetTypeOfArc(arc);
      s += L"] ";
      s += arc.Path;
      s += L"\n";
    }
    return s;
  }
};

#ifdef NEW_FOLDER_INTERFACE
class CArchiveFolderManager:
  public IFolderManager,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IFolderManager)

  INTERFACE_IFolderManager(;)

  CArchiveFolderManager(): _codecs(0) {}
private:
  void LoadFormats();
  int FindFormat(const UString &type);
  CCodecs *_codecs;
  CMyComPtr<ICompressCodecsInfo> _compressCodecsInfo;
};
#endif

#endif
