// 7zip/Far/Plugin.h

#ifndef ZIP7_INC_7ZIP_FAR_PLUGIN_H
#define ZIP7_INC_7ZIP_FAR_PLUGIN_H

#include "../../../Common/MyCom.h"

// #include "../../../Windows/COM.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/PropVariant.h"

#include "../Common/WorkDir.h"

#include "../Agent/Agent.h"

#include "FarUtils.h"

const UInt32 kNumInfoLinesMax = 64;

class CPlugin
{
  CAgent *_agent;
  CMyComPtr<IInFolderArchive> m_ArchiveHandler;
  CMyComPtr<IFolderFolder> _folder;

  // NWindows::NCOM::CComInitializer m_ComInitializer;
  UString m_CurrentDir;

  UString m_PannelTitle;
  FString m_FileName;
  NWindows::NFile::NFind::CFileInfo m_FileInfo;

  UString _archiveTypeName;
  
  InfoPanelLine m_InfoLines[kNumInfoLinesMax];

  char m_FileNameBuffer[1024];
  char m_CurrentDirBuffer[1024];
  char m_PannelTitleBuffer[1024];

  AString PanelModeColumnTypes;
  AString PanelModeColumnWidths;
  // PanelMode _panelMode;
  void AddColumn(PROPID aPropID);

  void EnterToDirectory(const UString &dirName);
  void GetPathParts(UStringVector &pathParts);
  void SetCurrentDirVar();
  // HRESULT AfterUpdate(CWorkDirTempFile &tempFile, const UStringVector &pathVector);

public:

  bool PasswordIsDefined;
  UString Password;

  CPlugin(const FString &fileName, CAgent *agent, UString archiveTypeName);
  ~CPlugin();

  void ReadPluginPanelItem(PluginPanelItem &panelItem, UInt32 itemIndex);

  int GetFindData(PluginPanelItem **panelItems,int *itemsNumber,int opMode);
  void FreeFindData(PluginPanelItem *panelItem,int ItemsNumber);
  int SetDirectory(const char *aszDir, int opMode);
  void GetOpenPluginInfo(struct OpenPluginInfo *info);
  int DeleteFiles(PluginPanelItem *panelItems, unsigned itemsNumber, int opMode);

  HRESULT ExtractFiles(
      bool decompressAllItems,
      const UInt32 *indices,
      UInt32 numIndices,
      bool silent,
      NExtract::NPathMode::EEnum pathMode,
      NExtract::NOverwriteMode::EEnum overwriteMode,
      const UString &destPath,
      bool passwordIsDefined, const UString &password);

  NFar::NFileOperationReturnCode::EEnum GetFiles(struct PluginPanelItem *panelItem, unsigned itemsNumber,
      int move, char *destPath, int opMode);
  
  NFar::NFileOperationReturnCode::EEnum GetFilesReal(struct PluginPanelItem *panelItems,
      unsigned itemsNumber, int move, const char *_aDestPath, int opMode, bool showBox);

  NFar::NFileOperationReturnCode::EEnum PutFiles(struct PluginPanelItem *panelItems, unsigned itemsNumber,
      int move, int opMode);
  HRESULT CreateFolder();

  HRESULT ShowAttributesWindow();

  int ProcessKey(int key, unsigned controlState);
};

HRESULT CompressFiles(const CObjectVector<PluginPanelItem> &pluginPanelItems);

#endif
