// Extract.cpp

#include "StdAfx.h"

#include "../../../../C/Sort.h"

#include "../../../Common/StringConvert.h"

#include "../../../Windows/FileDir.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"

#include "../Common/ExtractingFilePath.h"

#include "Extract.h"
#include "SetProperties.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

static HRESULT DecompressArchive(
    CCodecs *codecs,
    const CArchiveLink &arcLink,
    UInt64 packSize,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    bool calcCrc,
    IExtractCallbackUI *callback,
    CArchiveExtractCallback *ecs,
    UString &errorMessage,
    UInt64 &stdInProcessed)
{
  const CArc &arc = arcLink.Arcs.Back();
  stdInProcessed = 0;
  IInArchive *archive = arc.Archive;
  CRecordVector<UInt32> realIndices;
  
  UStringVector removePathParts;

  FString outDir = options.OutputDir;
  UString replaceName = arc.DefaultName;
  
  if (arcLink.Arcs.Size() > 1)
  {
    // Most "pe" archives have same name of archive subfile "[0]" or ".rsrc_1".
    // So it extracts different archives to one folder.
    // We will use top level archive name
    const CArc &arc0 = arcLink.Arcs[0];
    if (StringsAreEqualNoCase_Ascii(codecs->Formats[arc0.FormatIndex].Name, "pe"))
      replaceName = arc0.DefaultName;
  }

  outDir.Replace(FSTRING_ANY_MASK, us2fs(GetCorrectFsPath(replaceName)));

  bool elimIsPossible = false;
  UString elimPrefix; // only pure name without dir delimiter
  FString outDirReduced = outDir;
  
  if (options.ElimDup.Val)
  {
    UString dirPrefix;
    SplitPathToParts_Smart(fs2us(outDir), dirPrefix, elimPrefix);
    if (!elimPrefix.IsEmpty())
    {
      if (IsCharDirLimiter(elimPrefix.Back()))
        elimPrefix.DeleteBack();
      if (!elimPrefix.IsEmpty())
      {
        outDirReduced = us2fs(dirPrefix);
        elimIsPossible = true;
      }
    }
  }

  if (!options.StdInMode)
  {
    UInt32 numItems;
    RINOK(archive->GetNumberOfItems(&numItems));
    
    UString filePath;

    for (UInt32 i = 0; i < numItems; i++)
    {
      RINOK(arc.GetItemPath(i, filePath));

      if (elimIsPossible && options.ElimDup.Val)
      {
        if (!IsPath1PrefixedByPath2(filePath, elimPrefix))
          elimIsPossible = false;
        else
        {
          wchar_t c = filePath[elimPrefix.Len()];
          if (c != 0 && !IsCharDirLimiter(c))
            elimIsPossible = false;
        }
      }

      bool isFolder;
      RINOK(Archive_IsItem_Folder(archive, i, isFolder));
      bool isAltStream;
      RINOK(Archive_IsItem_AltStream(archive, i, isAltStream));
      if (!options.NtOptions.AltStreams.Val && isAltStream)
        continue;
      if (!wildcardCensor.CheckPath(isAltStream, filePath, !isFolder))
        continue;
      realIndices.Add(i);
    }
    
    if (realIndices.Size() == 0)
    {
      callback->ThereAreNoFiles();
      return callback->ExtractResult(S_OK);
    }
  }

  if (elimIsPossible)
    outDir = outDirReduced;

  #ifdef _WIN32
  // GetCorrectFullFsPath doesn't like "..".
  // outDir.TrimRight();
  // outDir = GetCorrectFullFsPath(outDir);
  #endif

  if (outDir.IsEmpty())
    outDir = FString(FTEXT(".")) + FString(FSTRING_PATH_SEPARATOR);
  else
    if (!CreateComplexDir(outDir))
    {
      HRESULT res = ::GetLastError();
      if (res == S_OK)
        res = E_FAIL;
      errorMessage = ((UString)L"Can not create output directory ") + fs2us(outDir);
      return res;
    }

  ecs->Init(
      options.NtOptions,
      options.StdInMode ? &wildcardCensor : NULL,
      &arc,
      callback,
      options.StdOutMode, options.TestMode,
      outDir,
      removePathParts,
      packSize);

  
  #ifdef SUPPORT_LINKS
  
  if (!options.StdInMode &&
      !options.TestMode &&
      options.NtOptions.HardLinks.Val)
  {
    RINOK(ecs->PrepareHardLinks(&realIndices));
  }
    
  #endif

  
  HRESULT result;
  Int32 testMode = (options.TestMode && !calcCrc) ? 1: 0;
  if (options.StdInMode)
  {
    result = archive->Extract(NULL, (UInt32)(Int32)-1, testMode, ecs);
    NCOM::CPropVariant prop;
    if (archive->GetArchiveProperty(kpidPhySize, &prop) == S_OK)
      ConvertPropVariantToUInt64(prop, stdInProcessed);
  }
  else
    result = archive->Extract(&realIndices.Front(), realIndices.Size(), testMode, ecs);
  if (result == S_OK && !options.StdInMode)
    result = ecs->SetDirsTimes();
  return callback->ExtractResult(result);
}

