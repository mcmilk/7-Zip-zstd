// OptionsDialog.h

#pragma once

#ifndef __SEVENZIP_OPTIONSDIALOG_H
#define __SEVENZIP_OPTIONSDIALOG_H

#include "../../FileManager/PluginInterface.h"

// {23170F69-40C1-278D-1000-000100020000}
DEFINE_GUID(CLSID_CSevenZipOptions, 
  0x23170F69, 0x40C1, 0x278D, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

class CSevenZipOptions: 
  public IPluginOptions,
  public CComObjectRoot,
  public CComCoClass<CSevenZipOptions, &CLSID_CSevenZipOptions>
{
public:
BEGIN_COM_MAP(CSevenZipOptions)
  COM_INTERFACE_ENTRY(IPluginOptions)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSevenZipOptions)

// DECLARE_NO_REGISTRY();
DECLARE_REGISTRY(CSevenZipOptions, TEXT("SevenZip.Plugin7zipOptions.1"), 
                 TEXT("SevenZip.Plugin7zipOptions"), 
    UINT(0), THREADFLAGS_APARTMENT)
  STDMETHOD(PluginOptions)(HWND hWnd, IPluginOptionsCallback *callback);
  STDMETHOD(GetFileExtensions)(BSTR *extensions);
};

#endif
