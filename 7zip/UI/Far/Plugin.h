// Far/Plugin.h

#pragma once

#ifndef __FAR_PLUGIN_H
#define __FAR_PLUGIN_H

#include "Windows/COM.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"
#include "Common/MyCom.h"
#include "Far/FarUtils.h"

#include "../Common/ArchiverInfo.h"
#include "../Agent/IFolderArchive.h"

class CPlugin
{
  NWindows::NCOM::CComInitializer m_ComInitializer;
  UString m_CurrentDir;

  UString m_PannelTitle;
  
  InfoPanelLine m_InfoLines[30]; // Change it;

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
  UString m_FileName;
  // UString m_DefaultName;
  NWindows::NFile::NFind::CFileInfoW m_FileInfo;

  // std::auto_ptr<CProxyHandler> m_ProxyHandler; 
  CMyComPtr<IInFolderArchive> m_ArchiveHandler;
  CMyComPtr<IFolderFolder> _folder;
  
  // CArchiverInfo m_ArchiverInfo;
  UString _archiveTypeName;

  bool PasswordIsDefined;
  UString Password;


  CPlugin(const UString &fileName, 
        // const UString &aDefaultName, 
        IInFolderArchive *archiveHandler,
        UString archiveTypeName
        );
  ~CPlugin();

  void ReadValueSafe(PROPID aPropID, NWindows::NCOM::CPropVariant aPropVariant);
  void ReadPluginPanelItem(PluginPanelItem &aPanelItem, UINT32 anItemIndex);

  int GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
  void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
  int SetDirectory(const char *aDir, int opMode);
  void GetOpenPluginInfo(struct OpenPluginInfo *anInfo);

  int DeleteFiles(PluginPanelItem *aPanelItems, int itemsNumber, int opMode);


  /*
  void AddRealIndexOfFile(const CArchiveFolderItem &aFolder, int anIndexInVector, 
      std::vector<int> &aRealIndexes);
  void AddRealIndexes(const CArchiveFolderItem &anItem, 
      std::vector<int> &aRealIndexes);
  void GetRealIndexes(PluginPanelItem *aPanelItems, int itemsNumber,
      std::vector<int> &aRealIndexes);
  */

  HRESULT ExtractFiles(
      bool aDecompressAllItems,
      const UINT32 *anIndexes, 
      UINT32 numIndices, 
      bool aSilent, 
      NExtractionMode::NPath::EEnum aPathMode, 
      NExtractionMode::NOverwrite::EEnum overwriteMode,
      const UString &destPath,
      bool aPasswordIsDefined, const UString &password);

  NFar::NFileOperationReturnCode::EEnum GetFiles(struct PluginPanelItem *aPanelItem, int itemsNumber,
    int move, char *destPath, int opMode);
  
  NFar::NFileOperationReturnCode::EEnum GetFilesReal(struct PluginPanelItem *aPanelItems, 
    int itemsNumber, int move, const char *_aDestPath, int opMode, bool aShowBox);

  NFar::NFileOperationReturnCode::EEnum PutFiles(struct PluginPanelItem *aPanelItems, int itemsNumber,
    int move, int opMode);

  HRESULT ShowAttributesWindow();

  int ProcessKey(int aKey, unsigned int aControlState);
};

HRESULT CompressFiles(const CObjectVector<PluginPanelItem> &aPluginPanelItems);

#endif
