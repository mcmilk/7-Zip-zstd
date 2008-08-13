// Extract.cpp

#include "StdAfx.h"

#include "Extract.h"

#include "Windows/Defs.h"
#include "Windows/FileDir.h"

#include "OpenArchive.h"
#include "SetProperties.h"

using namespace NWindows;

static HRESULT DecompressArchive(
    IInArchive *archive,
    UInt64 packSize,
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
  outDir.Replace(L"*", options.DefaultItemName);
  #ifdef _WIN32
  outDir.TrimRight();
  #endif

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
      options.ArchiveFileInfo.MTime,
      options.ArchiveFileInfo.Attrib,
      packSize);

  #ifdef COMPRESS_MT
  RINOK(SetProperties(archive, options.Properties));
  #endif

  HRESULT result = archive->Extract(&realIndices.Front(),
    realIndices.Size(), options.TestMode? 1: 0, extractCallbackSpec);

  return callback->ExtractResult(result);
}

HRESULT DecompressArchives(
    CCodecs *codecs, const CIntVector &formatIndices,
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
    NFile::NFind::CFileInfoW fi;
    if (!NFile::NFind::FindFile(archivePath, fi))
      throw "there is no such archive";
    if (fi.IsDir())
      throw "can't decompress folder";
    archiveSizes.Add(fi.Size);
    totalPackSize += fi.Size;
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
    NFile::NFind::CFileInfoW fi;
    if (!NFile::NFind::FindFile(archivePath, fi))
      throw "there is no such archive";

    if (fi.IsDir())
      throw "there is no such archive";

    options.ArchiveFileInfo = fi;

    #ifndef _NO_CRYPTO
    openCallback->Open_ClearPasswordWasAskedFlag();
    #endif

    RINOK(extractCallback->BeforeOpen(archivePath));
    CArchiveLink archiveLink;

    CIntVector formatIndices2 = formatIndices;
    #ifndef _SFX
    if (formatIndices.IsEmpty())
    {
      int pos = archivePath.ReverseFind(L'.');
      if (pos >= 0)
      {
        UString s = archivePath.Mid(pos + 1);
        int index = codecs->FindFormatForExtension(s);
        if (index >= 0 && s == L"001")
        {
          s = archivePath.Left(pos);
          pos = s.ReverseFind(L'.');
          if (pos >= 0)
          {
            int index2 = codecs->FindFormatForExtension(s.Mid(pos + 1));
            if (index2 >= 0 && s.CompareNoCase(L"rar") != 0)
            {
              formatIndices2.Add(index2);
              formatIndices2.Add(index);
            }
          }
        }
      }
    }
    #endif
    HRESULT result = MyOpenArchive(codecs, formatIndices2, archivePath, archiveLink, openCallback);
    if (result == E_ABORT)
      return result;

    bool crypted = false;
    #ifndef _NO_CRYPTO
    crypted = openCallback->Open_WasPasswordAsked();
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
    RINOK(openCallback->Open_GetPasswordIfAny(password));
    if (!password.IsEmpty())
    {
      RINOK(extractCallback->SetPassword(password));
    }
    #endif

    options.DefaultItemName = archiveLink.GetDefaultItemName();
    RINOK(DecompressArchive(
        archiveLink.GetArchive(),
        fi.Size + archiveLink.VolumesSize,
        wildcardCensor, options, extractCallback, extractCallbackSpec, errorMessage));
    extractCallbackSpec->LocalProgressSpec->InSize += fi.Size +
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
