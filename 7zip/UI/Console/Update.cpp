// Update.cpp

#include "StdAfx.h"

#include "Update.h"

#include "Common/StdOutStream.h"
#include "Common/StringConvert.h"
#include "Common/MyCom.h"

#include "Windows/Defs.h"
#include "Windows/Error.h"
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

#include "TempFiles.h"
#include "ConsoleClose.h"
#include "UpdateCallback.h"

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

static const char *kCreatingArchiveMessage = "Creating archive ";
static const char *kUpdatingArchiveMessage = "Updating archive ";
static const char *kScanningMessage = "Scanning";
static const char *kNoFilesScannedMessage = "No files scanned";
static const char *kTotalFilesAddedMessage = "Total files added to archive: ";

using namespace NWindows;
using namespace NCOM;
using namespace NFile;
using namespace NName;

static LPCTSTR kTempArchiveFilePrefixString = _T("7zi");
static const char *kEverythingIsOk = "Everything is Ok";

static const char *kIllegalFileNameMessage = "Illegal file name for temp archive";

using namespace NUpdateArchive;

static LPCTSTR kTempArcivePrefix = _T("7zi");

static bool ParseNumberString(const UString &srcString, UINT32 &number)
{
  wchar_t *anEndPtr;
  number = wcstoul(srcString, &anEndPtr, 10);
  return (anEndPtr - srcString == srcString.Length());
}


static HRESULT CopyBlock(ISequentialInStream *inStream, ISequentialOutStream *outStream)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, NULL);
}

HRESULT Compress(
    const CActionSet &actionSet, 
    IInArchive *archive,
    const CCompressionMethodMode &compressionMethod,
    const CSysString &archiveName, 
    const CObjectVector<CArchiveItem> &archiveItems,
    const CObjectVector<CDirItem> &dirItems,
    bool enablePercents,
    bool sfxMode,
    const CSysString &sfxModule)
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
    {
      throw "update operations are not supported for this archive";
    }
  }
  else
  {
    #ifndef EXCLUDE_COM

    HRESULT result = loader.CreateHandler(compressionMethod.FilePath, 
        compressionMethod.ClassID1, (void **)&outArchive, true);

    if (result != S_OK)
    {
      throw "update operations are not supported for this archive";
      return E_FAIL;
    }
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


    if (outArchive == 0)
    {
      throw "update operations are not supported for this archive";
      return E_FAIL;
    }
  }
  
  NFileTimeType::EEnum fileTimeType;
  UINT32 value;
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
  
  CObjectVector<CUpdatePair2> operationChain;
  UpdateProduce(dirItems, archiveItems, updatePairs, actionSet,
      operationChain);
  
  CUpdateCallbackImp *updateCallbackSpec = new CUpdateCallbackImp;
  CMyComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  updateCallbackSpec->Init(&dirItems, &archiveItems, &operationChain, enablePercents,
      compressionMethod.PasswordIsDefined, compressionMethod.Password, 
      compressionMethod.AskPassword);
  
  COutFileStream *outStreamSpec = new COutFileStream;
  CMyComPtr<IOutStream> outStream(outStreamSpec);

  {
    CSysString resultPath;
    int pos;
    if(! NFile::NDirectory::MyGetFullPathName(archiveName, resultPath, pos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(resultPath.Left(pos));
  }
  if (!outStreamSpec->Open(archiveName))
  {
    CSysString message;
    NError::MyFormatMessage(::GetLastError(), message);
    g_StdOut << GetOemString(message) << endl;
    return E_FAIL;
  }

  CMyComPtr<ISetProperties> setProperties;
  if (outArchive.QueryInterface(IID_ISetProperties, &setProperties) == S_OK)
  {
    CObjectVector<CMyComBSTR> realNames;
    std::vector<CPropVariant> values;
	  int i;
    for(i = 0; i < compressionMethod.Properties.Size(); i++)
    {
      const CProperty &property = compressionMethod.Properties[i];
      NCOM::CPropVariant propVariant;
      UINT32 number;
      if (!property.Value.IsEmpty())
      {
        if (ParseNumberString(property.Value, number))
          propVariant = number;
        else
          propVariant = property.Value;
      }
      CMyComBSTR comBSTR(property.Name);
      realNames.Add(comBSTR);
      values.push_back(propVariant);
    }
    std::vector<BSTR> names;
    for(i = 0; i < realNames.Size(); i++)
      names.push_back(realNames[i]);
 
    RINOK(setProperties->SetProperties(&names.front(), 
       &values.front(), names.size()));
  }

  if (sfxMode)
  {
    CInFileStream *sfxStreamSpec = new CInFileStream;
    CMyComPtr<IInStream> sfxStream(sfxStreamSpec);
    if (!sfxStreamSpec->Open(sfxModule))
      throw "Can't open sfx module";
    RINOK(CopyBlock(sfxStream, outStream));
  }

  return outArchive->UpdateItems(outStream, operationChain.Size(),
     updateCallback);
}

static void EnumerateDirItems(const NWildcard::CCensorNode &curNode, 
    const CSysString &directory, 
    const UString &prefix, 
    bool checkNameFull,
    CObjectVector<CDirItem> &dirItems, 
    UINT codePage,
    bool enterToSubFolders)
{
  NConsoleClose::CheckCtrlBreak();

  NFind::CEnumerator enumerator(directory + TCHAR(kAnyStringWildcard));
  NFind::CFileInfo fileInfo;
  while (enumerator.Next(fileInfo))
  {
    NConsoleClose::CheckCtrlBreak();
    UString unicodeName = GetUnicodeString(fileInfo.Name, codePage);
    if (checkNameFull)
    {
      if (curNode.CheckNameFull(unicodeName))
        AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, 
            dirItems, codePage);
    }
    else
    {
      if (curNode.CheckNameRecursive(unicodeName))
        AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, 
            dirItems, codePage);
    }
    if (enterToSubFolders && fileInfo.IsDirectory())
    {
      const NWildcard::CCensorNode *nextNode = &curNode;
      if (checkNameFull)
      {
        nextNode = ((NWildcard::CCensorNode *)&curNode)->FindSubNode(unicodeName);
        if (nextNode == NULL)
          nextNode = &curNode;
      }
      EnumerateDirItems(*nextNode,   
          directory + fileInfo.Name + TCHAR(kDirDelimiter), 
          prefix + unicodeName + wchar_t(kDirDelimiter), 
          nextNode != (&curNode), dirItems, codePage, true);
    }
  }
}

