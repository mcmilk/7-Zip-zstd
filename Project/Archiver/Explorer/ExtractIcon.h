// ExtractIcon.h

#pragma once

#ifndef __EXTRACTICON_H
#define __EXTRACTICON_H

#include "../Common/IArchiveHandler2.h"

#include "Windows/ItemIDListUtils.h"

class CExtractIconImp: 
  public IExtractIcon,
  public CComObjectRoot
{
public:

BEGIN_COM_MAP(CExtractIconImp)
  COM_INTERFACE_ENTRY(IExtractIcon)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CExtractIconImp)
DECLARE_NO_REGISTRY()

  STDMETHOD(GetIconLocation)(
      UINT uFlags,
      LPTSTR szIconFile,
      UINT cchMax,
      int *piIndex,
      UINT *pwFlags);

  STDMETHOD(Extract)(
      LPCTSTR pszFile,
      UINT nIconIndex,
      HICON *phiconLarge,
      HICON *phiconSmall,
      UINT nIconSize);
  
  void Init(IArchiveFolder *anArchiveFolder, UINT32 anIndex);
private:
  CComPtr<IArchiveFolder> m_ArchiveFolder;
  UINT32 m_Index;
};


#endif