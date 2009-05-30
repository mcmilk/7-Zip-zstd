// FileFolderPluginOpen.h

#ifndef __FILE_FOLDER_PLUGIN_OPEN_H
#define __FILE_FOLDER_PLUGIN_OPEN_H

HRESULT OpenFileFolderPlugin(IInStream *inStream, const UString &path,
  HMODULE *module, IFolderFolder **resultFolder, HWND parentWindow, bool &encrypted, UString &password);

#endif
