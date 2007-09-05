// Extract.cpp

#include "StdAfx.h"

#include "Extract.h"

#include "Windows/Defs.h"
#include "Windows/FileDir.h"

#include "OpenArchive.h"
#include "SetProperties.h"

using namespace NWindows;

HRESULT DecompressArchive(
    IInArchive *archive,
    UInt64 packSize,
    const UString &defaultName,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    IExtractCallbackUI *callback,
    CArchiveExtractCallback *extractCallbackSpec,
    UString &errorMessage)
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

  UStringVector removePathParts;

  UString outDir = options.OutputDir;
  outDir.Replace(L"*", defaultName);
  if(!outDir.IsEmpty())
    if(!NFile::NDirectory::CreateComplexDirectory(outDir))
    {
      HRESULT res = ::GetLastError();
      if (res == S_OK)
        res = E_FAIL;
      errorMessage = ((UString)L"Can not create output directory ") + outDir;
      return res;
    }

  extractCallbackSpec->Init(
      archive, 
      callback,
      options.StdOutMode,
      outDir, 
      removePathParts, 
      options.DefaultItemName, 
      options.ArchiveFileInfo.LastWriteTime,
      options.ArchiveFileInfo.Attributes,
      packSize);

  #ifdef COMPRESS_MT
  RINOK(SetProperties(archive, options.Properties));
  #endif

  HRESULT result = archive->Extract(&realIndices.Front(), 
    realIndices.Size(), options.TestMode? 1: 0, extractCallbackSpec);

  return callback->ExtractResult(result);
}

HRESULT DecompressArchives(
    CCodecs *codecs,
    UStringVector &archivePaths, UStringVector &archivePathsFull,    
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &optionsSpec,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback, 
    UString &errorMessage, 
    CDecompressStat &stat)
{
  stat.Clear();
  CExtractOptions options = optionsSpec;
  int i;
  UInt64 totalPackSize = 0;
  CRecordVector<UInt64> archiveSizes;
  for (i = 0; i < archivePaths.Size(); i++)
  {
    const UString &archivePath = archivePaths[i];
    NFile::NFind::CFileInfoW archiveFileInfo;
    if (!NFile::NFind::FindFile(archivePath, archiveFileInfo))
      throw "there is no such archive";
    if (archiveFileInfo.IsDirectory())
      throw "can't decompress folder";
    archiveSizes.Add(archiveFileInfo.Size);
    totalPackSize += archiveFileInfo.Size;
  }
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> ec(extractCallbackSpec);
  bool multi = (archivePaths.Size() > 1);
  extractCallbackSpec->InitForMulti(multi, options.PathMode, options.OverwriteMode);
  if (multi)
  {
    RINOK(extractCallback->SetTotal(totalPackSize));  
  }
  for (i = 0; i < archivePaths.Size(); i++)
  {
    const UString &archivePath = archivePaths[i];
    NFile::NFind::CFileInfoW archiveFileInfo;
    if (!NFile::NFind::FindFile(archivePath, archiveFileInfo))
      throw "there is no such archive";

    if (archiveFileInfo.IsDirectory())
      throw "there is no such archive";

    options.ArchiveFileInfo = archiveFileInfo;

    #ifndef _NO_CRYPTO
    openCallback->ClearPasswordWasAskedFlag();
    #endif

    RINOK(extractCallback->BeforeOpen(archivePath));
    CArchiveLink archiveLink;
    HRESULT result = MyOpenArchive(codecs, archivePath, archiveLink, openCallback);

    bool crypted = false;
    #ifndef _NO_CRYPTO
    crypted = openCallback->WasPasswordAsked();
    #endif

    RINOK(extractCallback->OpenResult(archivePath, result, crypted));
    if (result != S_OK)
      continue;

    for (int v = 0; v < archiveLink.VolumePaths.Size(); v++)
    {
      int index = archivePathsFull.FindInSorted(archiveLink.VolumePaths[v]);
      if (index >= 0 && index > i)
      {
        archivePaths.Delete(index);
        archivePathsFull.Delete(index);
        totalPackSize -= archiveSizes[index];
        archiveSizes.Delete(index);
      }
    }
    if (archiveLink.VolumePaths.Size() != 0)
    {
      totalPackSize += archiveLink.VolumesSize;
      RINOK(extractCallback->SetTotal(totalPackSize));  
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
        archiveLink.GetArchive(), 
        archiveFileInfo.Size + archiveLink.VolumesSize,
        archiveLink.GetDefaultItemName(),
        wildcardCensor, options, extractCallback, extractCallbackSpec, errorMessage));
    extractCallbackSpec->LocalProgressSpec->InSize += archiveFileInfo.Size + 
        archiveLink.VolumesSize;
    extractCallbackSpec->LocalProgressSpec->OutSize = extractCallbackSpec->UnpackSize;
    if (!errorMessage.IsEmpty())
      return E_FAIL;
  }
  stat.NumFolders = extractCallbackSpec->NumFolders;
  stat.NumFiles = extractCallbackSpec->NumFiles;
  stat.UnpackSize = extractCallbackSpec->UnpackSize;
  stat.NumArchives = archivePaths.Size();
  stat.PackSize = extractCallbackSpec->LocalProgressSpec->InSize;
  return S_OK;
}
