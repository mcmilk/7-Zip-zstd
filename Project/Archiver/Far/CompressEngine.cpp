// CompressEngine.h

#include "StdAfx.h"

#include "ProxyHandler.h"
#include "CompressEngine.h"
#include "UpdateEngine.h"

#include "../../Archiver/Common/CompressEngineCommon.h"
#include "../../Archiver/Common/OpenEngine2.h"
#include "../../Archiver/Common/UpdateProducer.h"
#include "../../Archiver/Common/UpdateUtils.h"

#include "Windows/File/Name.h"
#include "Windows/File/Find.h"
#include "Windows/File/Directory.h"
#include "Windows/PropVariant.h"
#include "Windows/Error.h"

#include "../../../WinWrappers/PropVariantConversions.h"

// #include "CompressDialog.h"

#include "Common/StringConvert.h"
// #include "ArchiveStyleDirItemInfo.h"

#include "Interface/FileStreams.h"

#include "Messages.h"
#include "Far/FarUtils.h"
// #include "ZipViewUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;
using namespace NCOM;

using namespace NFar;
using namespace NUpdateArchive;

#define RETURN_IF_NOT_S_OK(x) { HRESULT aResult = (x); if(aResult != S_OK) return aResult; }

static LPCTSTR kTempArcivePrefix = "7zi";

static void GetFileTime(CProxyHandler *aProxyHandler, LPCITEMIDLIST anItemIDList, 
    FILETIME &aFileTime)
{
  CPropVariant aProperty;
  aProxyHandler->GetPropertyValue(anItemIDList, kaipidLastWriteTime, &aProperty);
  if (aProperty.vt == VT_FILETIME)
    aFileTime = aProperty.filetime;
  else if (aProperty.vt == VT_EMPTY)
    aFileTime = aProxyHandler->m_ArchiveFileInfo.LastWriteTime;
  else
    throw 4190407;
}

void EnumerateInArchiveItems(CProxyHandler *aProxyHandler, 
    const CArchiveFolderItem &anItem, const UString &aPrefix,
    CArchiveItemInfoVector &anArchiveItems)
{
  for(int i = 0; i < anItem.m_FileSubItems.Size(); i++)
  {
    const CArchiveFolderFileItem &aFileItem = anItem.m_FileSubItems[i];
    CArchiveItemInfo anItemInfo;

    GetFileTime(aProxyHandler, aFileItem.m_Properties, anItemInfo.LastWriteTime);

    CPropVariant aProperty;
    aProxyHandler->GetPropertyValue(aFileItem.m_Properties, kaipidSize, &aProperty);
    if (anItemInfo.SizeIsDefined = (aProperty.vt != VT_EMPTY))
      anItemInfo.Size = ConvertPropVariantToUINT64(aProperty);
    anItemInfo.IsDirectory = false;
    anItemInfo.Name = aPrefix + aFileItem.m_Name;
    anItemInfo.Censored = true; // test it
    anItemInfo.IndexInServer = aProxyHandler->GetHandlerItemIndex(aFileItem.m_Properties);
    anArchiveItems.Add(anItemInfo);
  }
  for(i = 0; i < anItem.m_DirSubItems.Size(); i++)
  {
    const CArchiveFolderItem &aDirItem = anItem.m_DirSubItems[i];
    if(!aDirItem.m_IsLeaf)
      continue;
    CArchiveItemInfo anItemInfo;
    GetFileTime(aProxyHandler, aDirItem.m_Properties, anItemInfo.LastWriteTime);
    anItemInfo.IsDirectory = true;
    anItemInfo.SizeIsDefined = false;
    anItemInfo.Name = aPrefix + aDirItem.m_Name;
    anItemInfo.Censored = true; // test it
    anItemInfo.IndexInServer = aProxyHandler->GetHandlerItemIndex(
        aDirItem.m_Properties);
    anArchiveItems.Add(anItemInfo);
    EnumerateInArchiveItems(aProxyHandler, aDirItem, anItemInfo.Name + 
        wchar_t(kDirDelimiter), anArchiveItems);
  }
}


static const char *kExtension = _T(".zip");


