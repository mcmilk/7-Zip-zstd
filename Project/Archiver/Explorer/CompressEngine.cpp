// CompressEngine.cpp

#include "StdAfx.h"

#include "CompressEngine.h"

#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"

#include "Common/StringConvert.h"

#include "CompressDialog.h"

#include "../Common/CompressEngineCommon.h"
#include "../Common/UpdateUtils.h"
#include "../Common/IArchiveHandler2.h"

#include "../Common/OpenEngine2.h"
#include "../Common/HelpUtils.h"

#include "../Agent/Handler.h"

#include "../Resource/Extract/resource.h"

#include "MyMessages.h"
#include "FormatUtils.h"
#include "UpdateCallback100.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;
using namespace NName;

using namespace NUpdateArchive;

static LPCTSTR kTempArcivePrefix = TEXT("7zi");
static LPCTSTR kDefaultSfxModule = TEXT("7zC.sfx");


/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

static void SplitString(const CSysString &aString, CSysStringVector &aStrings)
{
  aStrings.Clear();
  for (int aPos = 0; aPos < aString.Length();)
  {
    int aSpacePos = aString.Find(TEXT(' '), aPos);
    if (aSpacePos < 0)
    {
      aStrings.Add(aString.Mid(aPos));
      return;
    }
    if (aSpacePos != aPos)
      aStrings.Add(aString.Mid(aPos, aSpacePos - aPos));
    aPos = aSpacePos + 1;
  }
}

static bool ParseNumberString(const UString &aString, UINT32 &aNumber)
{
  wchar_t *anEndPtr;
  aNumber = wcstoul(aString, &anEndPtr, 10);
  return (anEndPtr - aString == aString.Length());
}

static void SetOptions(const CSysString &anOptions,
    CObjectVector<CComBSTR> &aNamesReal,
    std::vector<NCOM::CPropVariant> &aValues)
{
  CSysStringVector aStrings;
  SplitString(anOptions, aStrings);
  for(int i = 0; i < aStrings.Size(); i++)
  {
    const UString &aString = GetUnicodeString(aStrings[i]);
    int anIndex = aString.Find(L'=');
    CComBSTR aName;
    NCOM::CPropVariant aPropVariant;
    if (anIndex < 0)
      aName = aString;
    else
    {
      aName = aString.Left(anIndex);
      UString aValue = aString.Mid(anIndex + 1);
      if (!aValue.IsEmpty())
      {
        UINT32 aNumber;
        if (ParseNumberString(aValue, aNumber))
          aPropVariant = aNumber;
        else
          aPropVariant = aValue;
      }
    }
    aNamesReal.Add(aName);
    aValues.push_back(aPropVariant);
  }
}

static HRESULT SetOutProperties(IOutArchiveHandler100 * anOutArchive, 
    UINT32 aMethod, bool aSolidModeIsAllowed, bool aSolidMode, 
    const CSysString &anOptions)
{
  CComPtr<ISetProperties> aSetProperties;
  if (anOutArchive->QueryInterface(&aSetProperties) == S_OK)
  {
    CComBSTR aComBSTR;
    switch(aMethod)
    {
      case 0:
        aComBSTR = "0";
        break;
      case 1:
        aComBSTR = "1";
        break;
      case 2:
        aComBSTR = "X";
        break;
      default:
        return E_INVALIDARG;
    }
    CObjectVector<CComBSTR> aNamesReal;
    std::vector<NCOM::CPropVariant> aValues;
    aNamesReal.Add(aComBSTR);
    aValues.push_back(NCOM::CPropVariant());
    
    // Solid
    if (aSolidModeIsAllowed)
    {
      aNamesReal.Add(L"s");
      aValues.push_back(NCOM::CPropVariant(aSolidMode ? L"on": L"off"));
    }

    // Options 
    SetOptions(anOptions, aNamesReal, aValues);
    
    std::vector<BSTR> aNames;
    for(int i = 0; i < aNamesReal.Size(); i++)
      aNames.push_back(aNamesReal[i]);
    RETURN_IF_NOT_S_OK(aSetProperties->SetProperties(&aNames.front(), 
      &aValues.front(), aNames.size()));


  }
  return S_OK;
}

