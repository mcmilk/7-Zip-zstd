// Update.cpp

#include "StdAfx.h"

#include <mapi.h>

#include "Update.h"

#include "Common/IntToString.h"
#include "Common/StringToInt.h"
#include "Common/StringConvert.h"
#include "Common/CommandLineParser.h"

#ifdef _WIN32
#include "Windows/DLL.h"
#endif

#include "Windows/Defs.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "../../Common/FileStreams.h"
#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/DirItem.h"
#include "../Common/EnumDirItems.h"
#include "../Common/UpdateProduce.h"
#include "../Common/OpenArchive.h"

#include "TempFiles.h"
#include "UpdateCallback.h"
#include "EnumDirItems.h"

#ifdef FORMAT_7Z
#include "../../Archive/7z/7zHandler.h"
#endif

#ifdef FORMAT_BZIP2
#include "../../Archive/BZip2/BZip2Handler.h"
#endif

#ifdef FORMAT_GZIP
#include "../../Archive/GZip/GZipHandler.h"
#endif

#ifdef FORMAT_TAR
#include "../../Archive/Tar/TarHandler.h"
#endif

#ifdef FORMAT_ZIP
#include "../../Archive/Zip/ZipHandler.h"
#endif

#ifndef EXCLUDE_COM
#include "../Common/HandlerLoader.h"
#endif

static const char *kUpdateIsNotSupoorted = 
  "update operations are not supported for this archive";

using namespace NCommandLineParser;
using namespace NWindows;
using namespace NCOM;
using namespace NFile;
using namespace NName;

static const wchar_t *kTempArchiveFilePrefixString = L"7zi";
static const wchar_t *kTempFolderPrefix = L"7zE";

static const char *kIllegalFileNameMessage = "Illegal file name for temp archive";

using namespace NUpdateArchive;

static void ParseNumberString(const UString &s, NCOM::CPropVariant &prop)
{
  const wchar_t *endPtr;
  UInt64 result = ConvertStringToUInt64(s, &endPtr);
  if (endPtr - (const wchar_t *)s != s.Length())
    prop = s;
  else if (result <= 0xFFFFFFFF)
    prop = (UInt32)result;
  else 
    prop = result;
}

static HRESULT CopyBlock(ISequentialInStream *inStream, ISequentialOutStream *outStream)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