HRESULT Compress(const CSysStringVector &aFileNames, 
    const UString &anArchiveNamePrefix, 
    const CActionSet &anActionSet, CProxyHandler *aProxyHandler,
    const CLSID &aClassID, bool aStoreMode, bool aMaximizeRatioMode,
    CSysString &anArchiveName, CProgressBox *aProgressBox)
{
  CComPtr<IOutArchiveHandler> anOutArchive;
  CArchiveItemInfoVector anArchiveItems;
  if(aProxyHandler != NULL)
  {
    HRESULT aResult = aProxyHandler->m_ArchiveHandler.QueryInterface(&anOutArchive);
    if(aResult != S_OK)
    {
      g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
      return E_FAIL;
    }
    EnumerateInArchiveItems(aProxyHandler, 
      aProxyHandler->m_FolderItemHead,  L"", anArchiveItems);
  }
  else
  {
    HRESULT aResult = anOutArchive.CoCreateInstance(aClassID);
    if (aResult != S_OK)
    {
      g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
      return E_FAIL;
    }
  }
  CArchiveStyleDirItemInfoVector aDirItems;

  EnumerateItems(aFileNames, anArchiveNamePrefix, aDirItems, CP_OEMCP);
  
  CUpdatePairInfoVector anUpdatePairs;

  NFileTimeType::EEnum aFileTimeType;
  UINT32 aValue;
  RETURN_IF_NOT_S_OK(anOutArchive->GetFileTimeType(&aValue));

  switch(aValue)
  {
    case NFileTimeType::kWindows:
    case NFileTimeType::kDOS:
    case NFileTimeType::kUnix:
      aFileTimeType = NFileTimeType::EEnum(aValue);
      break;
    default:
      return E_FAIL;
  }

  GetUpdatePairInfoList(aDirItems, anArchiveItems, aFileTimeType, anUpdatePairs);
  
  CUpdatePairInfo2Vector anOperationChain;
  UpdateProduce(aDirItems, anArchiveItems, anUpdatePairs, anActionSet,
      anOperationChain);
  
  CComObjectNoLock<CUpdateCallBackImp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBackImp>;
  CComPtr<IUpdateCallBack> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(&aDirItems, &anArchiveItems, &anOperationChain,
      aProgressBox);
  
  CComObjectNoLock<COutFileStream> *anOutStreamSpec =
    new CComObjectNoLock<COutFileStream>;
  CComPtr<IOutStream> anOutStream(anOutStreamSpec);

  {
    CSysString aResultPath;
    int aPos;
    if(! NFile::NDirectory::MyGetFullPathName(anArchiveName, aResultPath, aPos))
      throw 141716;
    NFile::NDirectory::CreateComplexDirectory(aResultPath.Left(aPos));
  }
  if (!anOutStreamSpec->Open(anArchiveName))
  {
    ShowLastErrorMessage();
    return E_FAIL;
  }
  
  HRESULT aResult = anOutArchive->UpdateItems(anOutStream, anOperationChain.Size(),
     BoolToMyBool(aStoreMode), BoolToMyBool(aMaximizeRatioMode), anUpdateCallBack);
  return aResult;
}


// The returned string ends with a backslash


