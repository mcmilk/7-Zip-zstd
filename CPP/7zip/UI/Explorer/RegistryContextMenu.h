// RegistryContextMenu.h

#ifndef ZIP7_INC_REGISTRY_CONTEXT_MENU_H
#define ZIP7_INC_REGISTRY_CONTEXT_MENU_H

#ifndef UNDER_CE

bool CheckContextMenuHandler(const UString &path, UInt32 wow = 0);
LONG SetContextMenuHandler(bool setMode, const UString &path, UInt32 wow = 0);

#endif

#endif
