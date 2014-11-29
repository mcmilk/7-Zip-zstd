// Agent/Agent.h

#ifndef __AGENT_AGENT_H
#define __AGENT_AGENT_H

#include "../../../Common/MyCom.h"

#include "../../../Windows/PropVariant.h"

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
  unsigned ProxyFolderIndex;
  unsigned Index;
};

class CAgent;

enum AGENT_OP
{
  AGENT_OP_Uni,
  AGENT_OP_Delete,
  AGENT_OP_CreateFolder,
  AGENT_OP_Rename,
  AGENT_OP_CopyFromFile
};

class CAgentFolder:
  public IFolderFolder,
  public IFolderProperties,
  public IArchiveGetRawProps,
  public IGetFolderArcProps,
  public IFolderCompare,
  public IFolderGetItemName,
  public IArchiveFolder,
  public IArchiveFolderInternal,
  public IInArchiveGetStream,
  // public IFolderSetReplaceAltStreamCharsMode,
#ifdef NEW_FOLDER_INTERFACE
  public IFolderOperations,
  public IFolderSetFlatMode,
#endif
  public CMyUnknownImp
{
  void LoadFolder(unsigned proxyFolderIndex);
public:

  MY_QUERYINTERFACE_BEGIN2(IFolderFolder)
    MY_QUERYINTERFACE_ENTRY(IFolderProperties)
    MY_QUERYINTERFACE_ENTRY(IArchiveGetRawProps)
    MY_QUERYINTERFACE_ENTRY(IGetFolderArcProps)
    MY_QUERYINTERFACE_ENTRY(IFolderCompare)
    MY_QUERYINTERFACE_ENTRY(IFolderGetItemName)
    MY_QUERYINTERFACE_ENTRY(IArchiveFolder)
    MY_QUERYINTERFACE_ENTRY(IArchiveFolderInternal)
    MY_QUERYINTERFACE_ENTRY(IInArchiveGetStream)
    // MY_QUERYINTERFACE_ENTRY(IFolderSetReplaceAltStreamCharsMode)
  #ifdef NEW_FOLDER_INTERFACE
    MY_QUERYINTERFACE_ENTRY(IFolderOperations)
    MY_QUERYINTERFACE_ENTRY(IFolderSetFlatMode)
  #endif
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  HRESULT BindToFolder_Internal(unsigned proxyFolderIndex, IFolderFolder **resultFolder);
  int GetRealIndex(unsigned index) const;
  void GetRealIndices(const UInt32 *indices, UInt32 numItems,
      bool includeAltStreams, bool includeFolderSubItemsInFlatMode, CUIntVector &realIndices) const;

  // INTERFACE_FolderSetReplaceAltStreamCharsMode(;)

  INTERFACE_FolderFolder(;)
  INTERFACE_FolderProperties(;)
  INTERFACE_IArchiveGetRawProps(;)
  INTERFACE_IFolderGetItemName(;)

  STDMETHOD(GetFolderArcProps)(IFolderArcProps **object);
  STDMETHOD_(Int32, CompareItems)(UInt32 index1, UInt32 index2, PROPID propID, Int32 propIsRaw);
  int CompareItems2(UInt32 index1, UInt32 index2, PROPID propID, Int32 propIsRaw);

  // IArchiveFolder
  INTERFACE_IArchiveFolder(;)
  
  STDMETHOD(GetAgentFolder)(CAgentFolder **agentFolder);

  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);

  #ifdef NEW_FOLDER_INTERFACE
  INTERFACE_FolderOperations(;)

  STDMETHOD(SetFlatMode)(Int32 flatMode);
  #endif

  CAgentFolder(): _proxyFolderItem(0), _flatMode(0) /* , _replaceAltStreamCharsMode(0) */ {}

  void Init(const CProxyArchive *proxyArc, const CProxyArchive2 *proxyArc2,
      unsigned proxyFolderItem,
      IFolderFolder *parentFolder,
      CAgent *agent)
  {
    _proxyArchive = proxyArc;
    _proxyArchive2 = proxyArc2;
    _proxyFolderItem = proxyFolderItem;
    _parentFolder = parentFolder;
    _agent = (IInFolderArchive *)agent;
    _agentSpec = agent;
  }

  void GetPathParts(UStringVector &pathParts);
  HRESULT CommonUpdateOperation(
      AGENT_OP operation,
      bool moveMode,
      const wchar_t *newItemName,
      const NUpdateArchive::CActionSet *actionSet,
      const UInt32 *indices, UInt32 numItems,
      IFolderArchiveUpdateCallback *updateCallback100);


  void GetPrefix(UInt32 index, UString &prefix) const;
  UString GetName(UInt32 index) const;
  UString GetFullPathPrefixPlusPrefix(UInt32 index) const;

