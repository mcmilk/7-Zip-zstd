// Extract.cpp

#include "StdAfx.h"

#include "Extract.h"

#include "Windows/Defs.h"
#include "Windows/FileDir.h"

#include "OpenArchive.h"
#include "SetProperties.h"

#ifndef EXCLUDE_COM
#include "Windows/DLL.h"
#endif

using namespace NWindows;

HRESULT DecompressArchive(
    IInArchive *archive,
    const UString &defaultName,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    IExtractCallbackUI *callback)
{
  CRecordVector<UInt32> realIndices;
  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));

  for(UInt32 i = 0; i < numItems; i++)
  {
    UString filePath;
    RINOK(GetArchiveItemPath(archive, i, options.DefaultItemName, filePath));
    bool isFolder;
    RINOK(IsArchiveItemFolder(archive, i, isFolder));
    if (!wildcardCensor.CheckPath(filePath, !isFolder))
      continue;
    realIndices.Add(i);
  }
  if (realIndices.Size() == 0)
  {
    callback->ThereAreNoFiles();
    return S_OK;
  }

  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
  
  UStringVector removePathParts;

  UString outDir = options.OutputDir;
  outDir.Replace(L"*", defaultName);
  if(!outDir.IsEmpty())
    if(!NFile::NDirectory::CreateComplexDirectory(outDir))
    {
      throw UString(L"Can not create output directory ") + outDir;
    }

  extractCallbackSpec->Init(
      archive, 
      callback,
      options.StdOutMode,
      outDir, 
      options.PathMode, 
      options.OverwriteMode,
      removePathParts, 
      options.DefaultItemName, 
      options.ArchiveFileInfo.LastWriteTime,
      options.ArchiveFileInfo.Attributes);

  #ifdef COMPRESS_MT
  RINOK(SetProperties(archive, options.Properties));
  #endif

  HRESULT result = archive->Extract(&realIndices.Front(), 
    realIndices.Size(), options.TestMode? 1: 0, 
      extractCallback);

  return callback->ExtractResult(result);
}

HRESULT DecompressArchives(
    UStringVector &archivePaths, UStringVector &archivePathsFull,    
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &optionsSpec,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback)
{
  CExtractOptions options = optionsSpec;
  for (int i = 0; i < archivePaths.Size(); i++)
  {
    const UString &archivePath = archivePaths[i];
    NFile::NFind::CFileInfoW archiveFileInfo;
    if (!NFile::NFind::FindFile(archivePath, archiveFileInfo))
      throw "there is no such archive";

    if (archiveFileInfo.IsDirectory())
      throw "there is no such archive";

    options.ArchiveFileInfo = archiveFileInfo;

    RINOK(extractCallback->BeforeOpen(archivePath));
    CArchiveLink archiveLink;
    HRESULT result = MyOpenArchive(archivePath, archiveLink, openCallback);
    RINOK(extractCallback->OpenResult(archivePath, result));
    if (result != S_OK)
      continue;

    for (int v = 0; v < archiveLink.VolumePaths.Size(); v++)
    {
      int index = archivePathsFull.FindInSorted(archiveLink.VolumePaths[v]);
      if (index >= 0 && index > i)
      {
        archivePaths.Delete(index);
        archivePathsFull.Delete(index);
      }
    }

    #ifndef _NO_CRYPTO
    UString password;
    RINOK(openCallback->GetPasswordIfAny(password));
    if (!password.IsEmpty())
    {
      RINOK(extractCallback->SetPassword(password));
    }
    #endif

    options.DefaultItemName = archiveLink.GetDefaultItemName();
    RINOK(DecompressArchive(
        archiveLink.GetArchive(), archiveLink.GetDefaultItemName(),
        wildcardCensor, options, extractCallback));
  }
  return S_OK;
}
