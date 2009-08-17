// RegistryContextMenu.h

#ifndef __REGISTRY_CONTEXT_MENU_H
#define __REGISTRY_CONTEXT_MENU_H

namespace NZipRootRegistry {

#ifndef UNDER_CE
  bool CheckContextMenuHandler();
  void AddContextMenuHandler();
  void DeleteContextMenuHandler();
#endif

}

#endif