public:
  const CProxyArchive *_proxyArchive;
  const CProxyArchive2 *_proxyArchive2;
  // const CProxyFolder *_proxyFolderItem;
  unsigned _proxyFolderItem;
  CMyComPtr<IFolderFolder> _parentFolder;
  CMyComPtr<IInFolderArchive> _agent;
  CAgent *_agentSpec;

  CRecordVector<CProxyItem> _items;
  bool _flatMode;
  // Int32 _replaceAltStreamCharsMode;
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

  HRESULT CommonUpdate(ISequentialOutStream *outArchiveStream,
      unsigned numUpdateItems, IArchiveUpdateCallback *updateCallback);
  
  HRESULT CreateFolder(ISequentialOutStream *outArchiveStream,
      const wchar_t *folderName, IFolderArchiveUpdateCallback *updateCallback100);

  HRESULT RenameItem(ISequentialOutStream *outArchiveStream,
      const UInt32 *indices, UInt32 numItems, const wchar_t *newItemName,
      IFolderArchiveUpdateCallback *updateCallback100);

  HRESULT UpdateOneFile(ISequentialOutStream *outArchiveStream,
      const UInt32 *indices, UInt32 numItems, const wchar_t *diskFilePath,
      IFolderArchiveUpdateCallback *updateCallback100);

  // ISetProperties
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, UInt32 numProps);
  #endif

  CCodecs *_codecs;
  CMyComPtr<ICompressCodecsInfo> _compressCodecsInfo;

  CAgent();
  ~CAgent();
private:
  HRESULT ReadItems();
public:
  CProxyArchive *_proxyArchive;
  CProxyArchive2 *_proxyArchive2;
  CArchiveLink _archiveLink;

  bool ThereIsPathProp;

  UString ArchiveType;

  FStringVector _names;
  FString _folderPrefix;

  UString _archiveNamePrefix;
  CAgentFolder *_agentFolder;

  UString _archiveFilePath;
  bool _isDeviceFile;

  #ifndef EXTRACT_ONLY
  CObjectVector<UString> m_PropNames;
  CObjectVector<NWindows::NCOM::CPropVariant> m_PropValues;
  #endif

  const CArc &GetArc() const { return _archiveLink.Arcs.Back(); }
  IInArchive *GetArchive() const { if ( _archiveLink.Arcs.IsEmpty()) return 0; return GetArc().Archive; }
  bool CanUpdate() const;

  UString GetTypeOfArc(const CArc &arc) const
  {
    if (arc.FormatIndex < 0)
      return L"Parser";
    return _codecs->GetFormatNamePtr(arc.FormatIndex);
  }

  UString GetErrorMessage() const
  {
    UString s;
    for (int i = _archiveLink.Arcs.Size() - 1; i >= 0; i--)
    {
      const CArc &arc = _archiveLink.Arcs[i];

      UString s2;
      if (arc.ErrorInfo.ErrorFormatIndex >= 0)
        s2 = L"Can not open the file as [" + _codecs->Formats[arc.ErrorInfo.ErrorFormatIndex].Name + L"] archive";

      if (!arc.ErrorInfo.ErrorMessage.IsEmpty())
      {
        if (!s2.IsEmpty())
          s2 += L"\n";
        s2 += L"\n[";
        s2 += GetTypeOfArc(arc);
        s2 += L"]: ";
        s2 += arc.ErrorInfo.ErrorMessage;
      }
      if (!s2.IsEmpty())
      {
        if (!s.IsEmpty())
          s += L"--------------------\n";
        s += arc.Path;
        s += L"\n";
        s += s2;
        s += L"\n";
      }
    }
    return s;
  }

  void KeepModeForNextOpen() { _archiveLink.KeepModeForNextOpen(); }

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