static void EnumerateItems(const NWildcard::CCensorNode &curNode, 
    const CSysString &directory, 
    const UString &prefix,
    CObjectVector<CDirItem> &dirItems, 
    UINT codePage)
{
  NConsoleClose::CheckCtrlBreak();
  if (!curNode.GetAllowedRecursedNamesVector(false).IsEmpty() || 
      !curNode.GetAllowedRecursedNamesVector(true).IsEmpty()) 
  {
    EnumerateDirItems(curNode, directory, prefix, true, dirItems, 
        codePage, true);
    return;
  }
  if (!curNode.GetAllowedNamesVector(false, true).IsEmpty())
  {
    EnumerateDirItems(curNode, directory, prefix, true, dirItems, 
        codePage, false);
  }
  else
  {
    const UStringVector &directNames = curNode.GetAllowedNamesVector(false, false);
    for (int i = 0; i < directNames.Size(); i++)
    {
      const UString &nameSpec = directNames[i];
      if (curNode.CheckName(nameSpec, false, false))
        continue;

      NFind::CFileInfo fileInfo;
      if (!NFind::FindFile(directory + GetSystemString(nameSpec, codePage), fileInfo))
        continue;
    
      AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, 
        dirItems, codePage);
    }
  }
  for (int i = 0; i < curNode.SubNodes.Size(); i++)
  {
    const NWildcard::CCensorNode &nextNode = curNode.SubNodes[i];
    EnumerateItems(nextNode, 
        directory + GetSystemString(nextNode.Name, codePage) + TCHAR(kDirDelimiter), 
        prefix + nextNode.Name + wchar_t(kDirDelimiter),
        dirItems, codePage);
  }
}

HRESULT GetFileTime(IInArchive *archive, UINT32 index, 
    FILETIME &fileTime, const FILETIME &defaultFileTime)
{
  CPropVariant property;
  RINOK(archive->GetProperty(index, kpidLastWriteTime, &property));
  if (property.vt == VT_FILETIME)
    fileTime = property.filetime;
  else if (property.vt == VT_EMPTY)
    fileTime = defaultFileTime;
  else
    throw 4190407;
  return S_OK;
}