/* v9.31: BUG was fixed:
   Sorted list for file paths was sorted with case insensitive compare function.
   But FindInSorted function did binary search via case sensitive compare function */

int Find_FileName_InSortedVector(const UStringVector &fileName, const UString &name)
{
  unsigned left = 0, right = fileName.Size();
  while (left != right)
  {
    unsigned mid = (left + right) / 2;
    const UString &midValue = fileName[mid];
    int compare = CompareFileNames(name, midValue);
    if (compare == 0)
      return mid;
    if (compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

HRESULT Extract(
    CCodecs *codecs,
    const CObjectVector<COpenType> &types,
    const CIntVector &excludedFormats,
    UStringVector &arcPaths, UStringVector &arcPathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback,
    #ifndef _SFX
    IHashCalc *hash,
    #endif
    UString &errorMessage,
    CDecompressStat &stat)
{
  stat.Clear();
  UInt64 totalPackSize = 0;
  CRecordVector<UInt64> arcSizes;

  unsigned numArcs = options.StdInMode ? 1 : arcPaths.Size();

  unsigned i;
  for (i = 0; i < numArcs; i++)
  {
    NFind::CFileInfo fi;
    fi.Size = 0;
    if (!options.StdInMode)
    {
      const FString &arcPath = us2fs(arcPaths[i]);
      if (!fi.Find(arcPath))
        throw "there is no such archive";
      if (fi.IsDir())
        throw "can't decompress folder";
    }
    arcSizes.Add(fi.Size);
    totalPackSize += fi.Size;
  }

  CBoolArr skipArcs(numArcs);
  for (i = 0; i < numArcs; i++)
    skipArcs[i] = false;

  CArchiveExtractCallback *ecs = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> ec(ecs);
  bool multi = (numArcs > 1);
  ecs->InitForMulti(multi, options.PathMode, options.OverwriteMode);
  #ifndef _SFX
  ecs->SetHashMethods(hash);
  #endif

  if (multi)
  {
    RINOK(extractCallback->SetTotal(totalPackSize));
  }

  UInt64 totalPackProcessed = 0;
  bool thereAreNotOpenArcs = false;
  
  for (i = 0; i < numArcs; i++)
  {
    if (skipArcs[i])
      continue;

    const UString &arcPath = arcPaths[i];
    NFind::CFileInfo fi;
    if (options.StdInMode)
    {
      fi.Size = 0;
      fi.Attrib = 0;
    }
    else
    {
      if (!fi.Find(us2fs(arcPath)) || fi.IsDir())
        throw "there is no such archive";
    }

    #ifndef _NO_CRYPTO
    openCallback->Open_ClearPasswordWasAskedFlag();
    #endif

    RINOK(extractCallback->BeforeOpen(arcPath));
    CArchiveLink arcLink;

    CObjectVector<COpenType> types2 = types;
    /*
    #ifndef _SFX
    if (types.IsEmpty())
    {
      int pos = arcPath.ReverseFind(L'.');
      if (pos >= 0)
      {
        UString s = arcPath.Ptr(pos + 1);
        int index = codecs->FindFormatForExtension(s);
        if (index >= 0 && s == L"001")
        {
          s = arcPath.Left(pos);
          pos = s.ReverseFind(L'.');
          if (pos >= 0)
          {
            int index2 = codecs->FindFormatForExtension(s.Ptr(pos + 1));
            if (index2 >= 0) // && s.CompareNoCase(L"rar") != 0
            {
              types2.Add(index2);
              types2.Add(index);
            }
          }
        }
      }
    }
    #endif
    */

    COpenOptions op;
    #ifndef _SFX
    op.props = &options.Properties;
    #endif
    op.codecs = codecs;
    op.types = &types2;
    op.excludedFormats = &excludedFormats;
    op.stdInMode = options.StdInMode;
    op.stream = NULL;
    op.filePath = arcPath;
    HRESULT result = arcLink.Open2(op, openCallback);

    if (result == E_ABORT)
      return result;

    bool crypted = false;
    #ifndef _NO_CRYPTO
    crypted = openCallback->Open_WasPasswordAsked();
    #endif

    if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
      result = S_FALSE;

    // arcLink.Set_ErrorsText();
    RINOK(extractCallback->OpenResult(arcPath, result, crypted));


    {
      FOR_VECTOR (r, arcLink.Arcs)
      {
        const CArc &arc = arcLink.Arcs[r];
        const CArcErrorInfo &er = arc.ErrorInfo;
        if (er.IsThereErrorOrWarning())
        {
          RINOK(extractCallback->SetError(r, arc.Path,
              er.GetErrorFlags(), er.ErrorMessage,
              er.GetWarningFlags(), er.WarningMessage));
        }
      }
    }

    if (result != S_OK)
    {
      thereAreNotOpenArcs = true;
      if (!options.StdInMode)
      {
        NFind::CFileInfo fi;
        if (fi.Find(us2fs(arcPath)))
          if (!fi.IsDir())
            totalPackProcessed += fi.Size;
      }
      continue;
    }

    if (!options.StdInMode)
    {
      // numVolumes += arcLink.VolumePaths.Size();
      // arcLink.VolumesSize;

      // totalPackSize -= DeleteUsedFileNamesFromList(arcLink, i + 1, arcPaths, arcPathsFull, &arcSizes);
      // numArcs = arcPaths.Size();
      if (arcLink.VolumePaths.Size() != 0)
      {
        Int64 correctionSize = arcLink.VolumesSize;
        FOR_VECTOR (v, arcLink.VolumePaths)
        {
          int index = Find_FileName_InSortedVector(arcPathsFull, arcLink.VolumePaths[v]);
          if (index >= 0)
          {
            if ((unsigned)index > i)
            {
              skipArcs[index] = true;
              correctionSize -= arcSizes[index];
            }
          }
        }
        if (correctionSize != 0)
        {
          Int64 newPackSize = (Int64)totalPackSize + correctionSize;
          if (newPackSize < 0)
            newPackSize = 0;
          totalPackSize = newPackSize;
          RINOK(extractCallback->SetTotal(totalPackSize));
        }
      }
    }

    #ifndef _NO_CRYPTO
    bool passwordIsDefined;
    UString password;
    RINOK(openCallback->Open_GetPasswordIfAny(passwordIsDefined, password));
    if (passwordIsDefined)
    {
      RINOK(extractCallback->SetPassword(password));
    }
    #endif

    FOR_VECTOR (k, arcLink.Arcs)
    {
      const CArc &arc = arcLink.Arcs[k];
      const CArcErrorInfo &er = arc.ErrorInfo;

      if (er.ErrorFormatIndex >= 0)
      {
        RINOK(extractCallback->OpenTypeWarning(arc.Path,
            codecs->GetFormatNamePtr(arc.FormatIndex),
            codecs->GetFormatNamePtr(er.ErrorFormatIndex)))
        /*
        UString s = L"Can not open the file as [" + codecs->Formats[arc.ErrorFormatIndex].Name + L"] archive\n";
        s += L"The file is open as [" + codecs->Formats[arc.FormatIndex].Name + L"] archive";
        RINOK(extractCallback->MessageError(s));
        */
      }
      {
        const UString &s = er.ErrorMessage;
        if (!s.IsEmpty())
        {
          RINOK(extractCallback->MessageError(s));
        }
      }
    }

    CArc &arc = arcLink.Arcs.Back();
    arc.MTimeDefined = (!options.StdInMode && !fi.IsDevice);
    arc.MTime = fi.MTime;

    UInt64 packProcessed;
    bool calcCrc =
        #ifndef _SFX
          (hash != NULL);
        #else
          false;
        #endif

    RINOK(DecompressArchive(
        codecs,
        arcLink,
        fi.Size + arcLink.VolumesSize,
        wildcardCensor,
        options,
        calcCrc,
        extractCallback, ecs, errorMessage, packProcessed));
    if (!options.StdInMode)
      packProcessed = fi.Size + arcLink.VolumesSize;
    totalPackProcessed += packProcessed;
    ecs->LocalProgressSpec->InSize += packProcessed;
    ecs->LocalProgressSpec->OutSize = ecs->UnpackSize;
    if (!errorMessage.IsEmpty())
      return E_FAIL;
  }

  if (multi || thereAreNotOpenArcs)
  {
    RINOK(extractCallback->SetTotal(totalPackSize));
    RINOK(extractCallback->SetCompleted(&totalPackProcessed));
  }
  stat.NumFolders = ecs->NumFolders;
  stat.NumFiles = ecs->NumFiles;
  stat.NumAltStreams = ecs->NumAltStreams;
  stat.UnpackSize = ecs->UnpackSize;
  stat.AltStreams_UnpackSize = ecs->AltStreams_UnpackSize;
  stat.NumArchives = arcPaths.Size();
  stat.PackSize = ecs->LocalProgressSpec->InSize;
  return S_OK;
}