static HRESULT Compress(
    const CActionSet &actionSet, 
    IInArchive *archive,
    const CCompressionMethodMode &compressionMethod,
    CArchivePath &archivePath, 
    const CObjectVector<CArchiveItem> &archiveItems,
    bool stdInMode,
    const UString &stdInFileName,
    bool stdOutMode,
    const CObjectVector<CDirItem> &dirItems,
    bool sfxMode,
    const UString &sfxModule,
    const CRecordVector<UInt64> &volumesSizes,
    CTempFiles &tempFiles,
    CUpdateErrorInfo &errorInfo,
    IUpdateCallbackUI *callback)
{
  #ifndef EXCLUDE_COM
  CHandlerLoader loader;
  #endif

  CMyComPtr<IOutArchive> outArchive;
  if(archive != NULL)
  {
    CMyComPtr<IInArchive> archive2 = archive;
    HRESULT result = archive2.QueryInterface(IID_IOutArchive, &outArchive);
    if(result != S_OK)
      throw kUpdateIsNotSupoorted;
  }
  else
  {
    #ifndef EXCLUDE_COM

    if (loader.CreateHandler(compressionMethod.FilePath, 
        compressionMethod.ClassID, (void **)&outArchive, true) != S_OK)
      throw kUpdateIsNotSupoorted;
    #endif

    #ifdef FORMAT_7Z
    if (compressionMethod.Name.CompareNoCase(L"7z") == 0)
      outArchive = new NArchive::N7z::CHandler;
    #endif

    #ifdef FORMAT_BZIP2
    if (compressionMethod.Name.CompareNoCase(L"BZip2") == 0)
      outArchive = new NArchive::NBZip2::CHandler;
    #endif

    #ifdef FORMAT_GZIP
    if (compressionMethod.Name.CompareNoCase(L"GZip") == 0)
      outArchive = new NArchive::NGZip::CHandler;
    #endif

    #ifdef FORMAT_TAR
    if (compressionMethod.Name.CompareNoCase(L"Tar") == 0)
      outArchive = new NArchive::NTar::CHandler;
    #endif
    
    #ifdef FORMAT_ZIP
    if (compressionMethod.Name.CompareNoCase(L"Zip") == 0)
      outArchive = new NArchive::NZip::CHandler;
    #endif

  }
  if (outArchive == 0)
    throw kUpdateIsNotSupoorted;
  
  NFileTimeType::EEnum fileTimeType;
  UInt32 value;
  RINOK(outArchive->GetFileTimeType(&value));

  switch(value)
  {
    case NFileTimeType::kWindows:
    case NFileTimeType::kDOS:
    case NFileTimeType::kUnix:
      fileTimeType = NFileTimeType::EEnum(value);
      break;
    default:
      return E_FAIL;
  }

  CObjectVector<CUpdatePair> updatePairs;
  GetUpdatePairInfoList(dirItems, archiveItems, fileTimeType, updatePairs); // must be done only once!!!
  
  CObjectVector<CUpdatePair2> updatePairs2;
  UpdateProduce(dirItems, archiveItems, updatePairs, actionSet, updatePairs2);
  
  CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
  CMyComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  updateCallbackSpec->StdInMode = stdInMode;
  updateCallbackSpec->Callback = callback;
  updateCallbackSpec->DirItems = &dirItems;
  updateCallbackSpec->ArchiveItems = &archiveItems;
  updateCallbackSpec->UpdatePairs = &updatePairs2;

  CMyComPtr<ISequentialOutStream> outStream;

  const UString &archiveName = archivePath.GetFinalPath();
  if (!stdOutMode)
  {
    UString resultPath;
    int pos;
    if(!NFile::NDirectory::MyGetFullPathName(archiveName, resultPath, pos))
      throw 1417161;
    NFile::NDirectory::CreateComplexDirectory(resultPath.Left(pos));
  }
  if (volumesSizes.Size() == 0)
  {
    if (stdOutMode)
      outStream = new CStdOutFileStream;
    else
    {
      COutFileStream *outStreamSpec = new COutFileStream;
      outStream = outStreamSpec;
      bool isOK = false;
      UString realPath;
      for (int i = 0; i < (1 << 16); i++)
      {
        if (archivePath.Temp)
        {
          if (i > 0)
          {
            wchar_t s[32];
            ConvertUInt64ToString(i, s);
            archivePath.TempPostfix = s;
          }
          realPath = archivePath.GetTempPath();
        }
        else
          realPath = archivePath.GetFinalPath();
        if (outStreamSpec->Create(realPath, false))
        {
          tempFiles.Paths.Add(realPath);
          isOK = true;
          break;
        }
        if (::GetLastError() != ERROR_FILE_EXISTS)
          break;
        if (!archivePath.Temp)
          break;
      }
      if (!isOK)
      {
        errorInfo.SystemError = ::GetLastError();
        errorInfo.FileName = realPath;
        errorInfo.Message = L"Can not open file";
        return E_FAIL;
      }
    }
  }
  else
  {
    updateCallbackSpec->VolumesSizes = volumesSizes;
    updateCallbackSpec->VolName = archivePath.Prefix + archivePath.Name;
    if (!archivePath.VolExtension.IsEmpty())
      updateCallbackSpec->VolExt = UString(L'.') + archivePath.VolExtension;
  }

  CMyComPtr<ISetProperties> setProperties;
  if (outArchive.QueryInterface(IID_ISetProperties, &setProperties) == S_OK)
  {
    UStringVector realNames;
    std::vector<CPropVariant> values;
	  int i;
    for(i = 0; i < compressionMethod.Properties.Size(); i++)
    {
      const CProperty &property = compressionMethod.Properties[i];
      NCOM::CPropVariant propVariant;
      if (!property.Value.IsEmpty())
        ParseNumberString(property.Value, propVariant);
      realNames.Add(property.Name);
      values.push_back(propVariant);
    }
    CRecordVector<const wchar_t *> names;
    for(i = 0; i < realNames.Size(); i++)
      names.Add((const wchar_t *)realNames[i]);
 
    RINOK(setProperties->SetProperties(&names.Front(), 
       &values.front(), names.Size()));
  }

  if (sfxMode)
  {
    CInFileStream *sfxStreamSpec = new CInFileStream;
    CMyComPtr<IInStream> sfxStream(sfxStreamSpec);
    if (!sfxStreamSpec->Open(sfxModule))
    {
      errorInfo.SystemError = ::GetLastError();
      errorInfo.Message = L"Can't open sfx module";
      errorInfo.FileName = sfxModule;
      return E_FAIL;
    }
    RINOK(CopyBlock(sfxStream, outStream));
  }

  HRESULT result = outArchive->UpdateItems(outStream, updatePairs2.Size(),
     updateCallback);
  callback->Finilize();
  return result;
}



