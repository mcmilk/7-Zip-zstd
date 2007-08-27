// ContextMenu.h

#ifndef __CONTEXTMENU_H
#define __CONTEXTMENU_H

// {23170F69-40C1-278A-1000-000100020000}
DEFINE_GUID(CLSID_CZipContextMenu, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

#include "Common/MyString.h"

#include "../FileManager/PluginInterface.h"
#include "../FileManager/MyCom2.h"


class CZipContextMenu: 
  public IContextMenu,
  public IShellExtInit,
  public IInitContextMenu,
  public CMyUnknownImp
{

public:

  enum ECommandInternalID
  {
    kCommandNULL,
    kOpen,
    kExtract,
    kExtractHere,
    kExtractTo,
    kTest,
    kCompress,
    kCompressEmail,
    kCompressTo7z,
    kCompressTo7zEmail,
    kCompressToZip,
    kCompressToZipEmail
  };
  
  struct CCommandMapItem
  {
    ECommandInternalID CommandInternalID;
    UString Verb;
    UString HelpString;
    UString Folder;
    UString Archive;
    UString ArchiveType;
  };

  MY_UNKNOWN_IMP3_MT(IContextMenu, IShellExtInit, IInitContextMenu)

  ///////////////////////////////
  // IShellExtInit

  STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, 
      LPDATAOBJECT dataObject, HKEY hkeyProgID);

  /////////////////////////////
  // IContextMenu
  
  STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu,
      UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
  STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
  STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved,
      LPSTR pszName, UINT cchMax);


  // IInitContextMenu
  STDMETHOD(InitContextMenu)(const wchar_t *folder, const wchar_t **names, UINT32 numFiles);  
private:
  UStringVector _fileNames;
  bool _dropMode;
  UString _dropPath;
  CObjectVector<CCommandMapItem> _commandMap;
  HRESULT GetFileNames(LPDATAOBJECT dataObject, UStringVector &fileNames);
  int FindVerb(const UString &verb);

  void FillCommand(ECommandInternalID id, UString &mainString, 
      CCommandMapItem &commandMapItem);
public:
  CZipContextMenu();
  ~CZipContextMenu();
};

#endif
