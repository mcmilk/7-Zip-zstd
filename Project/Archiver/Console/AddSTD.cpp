// AddStd.cpp

#include "StdAfx.h"

#include "AddSTD.h"

#include "Common/StdOutStream.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"

#include "../Common/ArchiveStyleDirItemInfo.h"
#include "../Common/CompressEngineCommon.h"

#include "Windows\PropVariantConversions.h"

#include "CompressEngine.h"
#include "FileCreationUtils.h"

#include "ConsoleCloseUtils.h"

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

static void EnumerateDirItems(const NWildcard::CCensorNode &curNode, 
    const CSysString &directory, 
    const UString &prefix, 
    bool checkNameFull,
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, 
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
            dirFileInfoVector, codePage);
    }
    else
    {
      if (curNode.CheckNameRecursive(unicodeName))
        AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, 
            dirFileInfoVector, codePage);
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
          nextNode != (&curNode), dirFileInfoVector, codePage, true);
    }
  }
}

static void EnumerateItems(const NWildcard::CCensorNode &curNode, 
    const CSysString &directory, 
    const UString &prefix,
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, 
    UINT codePage)
{
  NConsoleClose::CheckCtrlBreak();
  if (!curNode.GetAllowedRecursedNamesVector(false).IsEmpty() || 
      !curNode.GetAllowedRecursedNamesVector(true).IsEmpty()) 
  {
    EnumerateDirItems(curNode, directory, prefix, true, dirFileInfoVector, 
        codePage, true);
    return;
  }
  if (!curNode.GetAllowedNamesVector(false, true).IsEmpty())
  {
    EnumerateDirItems(curNode, directory, prefix, true, dirFileInfoVector, 
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
        dirFileInfoVector, codePage);
    }
  }
  for (int i = 0; i < curNode.SubNodes.Size(); i++)
  {
    const NWildcard::CCensorNode &nextNode = curNode.SubNodes[i];
    EnumerateItems(nextNode, 
        directory + GetSystemString(nextNode.Name, codePage) + TCHAR(kDirDelimiter), 
        prefix + nextNode.Name + wchar_t(kDirDelimiter),
        dirFileInfoVector, codePage);
  }
}

HRESULT GetFileTime(IArchiveHandler200 *archive, UINT32 index, 
    FILETIME &fileTime, const FILETIME &defaultFileTime)
{
  CPropVariant property;
  RETURN_IF_NOT_S_OK(archive->GetProperty(index, kpidLastWriteTime, &property));
  if (property.vt == VT_FILETIME)
    fileTime = property.filetime;
  else if (property.vt == VT_EMPTY)
    fileTime = defaultFileTime;
  else
    throw 4190407;
  return S_OK;
}

HRESULT EnumerateInArchiveItems(const NWildcard::CCensor &censor,
    IArchiveHandler200 *archive,
    const UString &defaultItemName,
    const NWindows::NFile::NFind::CFileInfo &archiveFileInfo,
    CArchiveItemInfoVector &archiveItems)
{
  archiveItems.Clear();
  UINT32 numItems;
  RETURN_IF_NOT_S_OK(archive->GetNumberOfItems(&numItems));
  archiveItems.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
  {
    CArchiveItemInfo itemInfo;
    NCOM::CPropVariant propVariantPath;
    RETURN_IF_NOT_S_OK(archive->GetProperty(i, kpidPath, &propVariantPath));
    UString filePath;
    if(propVariantPath.vt == VT_EMPTY)
      itemInfo.Name = defaultItemName;
    else
    {
      if(propVariantPath.vt != VT_BSTR)
        return E_FAIL;
      itemInfo.Name = propVariantPath.bstrVal;
    }
    itemInfo.Censored = censor.CheckName(itemInfo.Name);

    RETURN_IF_NOT_S_OK(GetFileTime(archive, i, itemInfo.LastWriteTime, 
        archiveFileInfo.LastWriteTime));

    CPropVariant propertySize;
    RETURN_IF_NOT_S_OK(archive->GetProperty(i, kpidSize, &propertySize));
    if (itemInfo.SizeIsDefined = (propertySize.vt != VT_EMPTY))
      itemInfo.Size = ConvertPropVariantToUINT64(propertySize);

    CPropVariant propertyIsFolder;
    RETURN_IF_NOT_S_OK(archive->GetProperty(i, kpidIsFolder, &propertyIsFolder));
    if(propertyIsFolder.vt != VT_BOOL)
      return E_FAIL;
    itemInfo.IsDirectory = VARIANT_BOOLToBool(propertyIsFolder.boolVal);

    itemInfo.IndexInServer = i;
    archiveItems.Add(itemInfo);
  }
  return S_OK;
}

static HRESULT UpdateWithItemLists(const CUpdateArchiveOptions &options,
    IArchiveHandler200 *archive, 
    const CArchiveItemInfoVector &archiveItems,
    const CArchiveStyleDirItemInfoVector &dirItems,
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

    RETURN_IF_NOT_S_OK(Compress(command.ActionSet, archive,
        options.MethodMode, realArchivePath, 
        archiveItems, dirItems, enablePercents,
        options.SfxMode, options.SfxModule));

    g_StdOut << endl;
  }
  return S_OK;
}

HRESULT UpdateArchiveStdMain(const NWildcard::CCensor &censor, 
    CUpdateArchiveOptions  &options, const CSysString &workingDir, 
    IArchiveHandler200 *archive,
    const UString *defaultItemName,
    const NWindows::NFile::NFind::CFileInfo *archiveFileInfo,
    bool enablePercents)
{
  CArchiveStyleDirItemInfoVector dirItems;
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

  CArchiveItemInfoVector archiveItems;
  if (archive != NULL)
  {
    RETURN_IF_NOT_S_OK(EnumerateInArchiveItems(censor, 
        archive, *defaultItemName, *archiveFileInfo, archiveItems));
  }

  RETURN_IF_NOT_S_OK(UpdateWithItemLists(options, archive, archiveItems, dirItems, enablePercents));

  if (archive != NULL)
  {
    RETURN_IF_NOT_S_OK(archive->Close());
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