HRESULT EnumerateInArchiveItems(const NWildcard::CCensor &censor,
    IInArchive *archive,
    const UString &defaultItemName,
    const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
    CObjectVector<CArchiveItem> &archiveItems)
{
  archiveItems.Clear();
  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));
  archiveItems.Reserve(numItems);
  for(UInt32 i = 0; i < numItems; i++)
  {
    CArchiveItem ai;

    RINOK(GetArchiveItemPath(archive, i, defaultItemName, ai.Name));
    RINOK(IsArchiveItemFolder(archive, i, ai.IsDirectory));
    ai.Censored = censor.CheckPath(ai.Name, !ai.IsDirectory);
    RINOK(GetArchiveItemFileTime(archive, i, 
        archiveFileInfo.LastWriteTime, ai.LastWriteTime));

    CPropVariant propertySize;
    RINOK(archive->GetProperty(i, kpidSize, &propertySize));
    if (ai.SizeIsDefined = (propertySize.vt != VT_EMPTY))
      ai.Size = ConvertPropVariantToUINT64(propertySize);

    ai.IndexInServer = i;
    archiveItems.Add(ai);
  }
  return S_OK;
}


static HRESULT UpdateWithItemLists(
    CUpdateOptions &options,
    IInArchive *archive, 
    const CObjectVector<CArchiveItem> &archiveItems,
    const CObjectVector<CDirItem> &dirItems,
    CTempFiles &tempFiles,
    CUpdateErrorInfo &errorInfo,
    IUpdateCallbackUI2 *callback)
{
  for(int i = 0; i < options.Commands.Size(); i++)
  {
    CUpdateArchiveCommand &command = options.Commands[i];
    if (options.StdOutMode)
    {
      RINOK(callback->StartArchive(0, archive != 0));
    }
    else
    {
      RINOK(callback->StartArchive(command.ArchivePath.GetFinalPath(), 
          i == 0 && options.UpdateArchiveItself && archive != 0));
    }

    RINOK(Compress(command.ActionSet, archive,
        options.MethodMode, 
        command.ArchivePath, 
        archiveItems, 
        options.StdInMode, options.StdInFileName, 
        options.StdOutMode,
        dirItems, 
        options.SfxMode, options.SfxModule, 
        options.VolumesSizes,
        tempFiles,
        errorInfo, callback));

    RINOK(callback->FinishArchive());
  }
  return S_OK;
}

class CCurrentDirRestorer
{
  CSysString m_CurrentDirectory;
public:
  CCurrentDirRestorer()
    { NFile::NDirectory::MyGetCurrentDirectory(m_CurrentDirectory); }
  ~CCurrentDirRestorer()
    { RestoreDirectory();}
  bool RestoreDirectory()
    { return BOOLToBool(::SetCurrentDirectory(m_CurrentDirectory)); }
};

