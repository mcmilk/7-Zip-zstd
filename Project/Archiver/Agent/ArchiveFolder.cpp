// Zip/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/FileDir.h"

#include "Interface/FileStreams.h"
#include "Compression/CopyCoder.h"

#include "../Common/UpdatePairInfo.h"
#include "../Common/UpdatePairBasic.h"
#include "../Common/CompressEngineCommon.h"
#include "../Common/UpdateProducer.h"

#include "Handler.h"

#include "ArchiveExtractCallback.h"

using namespace NWindows;
using namespace NCOM;

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

STDMETHODIMP CAgentFolder::CopyTo(const UINT32 *indices, UINT32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  UINT codePage = GetCurrentFileCodePage();
  CComObjectNoLock<CArchiveExtractCallback> *extractCallbackSpec = new 
      CComObjectNoLock<CArchiveExtractCallback>;
  CComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UStringVector pathParts;
  CFolderItem *currentProxyFolder = _proxyFolderItem;
  while (currentProxyFolder->Parent)
  {
    pathParts.Insert(0, currentProxyFolder->Name);
    currentProxyFolder = currentProxyFolder->Parent;
  }

  CComPtr<IFolderArchiveExtractCallback> extractCallback2;
  {
    CComPtr<IFolderOperationsExtractCallback> callbackWrap = callback;
    RETURN_IF_NOT_S_OK(callbackWrap.QueryInterface(&extractCallback2));
  }

  extractCallbackSpec->Init(_proxyHandler->Archive, 
      extractCallback2, 
      GetSystemString(path, codePage),
      NExtractionMode::NPath::kCurrentPathnames, 
      NExtractionMode::NOverwrite::kAskBefore, 
      pathParts, 
      codePage, 
      _proxyHandler->ItemDefaultName,
      _proxyHandler->DefaultTime, 
      _proxyHandler->DefaultAttributes);
  CUIntVector realIndices;
  _proxyHandler->GetRealIndices(*_proxyFolderItem, indices, numItems, realIndices);
  return _proxyHandler->Archive->Extract(&realIndices.Front(), 
      realIndices.Size(), BoolToInt(false), extractCallback);
}

STDMETHODIMP CAgentFolder::MoveTo(const UINT32 *indices, UINT32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  return E_NOTIMPL;
}

