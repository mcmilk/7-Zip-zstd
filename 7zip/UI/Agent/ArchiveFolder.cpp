// Zip/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/FileDir.h"

#include "../../Common/FileStreams.h"

#include "../Common/UpdatePair.h"

#include "Agent.h"
#include "ArchiveExtractCallback.h"

using namespace NWindows;
using namespace NCOM;

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

STDMETHODIMP CAgentFolder::CopyTo(const UINT32 *indices, UINT32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  UINT codePage = GetCurrentFileCodePage();
  CArchiveExtractCallback *extractCallbackSpec = new 
      CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UStringVector pathParts;
  CProxyFolder *currentProxyFolder = _proxyFolderItem;
  while (currentProxyFolder->Parent)
  {
    pathParts.Insert(0, currentProxyFolder->Name);
    currentProxyFolder = currentProxyFolder->Parent;
  }

  CMyComPtr<IFolderArchiveExtractCallback> extractCallback2;
  {
    CMyComPtr<IFolderOperationsExtractCallback> callbackWrap = callback;
    RINOK(callbackWrap.QueryInterface(
        IID_IFolderArchiveExtractCallback, &extractCallback2));
  }

  extractCallbackSpec->Init(_agentSpec->_archive, 
      extractCallback2, 
      GetSystemString(path, codePage),
      NExtractionMode::NPath::kCurrentPathnames, 
      NExtractionMode::NOverwrite::kAskBefore, 
      pathParts, 
      codePage, 
      _agentSpec->DefaultName,
      _agentSpec->DefaultTime, 
      _agentSpec->DefaultAttributes
      // ,_agentSpec->_srcDirectoryPrefix
      );
  CUIntVector realIndices;
  _proxyFolderItem->GetRealIndices(indices, numItems, realIndices);
  return _agentSpec->_archive->Extract(&realIndices.Front(), 
      realIndices.Size(), BoolToInt(false), extractCallback);
}

STDMETHODIMP CAgentFolder::MoveTo(const UINT32 *indices, UINT32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  return E_NOTIMPL;
}

