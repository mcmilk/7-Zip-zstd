// RegistryAssociations.h

#ifndef __REGISTRYASSOCIATIONS_H
#define __REGISTRYASSOCIATIONS_H

#include "Common/String.h"
#include "Common/Vector.h"

namespace NRegistryAssociations {

  struct CExtInfo
  {
    UString Ext;
    UStringVector Plugins;
    // bool Enabled;
  };
  bool ReadInternalAssociation(const wchar_t *ext, CExtInfo &extInfo);
  void ReadInternalAssociations(CObjectVector<CExtInfo> &items);
  void WriteInternalAssociations(const CObjectVector<CExtInfo> &items);

  bool CheckShellExtensionInfo(const CSysString &extension);

  // void ReadCompressionInfo(NZipSettings::NCompression::CInfo &anInfo, 
  void DeleteShellExtensionInfo(const CSysString &extension);

  void AddShellExtensionInfo(const CSysString &extension,
      const CSysString &programTitle, 
      const CSysString &programOpenCommand, 
      const CSysString &iconPath,
      const void *shellNewData, int shellNewDataSize);


  ///////////////////////////
  // ContextMenu
  /*
  bool CheckContextMenuHandler();
  void AddContextMenuHandler();
  void DeleteContextMenuHandler();
  */

}

// bool GetProgramDirPrefix(CSysString &aFolder);

#endif