HRESULT EnumerateInArchiveItems(const NWildcard::CCensor &censor,
    IInArchive *archive,
    const UString &defaultItemName,
    const NWindows::NFile::NFind::CFileInfo &archiveFileInfo,
    CObjectVector<CArchiveItem> &archiveItems)
{
  archiveItems.Clear();
  UINT32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));
  archiveItems.Reserve(numItems);
  for(UINT32 i = 0; i < numItems; i++)
  {
    CArchiveItem archiveItem;
    NCOM::CPropVariant propVariantPath;
    RINOK(archive->GetProperty(i, kpidPath, &propVariantPath));
    UString filePath;
    if(propVariantPath.vt == VT_EMPTY)
      archiveItem.Name = defaultItemName;
    else
    {
      if(propVariantPath.vt != VT_BSTR)
        return E_FAIL;
      archiveItem.Name = propVariantPath.bstrVal;
    }
    archiveItem.Censored = censor.CheckName(archiveItem.Name);

    RINOK(GetFileTime(archive, i, archiveItem.LastWriteTime, 
        archiveFileInfo.LastWriteTime));

    CPropVariant propertySize;
    RINOK(archive->GetProperty(i, kpidSize, &propertySize));
    if (archiveItem.SizeIsDefined = (propertySize.vt != VT_EMPTY))
      archiveItem.Size = ConvertPropVariantToUINT64(propertySize);

    CPropVariant propertyIsFolder;
    RINOK(archive->GetProperty(i, kpidIsFolder, &propertyIsFolder));
    if(propertyIsFolder.vt != VT_BOOL)
      return E_FAIL;
    archiveItem.IsDirectory = VARIANT_BOOLToBool(propertyIsFolder.boolVal);

    archiveItem.IndexInServer = i;
    archiveItems.Add(archiveItem);
  }
  return S_OK;
}


static HRESULT UpdateWithItemLists(
    const CUpdateArchiveOptions &options,
    IInArchive *archive, 
    const CObjectVector<CArchiveItem> &archiveItems,
    const CObjectVector<CDirItem> &dirItems,
    bool enablePercents)
{
  for(int i = 0; i < options.Commands.Size(); i++)
  {
    const CUpdateArchiveCommand &command = options.Commands[i];
    const CSysString &realArchivePath = command.ArchivePath;
    if (i == 0 && options.UpdateArchiveItself)
    {
      if(archive != 0)
        g_StdOut << kUpdatingArchiveMessage;
      else
        g_StdOut << kCreatingArchiveMessage; 
      g_StdOut << GetOemString(options.ArchivePath);
    }
    else
      g_StdOut << kCreatingArchiveMessage << GetOemString(realArchivePath);

    g_StdOut << endl << endl;

    RINOK(Compress(command.ActionSet, archive,
        options.MethodMode, realArchivePath, 
        archiveItems, dirItems, enablePercents,
        options.SfxMode, options.SfxModule));

    g_StdOut << endl;
  }
  return S_OK;
}

HRESULT UpdateArchiveStdMain(const NWildcard::CCensor &censor, 
    CUpdateArchiveOptions  &options, const CSysString &workingDir, 
    IInArchive *archive,
    const UString *defaultItemName,
    const NWindows::NFile::NFind::CFileInfo *archiveFileInfo,
    bool enablePercents)
{
  CObjectVector<CDirItem> dirItems;
  g_StdOut << kScanningMessage;
  EnumerateItems(censor._head, _T(""), L"", dirItems, CP_OEMCP);
  g_StdOut << endl;

  CFileVectorBundle fileVectorBundle;
  if(options.UpdateArchiveItself)
  {
    if (!NDirectory::MyGetTempFileName(workingDir, kTempArchiveFilePrefixString, 
        options.Commands[0].ArchivePath))
      throw "create temp file error";
  }

  int i;
  for(i = 0; i < options.Commands.Size(); i++)
  {
    fileVectorBundle.Add(options.Commands[i].ArchivePath, 
        i > 0 || !options.UpdateArchiveItself);
    // SetBanOnFile(censor,currentDir, options.Commands[i].ArchivePath);
  }
  g_StdOut << endl;

  CObjectVector<CArchiveItem> archiveItems;
  if (archive != NULL)
  {
    RINOK(EnumerateInArchiveItems(censor, 
        archive, *defaultItemName, *archiveFileInfo, archiveItems));
  }

  RINOK(UpdateWithItemLists(options, archive, archiveItems, dirItems, enablePercents));

  if (archive != NULL)
  {
    RINOK(archive->Close());
  }

  int firstNotTempArchiveIndex = options.UpdateArchiveItself ? 1 : 0;
  for(i = options.Commands.Size() - 1; i >= firstNotTempArchiveIndex; i--)
    fileVectorBundle.DisableDeleting(i);
  if(options.UpdateArchiveItself)
  {
    try
    {
      if (archive != NULL)
        if (!NDirectory::DeleteFileAlways(options.ArchivePath))
          throw "delete file error";
      if (!::MoveFile(options.Commands[0].ArchivePath, options.ArchivePath))
        throw "move file error";
    }
    catch(...)
    {
      fileVectorBundle.DisableDeleting(0);
      throw;
    }
  }
  g_StdOut << kEverythingIsOk << endl;
  return S_OK;
}
