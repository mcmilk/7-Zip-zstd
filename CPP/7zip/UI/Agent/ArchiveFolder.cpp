// Agent/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"

#include "../Common/ArchiveExtractCallback.h"

#include "Agent.h"

STDMETHODIMP CAgentFolder::CopyTo(const UInt32 *indices, UInt32 numItems,
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  COM_TRY_BEGIN
  CMyComPtr<IFolderArchiveExtractCallback> extractCallback2;
  {
    CMyComPtr<IFolderOperationsExtractCallback> callbackWrap = callback;
    RINOK(callbackWrap.QueryInterface(IID_IFolderArchiveExtractCallback, &extractCallback2));
  }
  NExtract::NPathMode::EEnum pathMode = _flatMode ?
      NExtract::NPathMode::kNoPathnames :
      NExtract::NPathMode::kCurrentPathnames;
  return Extract(indices,numItems, pathMode, NExtract::NOverwriteMode::kAskBefore,
      path, BoolToInt(false), extractCallback2);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::MoveTo(const UInt32 * /* indices */, UInt32 /* numItems */,
    const wchar_t * /* path */, IFolderOperationsExtractCallback * /* callback */)
{
  return E_NOTIMPL;
}
