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

static const char *kTempArchiveFilePrefixString = "7zi";
static const char *kEverythingIsOk = "Everything is Ok";

static const char *kIllegalFileNameMessage = "Illegal file name for temp archive";

void EnumerateRecursed(const NWildcard::CCensorNode &aCurNode, 
    const CSysString &aDirectory, 
    const UString &aPrefix, 
    bool aCheckNameFull,
    CArchiveStyleDirItemInfoVector &aDirFileInfoVector, 
    UINT aCodePage,
    bool anEnterToSubFolders)
{
  NConsoleClose::CheckCtrlBreak();

  NFind::CEnumerator anEnumerator(aDirectory + TCHAR(kAnyStringWildcard));
  NFind::CFileInfo aFileInfo;
  while (anEnumerator.Next(aFileInfo))
  {
    NConsoleClose::CheckCtrlBreak();
    UString anUnicodeName = GetUnicodeString(aFileInfo.Name, aCodePage);
    if (aCheckNameFull)
    {
      if (aCurNode.CheckNameFull(anUnicodeName))
        AddDirFileInfo(aPrefix, aDirectory + aFileInfo.Name, aFileInfo, 
            aDirFileInfoVector, aCodePage);
    }
    else
    {
      if (aCurNode.CheckNameRecursive(anUnicodeName))
        AddDirFileInfo(aPrefix, aDirectory + aFileInfo.Name, aFileInfo, 
            aDirFileInfoVector, aCodePage);
    }
    if (anEnterToSubFolders && aFileInfo.IsDirectory())
    {
      const NWildcard::CCensorNode *aNextNode = &aCurNode;
      if (aCheckNameFull)
      {
        aNextNode = ((NWildcard::CCensorNode *)&aCurNode)->FindSubNode(anUnicodeName);
        if (aNextNode == NULL)
          aNextNode = &aCurNode;
      }
      EnumerateRecursed(*aNextNode,   
          aDirectory + aFileInfo.Name + TCHAR(kDirDelimiter), 
          aPrefix + anUnicodeName + wchar_t(kDirDelimiter), 
          aNextNode != (&aCurNode), aDirFileInfoVector, aCodePage, true);
    }
  }
}

void EnumerateItems(const NWildcard::CCensorNode &aCurNode, 
    const CSysString &aDirectory, 
    const UString &aPrefix,
    CArchiveStyleDirItemInfoVector &aDirFileInfoVector, 
    UINT aCodePage)
{
  NConsoleClose::CheckCtrlBreak();
  if (!aCurNode.GetAllowedRecursedNamesVector(false).IsEmpty() || 
      !aCurNode.GetAllowedRecursedNamesVector(true).IsEmpty()) 
  {
    EnumerateRecursed(aCurNode, aDirectory, aPrefix, true, aDirFileInfoVector, 
        aCodePage, true);
    return;
  }
  if (!aCurNode.GetAllowedNamesVector(false, true).IsEmpty())
  {
    EnumerateRecursed(aCurNode, aDirectory, aPrefix, true, aDirFileInfoVector, 
        aCodePage, false);
  }
  else
  {
    const UStringVector &aDirectNames = aCurNode.GetAllowedNamesVector(false, false);
    for (int i = 0; i < aDirectNames.Size(); i++)
    {
      const UString &aNameSpec = aDirectNames[i];
      if (aCurNode.CheckName(aNameSpec, false, false))
        continue;

      NFind::CFileInfo aFileInfo;
      if (!NFind::FindFile(aDirectory + UnicodeStringToMultiByte(aNameSpec, aCodePage), aFileInfo))
        continue;
    
      AddDirFileInfo(aPrefix, aDirectory + aFileInfo.Name, aFileInfo, 
        aDirFileInfoVector, aCodePage);
    }
  }
  for (int i = 0; i < aCurNode.m_SubNodes.Size(); i++)
  {
    const NWildcard::CCensorNode &aNextNode = aCurNode.m_SubNodes[i];
    EnumerateItems(aNextNode, 
        aDirectory + UnicodeStringToMultiByte(aNextNode.m_Name, aCodePage) + TCHAR(kDirDelimiter), 
        aPrefix + aNextNode.m_Name + wchar_t(kDirDelimiter),
        aDirFileInfoVector, aCodePage);
  }
}

HRESULT GetFileTime(IArchiveHandler200 *anArchive, UINT32 anIndex, 
    FILETIME &aFileTime, const FILETIME &aDefaultFileTime)
{
  CPropVariant aProperty;
  RETURN_IF_NOT_S_OK(anArchive->GetProperty(anIndex, kaipidLastWriteTime, &aProperty));
  if (aProperty.vt == VT_FILETIME)
    aFileTime = aProperty.filetime;
  else if (aProperty.vt == VT_EMPTY)
    aFileTime = aDefaultFileTime;
  else
    throw 4190407;
  return S_OK;
}

