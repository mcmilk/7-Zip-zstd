// Far/Plugin.h

#pragma once

#ifndef __FAR_PLUGIN_H
#define __FAR_PLUGIN_H

#include "Windows/COM.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "Far/FarUtils.h"

#include "../Common/ZipRegistryMain.h"
#include "../Common/IArchiveHandler2.h"

class CPlugin
{
  NWindows::NCOM::CComInitializer m_ComInitializer;
  CSysString m_CurrentDir;

  CSysString m_PannelTitle;
  
  InfoPanelLine m_InfoLines[2];

  char m_FileNameBuffer[1024];
  char m_CurrentDirBuffer[1024];
  char m_PannelTitleBuffer[1024];

  AString PanelModeColumnTypes;
  AString PanelModeColumnWidths;
  PanelMode PanelMode;
  void AddColumn(PROPID aPropID);


  void EnterToDirectory(const UString &aDirName);

  void GetPathParts(UStringVector &aPathParts);
public:
  CSysString m_FileName;
  UString m_DefaultName;
  NWindows::NFile::NFind::CFileInfo m_FileInfo;

  // std::auto_ptr<CProxyHandler> m_ProxyHandler; 
  CComPtr<IArchiveHandler100> m_ArchiveHandler;
  CComPtr<IFolderFolder> _folder;
  
  NZipRootRegistry::CArchiverInfo m_ArchiverInfo;

  CPlugin(const CSysString &aFileName, const UString &aDefaultName, 
        IArchiveHandler100 *anArchiveHandler,
        const NZipRootRegistry::CArchiverInfo &anArchiverInfo);
  ~CPlugin();

  void ReadValueSafe(PROPID aPropID, NWindows::NCOM::CPropVariant aPropVariant);
  void ReadPluginPanelItem(PluginPanelItem &aPanelItem, UINT32 anItemIndex);

  int GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
  void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
  int SetDirectory(const char *aDir, int anOpMode);
  void GetOpenPluginInfo(struct OpenPluginInfo *anInfo);

  int DeleteFiles(PluginPanelItem *aPanelItems, int anItemsNumber, int anOpMode);


  /*
  void AddRealIndexOfFile(const CArchiveFolderItem &aFolder, int anIndexInVector, 
      std::vector<int> &aRealIndexes);
  void AddRealIndexes(const CArchiveFolderItem &anItem, 
      std::vector<int> &aRealIndexes);
  void GetRealIndexes(PluginPanelItem *aPanelItems, int anItemsNumber,
      std::vector<int> &aRealIndexes);
  */

  HRESULT ExtractFiles(
      bool aDecompressAllItems,
      const UINT32 *anIndexes, 
      UINT32 aNumIndexes, 
      bool aSilent, 
      NExtractionMode::NPath::EEnum aPathMode, 
      NExtractionMode::NOverwrite::EEnum anOverwriteMode,
      const CSysString &aDestPath,
      bool aPasswordIsDefined, const UString &aPassword);

  NFar::NFileOperationReturnCode::EEnum GetFiles(struct PluginPanelItem *aPanelItem, int anItemsNumber,
    int aMove, char *aDestPath, int anOpMode);
  
  NFar::NFileOperationReturnCode::EEnum GetFilesReal(struct PluginPanelItem *aPanelItems, 
    int anItemsNumber, int aMove, char *_aDestPath, int anOpMode, bool aShowBox);

  NFar::NFileOperationReturnCode::EEnum PutFiles(struct PluginPanelItem *aPanelItems, int anItemsNumber,
    int aMove, int anOpMode);

  HRESULT ShowAttributesWindow();

  int ProcessKey(int aKey, unsigned int aControlState);
};

HRESULT CompressFiles(const CObjectVector<PluginPanelItem> &aPluginPanelItems);

#endif
