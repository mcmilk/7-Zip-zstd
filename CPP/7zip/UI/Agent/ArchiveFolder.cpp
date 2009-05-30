// Agent/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"

#include "../Common/ArchiveExtractCallback.h"

#include "Agent.h"

using namespace NWindows;
using namespace NCOM;

STDMETHODIMP CAgentFolder::CopyTo(const UInt32 *indices, UInt32 numItems,
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  COM_TRY_BEGIN
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

  NExtract::NPathMode::EEnum pathMode = _flatMode ?
      NExtract::NPathMode::kNoPathnames :
      NExtract::NPathMode::kCurrentPathnames;

  extractCallbackSpec->InitForMulti(false, pathMode, NExtract::NOverwriteMode::kAskBefore);
  extractCallbackSpec->Init(NULL, &_agentSpec->GetArc(),
      extractCallback2,
      false, false, false,
      path,
      pathParts,
      (UInt64)(Int64)-1);
  CUIntVector realIndices;
  GetRealIndices(indices, numItems, realIndices);
  return _agentSpec->GetArchive()->Extract(&realIndices.Front(),
      realIndices.Size(), BoolToInt(false), extractCallback);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::MoveTo(const UInt32 * /* indices */, UInt32 /* numItems */,
    const wchar_t * /* path */, IFolderOperationsExtractCallback * /* callback */)
{
  return E_NOTIMPL;
}

