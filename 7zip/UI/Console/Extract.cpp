// Extract.cpp

#include "StdAfx.h"

#include "Extract.h"
#include "ExtractCallback.h"
#include "ArError.h"

#include "Common/StdOutStream.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"

using namespace NWindows;

static const char *kEverythingIsOk = "Everything is Ok";

HRESULT DeCompressArchiveSTD(
    IInArchive *archive,
    const NWildcard::CCensor &wildcardCensor,
    const CExtractOptions &options)
{
  CRecordVector<UINT32> realIndices;
  UINT32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));

  for(UINT32 i = 0; i < numItems; i++)
  {
    NCOM::CPropVariant propVariant;
    RINOK(archive->GetProperty(i, kpidPath, &propVariant));
    UString filePath;
    if(propVariant.vt == VT_EMPTY)
      filePath = options.DefaultItemName;
    else
    {
      if(propVariant.vt != VT_BSTR)
        return E_FAIL;
      filePath = propVariant.bstrVal;
    }
    if (!wildcardCensor.CheckName(filePath))
      continue;
    realIndices.Add(i);
  }
  if (realIndices.Size() == 0)
  {
    g_StdOut << endl << "No files to process" << endl;
    return S_OK;
  }

  CExtractCallbackImp *extractCallbackSpec = new CExtractCallbackImp;
  CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
  
  UStringVector removePathParts;

  NExtraction::CInfo extractionInfo;
  extractionInfo.PathMode = options.FullPathMode() ? NExtraction::NPathMode::kFullPathnames:
      NExtraction::NPathMode::kNoPathnames;

  if (options.YesToAll)
    extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kWithoutPrompt;
  else
  {
    extractionInfo.OverwriteMode = options.OverwriteMode;
  }

  if(!options.OutputBaseDir.IsEmpty())
    if(!NFile::NDirectory::CreateComplexDirectory(options.OutputBaseDir))
    {
      throw "Can not create output directory";
    }

  extractCallbackSpec->Init(archive, 
      options.OutputBaseDir, extractionInfo, removePathParts, CP_OEMCP,
      options.DefaultItemName, 
      options.ArchiveFileInfo.LastWriteTime,
      options.ArchiveFileInfo.Attributes,
      options.PasswordEnabled, 
      options.Password);

  HRESULT result = archive->Extract(&realIndices.Front(), 
      realIndices.Size(), options.ExtractMode == NExtractMode::kTest, 
      extractCallback);

  if (extractCallbackSpec->m_NumErrors != 0)
    throw NExitCode::CMultipleErrors(extractCallbackSpec->m_NumErrors);

  if (result != S_OK)
    throw NExitCode::CSystemError(result);

  g_StdOut << endl << kEverythingIsOk << endl;

  return S_OK;
}
