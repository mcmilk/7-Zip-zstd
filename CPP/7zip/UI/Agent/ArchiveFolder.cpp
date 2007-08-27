// Zip/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/FileDir.h"

#include "../../Common/FileStreams.h"

#include "../Common/UpdatePair.h"
#include "../Common/ArchiveExtractCallback.h"

#include "Agent.h"

using namespace NWindows;
using namespace NCOM;

STDMETHODIMP CAgentFolder::CopyTo(const UINT32 *indices, UINT32 numItems, 
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
  extractCallbackSpec->Init(_agentSpec->GetArchive(), 
      extractCallback2, 
      false,
      path,
      pathParts, 
      _agentSpec->DefaultName,
      _agentSpec->DefaultTime, 
      _agentSpec->DefaultAttributes,
      (UInt64)(Int64)-1

      // ,_agentSpec->_srcDirectoryPrefix
      );
  CUIntVector realIndices;
  GetRealIndices(indices, numItems, realIndices);
  return _agentSpec->GetArchive()->Extract(&realIndices.Front(), 
      realIndices.Size(), BoolToInt(false), extractCallback);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::MoveTo(const UINT32 * /* indices */, UINT32 /* numItems */, 
    const wchar_t * /* path */, IFolderOperationsExtractCallback * /* callback */)
{
  return E_NOTIMPL;
}

