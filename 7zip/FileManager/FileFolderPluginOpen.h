// FileFolderPluginOpen.h

#pragma once

#ifndef __FILEFOLDERPLUGINOPEN_H
#define __FILEFOLDERPLUGINOPEN_H

HRESULT OpenFileFolderPlugin(const UString &path, 
  HMODULE *module, IFolderFolder **resultFolder, HWND parentWindow);

#endif
