// Zip/ArchiveFolder.cpp

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

#include "ExtractCallback200.h"

using namespace NWindows;
using namespace NCOM;

static inline UINT GetCurrentFileCodePage()
{
  return AreFileApisANSI() ? CP_ACP : CP_OEMCP;
}

STDMETHODIMP CAgentFolder::Copy(const UINT32 *anIndexes, 
    UINT32 aNumItems, 
    const wchar_t *aPath,
    IExtractCallback3 *anExtractCallBack3)
{
  UINT aCodePage = GetCurrentFileCodePage();
  CComObjectNoLock<CExtractCallBack200Imp> *anExtractCallBackSpec = new 
      CComObjectNoLock<CExtractCallBack200Imp>;
  CComPtr<IExtractCallback200> anExtractCallBack = anExtractCallBackSpec;
  UStringVector aPathParts;
  CFolderItem *aCurrentProxyFolder = m_ProxyFolderItem;
  while (aCurrentProxyFolder->m_Parent)
  {
    aPathParts.Insert(0, aCurrentProxyFolder->m_Name);
    aCurrentProxyFolder = aCurrentProxyFolder->m_Parent;
  }
  anExtractCallBackSpec->Init(m_ProxyHandler->m_ArchiveHandler, 
      anExtractCallBack3, 
      GetSystemString(aPath, aCodePage),
      NExtractionMode::NPath::kCurrentPathnames, 
      NExtractionMode::NOverwrite::kAskBefore, 
      aPathParts, 
      aCodePage, 
      m_ProxyHandler->m_ItemDefaultName,
      m_ProxyHandler->m_DefaultTime, 
      m_ProxyHandler->m_DefaultAttributes, 
      false, 
      L"");
  std::vector<UINT32> aRealIndexes;
  m_ProxyHandler->GetRealIndexes(*m_ProxyFolderItem, anIndexes, aNumItems, aRealIndexes);
  return m_ProxyHandler->m_ArchiveHandler->Extract(&aRealIndexes.front(), 
      aRealIndexes.size(), BoolToMyBool(false), anExtractCallBack);
}

STDMETHODIMP CAgentFolder::Move(const UINT32 *anIndexes, 
    UINT32 aNumItems, 
    const wchar_t *aPath,
    IExtractCallback3 *anExtractCallBack3)
{
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::Delete(const UINT32 *anIndexes, UINT32 aNumItems)
{
  return E_NOTIMPL;
  /*
  for (UINT32 i = 0; i < aNumItems; i++)
  {
    int anIndex = anIndexes[i];
    const CFileInfo &aFileInfo = m_Files[anIndexes[i]];
    const CSysString aFullPath = m_Path + aFileInfo.Name;
    bool aResult;
    if (aFileInfo.IsDirectory())
      aResult = NDirectory::RemoveDirectoryWithSubItems(aFullPath);
    else
      aResult = NDirectory::DeleteFileAlways(aFullPath);
    if (!aResult)
      return GetLastError();
  }
  return S_OK;
  */
}

STDMETHODIMP CAgentFolder::Rename(UINT32 anIndex, const wchar_t *aNewName)
{
  return E_NOTIMPL;
  /*
  const CFileInfo &aFileInfo = m_Files[anIndex];
  if (!::MoveFile(m_Path + aFileInfo.Name, m_Path + 
      GetSystemString(aNewName, m_FileCodePage)))
    return GetLastError();
  return S_OK;
  */
}

STDMETHODIMP CAgentFolder::CreateFolder(const wchar_t *aName)
{
  return E_NOTIMPL;
  /*
  CSysString aProcessedName;
  RETURN_IF_NOT_S_OK(GetComplexName(aName, aProcessedName));
  if (!NDirectory::CreateComplexDirectory(aProcessedName))
    return GetLastError();
  return S_OK;
  */
}

STDMETHODIMP CAgentFolder::CreateFile(const wchar_t *aName)
{
  return E_NOTIMPL;
  /*
  CSysString aProcessedName;
  RETURN_IF_NOT_S_OK(GetComplexName(aName, aProcessedName));
  NIO::COutFile anOutFile;
  if (!anOutFile.Open(aProcessedName))
    return GetLastError();
  return S_OK;
  */
}