HRESULT EnumerateInArchiveItems(const NWildcard::CCensor &aCensor,
    IArchiveHandler200 *anArchive,
    const UString &aDefaultItemName,
    const NWindows::NFile::NFind::CFileInfo &anArchiveFileInfo,
    CArchiveItemInfoVector &anArchiveItems)
{
  anArchiveItems.Clear();
  UINT32 aNumItems;
  RETURN_IF_NOT_S_OK(anArchive->GetNumberOfItems(&aNumItems));
  anArchiveItems.Reserve(aNumItems);
  for(int i = 0; i < aNumItems; i++)
  {
    CArchiveItemInfo anItemInfo;
    NCOM::CPropVariant aPropVariantPath;
    RETURN_IF_NOT_S_OK(anArchive->GetProperty(i, kaipidPath, &aPropVariantPath));
    UString aFilePath;
    if(aPropVariantPath.vt == VT_EMPTY)
      anItemInfo.Name = aDefaultItemName;
    else
    {
      if(aPropVariantPath.vt != VT_BSTR)
        return E_FAIL;
      anItemInfo.Name = aPropVariantPath.bstrVal;
    }
    anItemInfo.Censored = aCensor.CheckName(anItemInfo.Name);

    RETURN_IF_NOT_S_OK(GetFileTime(anArchive, i, anItemInfo.LastWriteTime, 
        anArchiveFileInfo.LastWriteTime));

    CPropVariant aPropertySize;
    RETURN_IF_NOT_S_OK(anArchive->GetProperty(i, kaipidSize, &aPropertySize));
    if (anItemInfo.SizeIsDefined = (aPropertySize.vt != VT_EMPTY))
      anItemInfo.Size = ConvertPropVariantToUINT64(aPropertySize);

    CPropVariant aPropertyIsFolder;
    RETURN_IF_NOT_S_OK(anArchive->GetProperty(i, kaipidIsFolder, &aPropertyIsFolder));
    if(aPropertyIsFolder.vt != VT_BOOL)
      return E_FAIL;
    anItemInfo.IsDirectory = VARIANT_BOOLToBool(aPropertyIsFolder.boolVal);

    anItemInfo.IndexInServer = i;
    anArchiveItems.Add(anItemInfo);
  }
  return S_OK;
}

static HRESULT UpdateWithItemLists(const CUpdateArchiveOptions &anOptions,
    IArchiveHandler200 *anArchive, 
    const CArchiveItemInfoVector &anArchiveItems,
    const CArchiveStyleDirItemInfoVector &aDirItems,
    bool anEnablePercents)
{
  for(int i = 0; i < anOptions.Commands.Size(); i++)
  {
    const CUpdateArchiveCommand &aCommand = anOptions.Commands[i];
    const CSysString &aRealArchivePath = aCommand.ArchivePath;
    if (i == 0 && anOptions.UpdateArchiveItself)
    {
      if(anArchive != 0)
        g_StdOut << kUpdatingArchiveMessage;
      else
        g_StdOut << kCreatingArchiveMessage; 
      g_StdOut << anOptions.ArchivePath;
    }
    else
      g_StdOut << kCreatingArchiveMessage << aRealArchivePath;

    g_StdOut << endl << endl;

    RETURN_IF_NOT_S_OK(Compress(aCommand.ActionSet, anArchive,
        anOptions.MethodMode, aRealArchivePath, 
        anArchiveItems, aDirItems, anEnablePercents,
        anOptions.SfxMode, anOptions.SfxModule));

    g_StdOut << endl;
  }
  return S_OK;
}

HRESULT UpdateArchiveStdMain(const NWildcard::CCensor &aCensor, 
    CUpdateArchiveOptions  &anOptions, const CSysString &aWorkingDir, 
    IArchiveHandler200 *anArchive,
    const UString *aDefaultItemName,
    const NWindows::NFile::NFind::CFileInfo *anArchiveFileInfo,
    bool anEnablePercents)
{
  CArchiveStyleDirItemInfoVector aDirItems;
  g_StdOut << kScanningMessage;
  EnumerateItems(aCensor.m_Head, _T(""), L"", aDirItems, CP_OEMCP);
  g_StdOut << endl;

  CFileVectorBundle aFileVectorBundle;
  if(anOptions.UpdateArchiveItself)
  {
    if (!NDirectory::MyGetTempFileName(aWorkingDir, kTempArchiveFilePrefixString, 
        anOptions.Commands[0].ArchivePath))
      throw "create temp file error";
  }

  for(int i = 0; i < anOptions.Commands.Size(); i++)
  {
    aFileVectorBundle.Add(anOptions.Commands[i].ArchivePath, 
        i > 0 || !anOptions.UpdateArchiveItself);
    // SetBanOnFile(aCensor, aCurrentDir, anOptions.Commands[i].ArchivePath);
  }
  g_StdOut << endl;

  CArchiveItemInfoVector anArchiveItems;
  if (anArchive != NULL)
  {
    RETURN_IF_NOT_S_OK(EnumerateInArchiveItems(aCensor, 
        anArchive, *aDefaultItemName, *anArchiveFileInfo, anArchiveItems));
  }

  RETURN_IF_NOT_S_OK(UpdateWithItemLists(anOptions, anArchive, anArchiveItems, aDirItems, anEnablePercents));

  if (anArchive != NULL)
  {
    RETURN_IF_NOT_S_OK(anArchive->Close());
  }

  int aFirstNotTempArchiveIndex = anOptions.UpdateArchiveItself ? 1 : 0;
  for(i = anOptions.Commands.Size() - 1; i >= aFirstNotTempArchiveIndex; i--)
    aFileVectorBundle.DisableDeleting(i);
  if(anOptions.UpdateArchiveItself)
  {
    try
    {
      if (anArchive != NULL)
        if (!NDirectory::DeleteFileAlways(anOptions.ArchivePath))
          throw "delete file error";
      if (!::MoveFile(anOptions.Commands[0].ArchivePath, anOptions.ArchivePath))
        throw "move file error";
    }
    catch(...)
    {
      aFileVectorBundle.DisableDeleting(0);
      throw;
    }
  }
  g_StdOut << kEverythingIsOk << endl;
  return S_OK;
}
