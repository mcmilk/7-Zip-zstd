// ClassDefs.cpp

#include "StdAfx.h"

// #define INITGUID
#include <initguid.h>

#include "IFolder.h"
#include "../IPassword.h"
// #include "../Archiver/Format/Common/ArchiveInterface.h"
#include "PluginInterface.h"
#include "ExtractCallback.h"
#include "../ICoder.h"

/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

// {23170F69-40C1-278A-1000-000100020000}
DEFINE_GUID(CLSID_CZipContextMenu, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

/*
// {23170F69-40C1-278F-1000-000110050000}
DEFINE_GUID(CLSID_CTest, 
  0x23170F69, 0x40C1, 0x278F, 0x10, 0x00, 0x00, 0x01, 0x10, 0x05, 0x00, 0x00);
*/
