// PluginInterface.h

#ifndef __PLUGININTERFACE_H
#define __PLUGININTERFACE_H

#include "Common/MyString.h"

// {23170F69-40C1-278D-0000-000100010000}
DEFINE_GUID(IID_IInitContextMenu, 
0x23170F69, 0x40C1, 0x278D, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278D-0000-000100010000")
IInitContextMenu: public IUnknown
{
public:
  STDMETHOD(InitContextMenu)(const wchar_t *aFolder, const wchar_t **aNames, UINT32 aNumFiles) PURE;  

};

// {23170F69-40C1-278D-0000-000100020100}
DEFINE_GUID(IID_IPluginOptionsCallback, 
0x23170F69, 0x40C1, 0x278D, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278D-0000-000100020000")
IPluginOptionsCallback: public IUnknown
{
public:
  STDMETHOD(GetProgramFolderPath)(BSTR *value) PURE;  
  STDMETHOD(GetProgramPath)(BSTR *value) PURE;  
  STDMETHOD(GetRegistryCUPath)(BSTR *value) PURE;  
};

// {23170F69-40C1-278D-0000-000100020000}
DEFINE_GUID(IID_IPluginOptions, 
0x23170F69, 0x40C1, 0x278D, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278D-0000-000100020000")
IPluginOptions: public IUnknown
{
public:
  STDMETHOD(PluginOptions)(HWND hWnd, IPluginOptionsCallback *callback) PURE;  
  // STDMETHOD(GetFileExtensions)(BSTR *extensions) PURE;
};

#endif
