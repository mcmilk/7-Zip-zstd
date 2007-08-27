// ClassDefs.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "IFolder.h"
#include "../../IPassword.h"
#include "PluginInterface.h"
#include "ExtractCallback.h"
#include "../../ICoder.h"

#include "../Agent/Agent.h"

// {23170F69-40C1-278A-1000-000100020000}
DEFINE_GUID(CLSID_CZipContextMenu, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);
