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
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

STDMETHODIMP CAgentFolder::CopyTo(const UINT32 *indices, UINT32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  UINT codePage = GetCurrentFileCodePage();
  CComObjectNoLock<CExtractCallBack200Imp> *extractCallback200Spec = new 
      CComObjectNoLock<CExtractCallBack200Imp>;
  CComPtr<IExtractCallback200> extractCallback200 = extractCallback200Spec;
  UStringVector pathParts;
  CFolderItem *currentProxyFolder = _proxyFolderItem;
  while (currentProxyFolder->m_Parent)
  {
    pathParts.Insert(0, currentProxyFolder->m_Name);
    currentProxyFolder = currentProxyFolder->m_Parent;
  }

  CComPtr<IExtractCallback2> extractCallback2;
  {
    CComPtr<IFolderOperationsExtractCallback> callbackWrap = callback;
    RETURN_IF_NOT_S_OK(callbackWrap.QueryInterface(&extractCallback2));
  }

  extractCallback200Spec->Init(_proxyHandler->_archiveHandler, 
      extractCallback2, 
      GetSystemString(path, codePage),
      NExtractionMode::NPath::kCurrentPathnames, 
      NExtractionMode::NOverwrite::kAskBefore, 
      pathParts, 
      codePage, 
      _proxyHandler->_itemDefaultName,
      _proxyHandler->_defaultTime, 
      _proxyHandler->_defaultAttributes, 
      false, 
      L"");
  std::vector<UINT32> realIndices;
  _proxyHandler->GetRealIndexes(*_proxyFolderItem, indices, numItems, realIndices);
  return _proxyHandler->_archiveHandler->Extract(&realIndices.front(), 
      realIndices.size(), BoolToMyBool(false), extractCallback200);
}

STDMETHODIMP CAgentFolder::MoveTo(const UINT32 *indices, UINT32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::Rename(UINT32 index, const wchar_t *newName, IProgress *progress)
{
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::CreateFolder(const wchar_t *name, IProgress *progress)
{
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::CreateFile(const wchar_t *name, IProgress *progress)
{
  return E_NOTIMPL;
}
