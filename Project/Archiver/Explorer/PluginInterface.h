// PluginInterface.h

#pragma once

#ifndef __PLUGININTERFACE_H
#define __PLUGININTERFACE_H

#include "Common/String.h"

// {23170F69-40C1-278D-0000-000100000000}
DEFINE_GUID(IID_IInitContextMenu, 
0x23170F69, 0x40C1, 0x278D, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278D-0000-000100000000")
IInitContextMenu: public IUnknown
{
public:
  STDMETHOD(InitContextMenu)(const wchar_t *aFolder, const wchar_t **aNames, UINT32 aNumFiles) PURE;  
};

#endif
