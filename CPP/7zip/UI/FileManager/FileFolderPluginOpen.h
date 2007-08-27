// FileFolderPluginOpen.h

#ifndef __FILEFOLDERPLUGINOPEN_H
#define __FILEFOLDERPLUGINOPEN_H

HRESULT OpenFileFolderPlugin(const UString &path, 
  HMODULE *module, IFolderFolder **resultFolder, HWND parentWindow, bool &encrypted);

#endif
