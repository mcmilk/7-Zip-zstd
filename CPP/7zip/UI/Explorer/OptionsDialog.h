// OptionsDialog.h

#ifndef __SEVENZIP_OPTIONSDIALOG_H
#define __SEVENZIP_OPTIONSDIALOG_H

#include "../FileManager/PluginInterface.h"
#include "Common/MyCom.h"

// {23170F69-40C1-278D-1000-000100020000}
DEFINE_GUID(CLSID_CSevenZipOptions, 
  0x23170F69, 0x40C1, 0x278D, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

class CSevenZipOptions: 
  public IPluginOptions,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP
  STDMETHOD(PluginOptions)(HWND hWnd, IPluginOptionsCallback *callback);
  STDMETHOD(GetFileExtensions)(BSTR *extensions);
};

#endif