HRESULT CompressArchive(const CSysStringVector &aFileNames)
{
  if (aFileNames.Size() == 0)
    return S_OK;

  UString aCurrentDirU;
  const CSysString &aFrontName = aFileNames.Front();
  UString aFrontNameU = GetUnicodeString(aFrontName);
  /*
  if (!GetOnlyDirPrefix(aFrontName, aCurrentDir))
    return E_FAIL;
  */

  UString aResultPathU;
  {
    /*
    bool aPrevFolderOK = false;
    if (aCurrentDir.Length() > 3)
      aPrevFolderOK = GetOnlyName(aCurrentDir.Left(aCurrentDir.Length() - 1), aResultPath);
    if (aFileNames.Size() == 1 || !aPrevFolderOK)
    {
      if (!GetOnlyName(aFrontName, aResultPath))
        return E_FAIL;
    }
    */
    CParsedPath aParsedPath;
    aParsedPath.ParsePath(aFrontNameU);
    if(aParsedPath.PathParts.Size() == 0)
      return E_FAIL; // Error
    if (aFileNames.Size() == 1 || aParsedPath.PathParts.Size() == 1)
    {
      CSysString aPureName, aDot, anExtension;
      aResultPathU = aParsedPath.PathParts.Back();
    }
    else
    {
      aParsedPath.PathParts.DeleteBack();
      aResultPathU = aParsedPath.PathParts.Back();
    }
  }
  
  {
    CParsedPath aParsedPath;
    aParsedPath.ParsePath(aFrontNameU);
    aParsedPath.PathParts.DeleteBack();
    aCurrentDirU = aParsedPath.MergePath();
    if (aParsedPath.PathParts.Size() > 0)
      aCurrentDirU += NFile::NName::kDirDelimiter;
  }

  CCompressDialog aDialog;
  CZipRegistryManager aZipRegistryManager;
  aDialog.m_ZipRegistryManager = &aZipRegistryManager;

  CObjectVector<NZipRootRegistry::CArchiverInfo> anArchiverInfoList;
  NZipRootRegistry::ReadArchiverInfoList(anArchiverInfoList);
  aDialog.m_ArchiverInfoList.Clear();
  for(int i = 0; i < anArchiverInfoList.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = anArchiverInfoList[i];
    if (anArchiverInfo.UpdateEnabled)
      aDialog.m_ArchiverInfoList.Add(anArchiverInfo);
  }
  if(aDialog.m_ArchiverInfoList.Size() == 0)
  {
    MyMessageBox(_T("No Update Engines"));
    return E_FAIL;
  }

  aDialog.m_Info.ArchiveName = GetSystemString(aResultPathU);
  aDialog.m_Info.CurrentDirPrefix = GetSystemString(aCurrentDirU);
  aDialog.m_Info.SFXMode = false;
  aDialog.m_Info.SolidMode = true;

  if(aDialog.Create(0) != IDOK)
    return S_OK;

  CSysString anArcPath;
  if (!aDialog.m_Info.GetFullPathName(anArcPath))
  {
    MyMessageBox(_T("Incorrect archive path"));
    return E_FAIL;
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
    return E_FAIL;

  const CLSID &aClassID = 
      aDialog.m_ArchiverInfoList[aDialog.m_Info.ArchiverInfoIndex].ClassID;
  
  NFind::CFileInfo aFileInfo;

  CComPtr<IOutArchiveHandler100> anOutArchive;

  CComPtr<IArchiveHandler100> anArchiveHandler;
  if(NFind::FindFile(anArcPath, aFileInfo))
  {
    if (aFileInfo.IsDirectory())
    {
      MyMessageBox(TEXT("There is a folder with such name"));
      return E_FAIL;
    }
    NZipRootRegistry::CArchiverInfo anArchiverInfoResult;
    UString aDefaultName;
    HRESULT aResult = OpenArchive(anArcPath, &anArchiveHandler, 
        anArchiverInfoResult, aDefaultName, NULL);
    if (aResult == S_FALSE)
    {
      MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      return E_FAIL;
    }
    if (aResult != S_OK)
    {
      MyMessageBox(TEXT("Open error"));
      return E_FAIL;
    }
    if (anArchiverInfoResult.ClassID != aClassID)
    {
      MyMessageBox(TEXT("Type of existing archive differs from specified type"));
      return E_FAIL;
    }
    aResult = anArchiveHandler.QueryInterface(&anOutArchive);
    if(aResult != S_OK)
    {
      MyMessageBox(MyFormat(IDS_CANT_UPDATE_ARCHIVE, 0x02000602, anArcPath));
      return E_FAIL;
    }
  }
  else
  {
    // HRESULT aResult = anOutArchive.CoCreateInstance(aClassID);
    CComObjectNoLock<CAgent> *anAgentSpec = new CComObjectNoLock<CAgent>;
    anOutArchive = anAgentSpec;

    /*
    HRESULT aResult = anOutArchive.CoCreateInstance(CLSID_CAgentArchiveHandler);
    if (aResult != S_OK)
    {
      MyMessageBox(MyFormat(IDS_CANT_UPDATE_ARCHIVE, anArcPath));
      return E_FAIL;
    }
    */
  }

  CRecordVector<const wchar_t *> aFileNamePointers;
  aFileNamePointers.Reserve(aFileNames.Size());

  UStringVector aFileNames2;
  for(i = 0; i < aFileNames.Size(); i++)
    aFileNames2.Add(GetUnicodeString(aFileNames[i]));

  for(i = 0; i < aFileNames.Size(); i++)
    aFileNamePointers.Add(aFileNames2[i]);

  anOutArchive->SetFolder(NULL);
  anOutArchive->SetFiles(&aFileNamePointers.Front(), aFileNamePointers.Size());
  BYTE anActionSetByte[6];
  for (i = 0; i < 6; i++)
    anActionSetByte[i] = anActionSet->StateActions[i];

  CComObjectNoLock<CUpdateCallBack100Imp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBack100Imp>;
  CComPtr<IUpdateCallback100> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(anArchiveHandler, 0);

  RETURN_IF_NOT_S_OK(SetOutProperties(anOutArchive, aDialog.m_Info.Method, 
      aDialog.m_Info.SolidModeIsAllowed, aDialog.m_Info.SolidMode, 
      aDialog.m_Info.Options));

  UString aSFXModule;
  if (aDialog.m_Info.SFXMode)
  {
    CSysString aSFXModule2;
    LPCTSTR aPath = NULL;
    CSysString aSFXModule3;
    if (GetProgramDirPrefix(aSFXModule3))
      aPath = aSFXModule3;
    if (!NDirectory::MySearchPath(aPath, kDefaultSfxModule, NULL, aSFXModule2))
    {
      MyMessageBox(TEXT("can't find sfx module"));
      return E_FAIL;
    }
    aSFXModule = GetUnicodeString(aSFXModule2);
  }

  UINT aCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  HRESULT aResult = anOutArchive->DoOperation(&aClassID,
      GetUnicodeString(aTempFileName, aCodePage), anActionSetByte, 
      (aDialog.m_Info.SFXMode ? (const wchar_t *)aSFXModule : NULL),
      anUpdateCallBack);
  anUpdateCallBack.Release();
  anOutArchive.Release();

  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return aResult;
  }
 
  if(anArchiveHandler)
  {
    anArchiveHandler->Close();
    if (!DeleteFileAlways(anArcPath))
    {
      ShowLastErrorMessage();
      return E_FAIL;
    }
  }
  aTempFile.DisableDeleting();
  if (!::MoveFile(aTempFileName, anArcPath))
  {
    ShowLastErrorMessage();
    return E_FAIL;
  }
  return S_OK;
}
