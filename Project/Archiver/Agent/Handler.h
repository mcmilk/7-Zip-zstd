// Agent/Handler.h

#pragma once

#ifndef __AGENT_HANDLER_H
#define __AGENT_HANDLER_H


#include "../Common/IArchiveHandler2.h"
// #include "../../Compress/Interface/CompressInterface.h"

#include "AgentProxyHandler.h"

#include "Windows/PropVariant.h"

#ifdef NEW_FOLDER_INTERFACE
#include "../../Util/FAM/FolderInterface.h"
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


  STDMETHOD(GetNumberOfItems)(UINT32 *aNumItems);  
  STDMETHOD(GetNumberOfSubFolders)(UINT32 *aNumSubFolders);  
  STDMETHOD(GetProperty)(UINT32 anItemIndex, PROPID aPropID, PROPVARIANT *aValue);
  STDMETHOD(BindToFolder)(UINT32 anIndex, IArchiveFolder **aFolder);
  STDMETHOD(BindToFolder)(const WCHAR *aFolderName, IArchiveFolder **aFolder);
  STDMETHOD(BindToParentFolder)(IArchiveFolder **aFolder);
  STDMETHOD(GetName)(BSTR *aName);

  STDMETHOD(Extract)(const UINT32 *anIndexes, UINT32 aNumItems, 
      NExtractionMode::NPath::EEnum aPathMode, 
      NExtractionMode::NOverwrite::EEnum anOverwriteMode, 
      const wchar_t *aPath,
      INT32 aTestMode,
      IExtractCallback2 *anExtractCallBack2);
  
  STDMETHOD(GetAgentFolder)(CAgentFolder **anAgentFolder);

#ifdef NEW_FOLDER_INTERFACE
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);  
  STDMETHOD(GetTypeID)(BSTR *aName);
  STDMETHOD(GetPath)(BSTR *aPath);


  // IFolderOperations

  STDMETHOD(Delete)(const UINT32 *anIndexes, UINT32 aNumItems);
  STDMETHOD(Rename)(UINT32 anIndex, const wchar_t *aNewName);
  STDMETHOD(CreateFolder)(const wchar_t *aName);
  STDMETHOD(CreateFile)(const wchar_t *aName);
  STDMETHOD(Copy)(const UINT32 *anIndexes, UINT32 aNumItems, 
      const wchar_t *aPath, 
      IExtractCallback3 *anExtractCallBack);
  STDMETHOD(Move)(const UINT32 *anIndexes, UINT32 aNumItems, 
      const wchar_t *aPath, 
      IExtractCallback3 *anExtractCallBack);
#endif

  CAgentFolder(): m_ProxyFolderItem(NULL) {}

  void Init(CAgentProxyHandler *aProxyHandler,
      CFolderItem *aProxyFolderItem,
      IArchiveFolder *aParentFolder,
      CAgent *anAgent)
  {
    m_ProxyHandler = aProxyHandler;
    m_ProxyFolderItem = aProxyFolderItem;
    m_ParentFolder = aParentFolder;
    m_Agent = (IArchiveHandler100 *)anAgent;
    m_AgentSpec = anAgent;
  }

public:
  CAgentProxyHandler *m_ProxyHandler;
  CFolderItem *m_ProxyFolderItem;
  CComPtr<IArchiveFolder> m_ParentFolder;
  CComPtr<IArchiveHandler100> m_Agent;
  CAgent *m_AgentSpec;
};

// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
class CAgent: 
  public IArchiveHandler100,
#ifdef NEW_FOLDER_INTERFACE
  public IFolderOpen,
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
  COM_INTERFACE_ENTRY(IFolderOpen)
#endif
  #endif
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CAgent)

// DECLARE_NO_REGISTRY();
DECLARE_REGISTRY(CAgent, TEXT("SevenZip.Agent.1"), TEXT("SevenZip.Agent"), 
    UINT(0), THREADFLAGS_APARTMENT)


  STDMETHOD(Open)(IInStream *aStream, 
      const wchar_t *aDefaultName,
      const FILETIME *aDefaultTime,
      UINT32 aDefaultAttributes,
      const UINT64 *aMaxCheckStartPosition,
      const CLSID *aCLSID, 
      IOpenArchive2CallBack *anOpenArchiveCallBack);  

  STDMETHOD(ReOpen)(IInStream *aStream, 
      const wchar_t *aDefaultName,
      const FILETIME *aDefaultTime,
      UINT32 aDefaultAttributes,
      const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);  
  STDMETHOD(BindToRootFolder)(IArchiveFolder **aFolder);  
  STDMETHOD(Extract)(
      NExtractionMode::NPath::EEnum aPathMode, 
      NExtractionMode::NOverwrite::EEnum anOverwriteMode, 
      const wchar_t *aPath, 
      INT32 aTestMode,
      IExtractCallback2 *anExtractCallBack2);

  #ifndef EXTRACT_ONLY
  STDMETHOD(SetFolder)(IArchiveFolder *aFolder);
  STDMETHOD(SetFiles)(const wchar_t **aNames, UINT32 aNumNames);
  STDMETHOD(DeleteItems)(const wchar_t *aNewArchiveName, const UINT32 *anIndexes, 
      UINT32 aNumItems, IUpdateCallback100 *anUpdateCallback);
  STDMETHOD(DoOperation)(const CLSID *aCLSID, 
      const wchar_t *aNewArchiveName, 
      const BYTE aStateActions[6], 
      const wchar_t *aSfxModule,
      IUpdateCallback100 *anUpdateCallback);

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties);
  #endif

#ifdef NEW_FOLDER_INTERFACE
  // IFolderOpen
  STDMETHOD(FolderOpen)(
      const wchar_t *aFileName, 
      IOpenArchive2CallBack *anOpenArchive2CallBack);
#endif


  CAgent();
  ~CAgent();
private:
  HRESULT ReadItems();
public:
  CAgentProxyHandler *m_ProxyHandler;
  CComPtr<IArchiveHandler200> m_Archive;
  CLSID m_CLSID;
  // CComPtr<IArchiveFolder> m_RootFolder;
  UString m_DefaultName;
  FILETIME m_DefaultTime;
  UINT32 m_DefaultAttributes;

  UStringVector m_Names;
  UString m_ArchiveNamePrefix;
  CFolderItem *m_ArchiveFolderItem;


  CObjectVector<CComBSTR> m_PropNames;
  std::vector<NWindows::NCOM::CPropVariant> m_PropValues;
};

#endif
