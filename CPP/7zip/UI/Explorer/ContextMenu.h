// ContextMenu.h

#ifndef __CONTEXT_MENU_H
#define __CONTEXT_MENU_H

#include "Common/MyString.h"

#include "../FileManager/MyCom2.h"

class CZipContextMenu:
  public IContextMenu,
  public IShellExtInit,
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
  
  MY_UNKNOWN_IMP2_MT(IContextMenu, IShellExtInit)

  // IShellExtInit
  STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT dataObject, HKEY hkeyProgID);

  // IContextMenu
  STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
  STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
  STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

  HRESULT InitContextMenu(const wchar_t *folder, const wchar_t **names, UINT32 numFiles);

  CZipContextMenu();
  ~CZipContextMenu();

private:

  struct CCommandMapItem
  {
    ECommandInternalID CommandInternalID;
    UString Verb;
    UString HelpString;
    UString Folder;
    UString ArcName;
    UString ArcType;
  };

  UStringVector _fileNames;
  bool _dropMode;
  UString _dropPath;
  CObjectVector<CCommandMapItem> _commandMap;

  HRESULT GetFileNames(LPDATAOBJECT dataObject, UStringVector &fileNames);
  int FindVerb(const UString &verb);
  void FillCommand(ECommandInternalID id, UString &mainString, CCommandMapItem &commandMapItem);
};

#endif