/*

void CompressArchive(const CSysStringVector &aFileNames)
{
  if (aFileNames.Size() == 0)
    return;
  CSysString aResultPath;
  {
    CParsedPath aParsedPath;
    aParsedPath.ParsePath(aFileNames.Front());
    if(aParsedPath.PathParts.Size() == 0)
      return; // Error
    if (aFileNames.Size() == 1 || aParsedPath.PathParts.Size() == 1)
    {
      CSysString aPureName, aDot, anExtension;
      SplitNameToPureNameAndExtension(aParsedPath.PathParts.Back(), 
        aPureName, aDot, anExtension);
      // aParsedPath.PathParts.Back() = aPureName;
      // aResultPath = aParsedPath.MergePath();
      aResultPath = aPureName;
    }
    else
    {
      aParsedPath.PathParts.DeleteBack();
      // aResultPath = aParsedPath.MergePath();
      // aResultPath += NFile::NName::kDirDelimiter;
      // aResultPath += aParsedPath.PathParts.Back();
      aResultPath = aParsedPath.PathParts.Back();
    }
    aResultPath += kExtension;
  }
  CSysString aCurrentDir;
  {
    CParsedPath aParsedPath;
    aParsedPath.ParsePath(aFileNames.Front());
    aParsedPath.PathParts.DeleteBack();
    aCurrentDir = aParsedPath.MergePath();
    if (aParsedPath.PathParts.Size() > 0)
      aCurrentDir += NFile::NName::kDirDelimiter;
  }


  CCompressDialog aDialog;
  
  
  
  CZipRegistryManager aZipRegistryManager;
  aDialog.m_ZipRegistryManager = &aZipRegistryManager;

  NZipRootRegistry::CArchiverInfoVector anArchiverInfoList;
  NZipRootRegistry::ReadArchiverInfoList(anArchiverInfoList);
  aDialog.m_ArchiverInfoList.Clear();
  for(int i = 0; i < anArchiverInfoList.Size(); i++)
  {
    NZipRootRegistry::CArchiverInfo anArchiverInfo = anArchiverInfoList[i];
    if (anArchiverInfo.UpdateEnabled)
      aDialog.m_ArchiverInfoList.Add(anArchiverInfo);
  }
  if(aDialog.m_ArchiverInfoList.Size() == 0)
  {
    AfxMessageBox("No Update Engines");
    return;
  }

  aDialog.m_Info.ArchiveName = aResultPath;
  aDialog.m_Info.CurrentDirPrefix = aCurrentDir;

  if(aDialog.DoModal() != IDOK)
    return;

  CSysString anArcPath;
  if (!aDialog.m_Info.GetFullPathName(anArcPath))
  {
    AfxMessageBox("Incorrect archive path");;
    return;
  }
  const CActionSet *anActionSet;
  switch(aDialog.m_Info.UpdateMode)
  {
    case NCompressDialog::NUpdateMode::kAdd:
      anActionSet = &kAddActionSet;
      break;
    case NCompressDialog::NUpdateMode::kUpdate:
      anActionSet = &kUpdateActionSet;
      break;
    case NCompressDialog::NUpdateMode::kFresh:
      anActionSet = &kFreshActionSet;
      break;
    case NCompressDialog::NUpdateMode::kSynchronize:
      anActionSet = &kSynchronizeActionSet;
      break;
    default:
      throw 1091756;
  }


  NZipSettings::NWorkDir::CInfo aWorkDirInfo;
  aZipRegistryManager.ReadWorkDirInfo(aWorkDirInfo);
  CSysString aWorkDir = GetWorkDir(aWorkDirInfo, anArcPath);
  NFile::NDirectory::CreateComplexDirectory(aWorkDir);

  NFile::NDirectory::CTempFile aTempFile;
  CSysString aTempFileName;
  if (aTempFile.Create(aWorkDir, kTempArcivePrefix, aTempFileName) == 0)
    return;

  CProxyHandler *aProxyHandler;
  NFind::CFileInfo aFileInfo;
  if(NFind::FindFile(anArcPath, aFileInfo))
  {
    if (aFileInfo.IsDirectory())
    {
      CString aMessage;
      AfxFormatString1(aMessage, IDS_CANT_UPDATE_ARCHIVE, anArcPath);
      AfxMessageBox(aMessage);
      return;
    }
    bool aHandlerIsNew;
    if (!g_HandlersManager.GetProxyHandler(anArcPath, &aProxyHandler, aHandlerIsNew))
    {
      CString aMessage;
      AfxFormatString1(aMessage, IDS_CANT_UPDATE_ARCHIVE, anArcPath);
      AfxMessageBox(aMessage);
      return;
    }
  }
  else
    aProxyHandler = NULL;


  HRESULT aResult = Compress(aFileNames, *anActionSet, aProxyHandler, 
      aDialog.m_ArchiverInfoList[aDialog.m_Info.ArchiverInfoIndex].ClassID,
      aDialog.m_Info.Method == NCompressDialog::NMethod::kStore,
      aDialog.m_Info.Method == NCompressDialog::NMethod::kMaximum,
        aTempFileName);
  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return;
  }
  if(aProxyHandler != 0) // Update
  {
    if (!NFile::NDirectory::DeleteFileAlways(anArcPath))
    {
      ShowLastErrorMessage();
      return;
    }
  }
  aTempFile.DisableDeleting();
  if (!MoveFile(aTempFileName, anArcPath))
  {
    ShowLastErrorMessage();
    return;
  }
}
*/