struct CEnumDirItemUpdateCallback: public IEnumDirItemCallback
{
  IUpdateCallbackUI2 *Callback;
  HRESULT CheckBreak() { return Callback->CheckBreak(); }
};

HRESULT UpdateArchive(const NWildcard::CCensor &censor, 
    CUpdateOptions &options,
    CUpdateErrorInfo &errorInfo,
    IOpenCallbackUI *openCallback,
    IUpdateCallbackUI2 *callback)
{
  if (options.StdOutMode && options.EMailMode)
    return E_FAIL;

  if (options.SfxMode)
  {
    CProperty property;
    property.Name = L"rsfx";
    property.Value = L"on";
    options.MethodMode.Properties.Add(property);
    if (options.SfxModule.IsEmpty())
    {
      errorInfo.Message = L"sfx file is not specified";
      return E_FAIL;
    }
    UString name = options.SfxModule;
    if (!NDirectory::MySearchPath(NULL, name, NULL, options.SfxModule))
    {
      errorInfo.Message = L"can't find specified sfx module";
      return E_FAIL;
    }
  }

  const UString archiveName = options.ArchivePath.GetFinalPath();

  UString defaultItemName;
  NFind::CFileInfoW archiveFileInfo;

  CArchiveLink archiveLink;
  IInArchive *archive = 0;
  if (NFind::FindFile(archiveName, archiveFileInfo))
  {
    if (archiveFileInfo.IsDirectory())
      throw "there is no such archive";
    HRESULT result = MyOpenArchive(archiveName, archiveLink, openCallback);
    RINOK(callback->OpenResult(archiveName, result));
    RINOK(result);
    if (archiveLink.VolumePaths.Size() > 1)
    {
      errorInfo.SystemError = E_NOTIMPL;
      errorInfo.Message = L"Updating for multivolume archives is not implemented";
      return E_NOTIMPL;
    }
    archive = archiveLink.GetArchive();
    defaultItemName = archiveLink.GetDefaultItemName();
  }
  else
  {
    /*
    if (archiveType.IsEmpty())
      throw "type of archive is not specified";
    */
  }

  CObjectVector<CDirItem> dirItems;
  if (options.StdInMode)
  {
    CDirItem item;
    item.FullPath = item.Name = options.StdInFileName;
    item.Size = (UInt64)(Int64)-1;
    item.Attributes = 0;
    SYSTEMTIME st;
    FILETIME ft;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    item.CreationTime = item.LastAccessTime = item.LastWriteTime = ft;
    dirItems.Add(item);
  }
  else
  {
    bool needScanning = false;
    for(int i = 0; i < options.Commands.Size(); i++)
      if (options.Commands[i].ActionSet.NeedScanning())
        needScanning = true;
    if (needScanning)
    {
      CEnumDirItemUpdateCallback enumCallback;
      enumCallback.Callback = callback;
      RINOK(callback->StartScanning());
      RINOK(EnumerateItems(censor, dirItems, &enumCallback));
      RINOK(callback->FinishScanning());
    }
  }

  if (options.VolumesSizes.Size() > 0 && archive)
    return E_NOTIMPL;

  UString tempDirPrefix;
  bool usesTempDir = false;
  NDirectory::CTempDirectoryW tempDirectory;
  if (options.EMailMode && options.EMailRemoveAfter)
  {
    tempDirectory.Create(kTempFolderPrefix);
    tempDirPrefix = tempDirectory.GetPath();
    NormalizeDirPathPrefix(tempDirPrefix);
    usesTempDir = true;
  }

  CTempFiles tempFiles;

  bool createTempFile = false;
  if(!options.StdOutMode && options.UpdateArchiveItself)
  {
    CArchivePath &ap = options.Commands[0].ArchivePath;
    ap = options.ArchivePath;
    if (archive != 0 && !usesTempDir)
    {
      createTempFile = true;
      ap.Temp = true;
      if (!options.WorkingDir.IsEmpty())
      {
        ap.TempPrefix = options.WorkingDir;
        NormalizeDirPathPrefix(ap.TempPrefix);
      }
    }
  }

  for(int i = 0; i < options.Commands.Size(); i++)
  {
    CArchivePath &ap = options.Commands[i].ArchivePath;
    if (usesTempDir)
    {
      // Check it
      ap.Prefix = tempDirPrefix;
      // ap.Temp = true;
      // ap.TempPrefix = tempDirPrefix;
    }
    if (i > 0 || !createTempFile)
    {
      const UString &path = ap.GetFinalPath();
      if (NFind::DoesFileExist(path))
      {
        errorInfo.SystemError = 0;
        errorInfo.Message = L"File already exists";
        errorInfo.FileName = path;
        return E_FAIL;
      }
    }
  }

  CObjectVector<CArchiveItem> archiveItems;
  if (archive != NULL)
  {
    RINOK(EnumerateInArchiveItems(censor, 
        archive, *defaultItemName, archiveFileInfo, archiveItems));
  }

  RINOK(UpdateWithItemLists(options, archive, archiveItems, dirItems, 
      tempFiles, errorInfo, callback));

  if (archive != NULL)
  {
    RINOK(archiveLink.Close());
    archiveLink.Release();
  }

  tempFiles.Paths.Clear();
  if(createTempFile)
  {
    try
    {
      CArchivePath &ap = options.Commands[0].ArchivePath;
      const UString &tempPath = ap.GetTempPath();
      if (archive != NULL)
        if (!NDirectory::DeleteFileAlways(archiveName))
        {
          errorInfo.SystemError = ::GetLastError();
          errorInfo.Message = L"delete file error";
          errorInfo.FileName = archiveName;
          return E_FAIL;
        }
      if (!NDirectory::MyMoveFile(tempPath, archiveName))
      {
        errorInfo.SystemError = ::GetLastError();
        errorInfo.Message = L"move file error";
        errorInfo.FileName = tempPath;
        errorInfo.FileName2 = archiveName;
        return E_FAIL;
      }
    }
    catch(...)
    {
      throw;
    }
  }

  #ifdef _WIN32
  if (options.EMailMode)
  {
    NDLL::CLibrary mapiLib;
    if (!mapiLib.Load(TEXT("Mapi32.dll")))
    {
      errorInfo.SystemError = ::GetLastError();
      errorInfo.Message = L"can not load Mapi32.dll";
      return E_FAIL;
    }
    LPMAPISENDDOCUMENTS fnSend = (LPMAPISENDDOCUMENTS)
        mapiLib.GetProcAddress("MAPISendDocuments");
    if (fnSend == 0)
    {
      errorInfo.SystemError = ::GetLastError();
      errorInfo.Message = L"can not find MAPISendDocuments function";
      return E_FAIL;
    }
    UStringVector fullPaths;
    int i;
    for(i = 0; i < options.Commands.Size(); i++)
    {
      CArchivePath &ap = options.Commands[i].ArchivePath;
      UString arcPath;
      if(!NFile::NDirectory::MyGetFullPathName(ap.GetFinalPath(), arcPath))
      {
        errorInfo.SystemError = ::GetLastError();
        return E_FAIL;
      }
      fullPaths.Add(arcPath);
    }
    CCurrentDirRestorer curDirRestorer;
    for(i = 0; i < fullPaths.Size(); i++)
    {
      UString arcPath = fullPaths[i];
      UString fileName = ExtractFileNameFromPath(arcPath);
      AString path = GetAnsiString(arcPath);
      AString name = GetAnsiString(fileName);
      // Warning!!! MAPISendDocuments function changes Current directory
      fnSend(0, ";", (LPSTR)(LPCSTR)path, (LPSTR)(LPCSTR)name, 0); 
    }
  }
  #endif
  return S_OK;
}

