// ItemInfoUtils.h

#pragma once

#ifndef __7Z_ITEMINFOUTILS_H
#define __7Z_ITEMINFOUTILS_H

#include "ItemInfo.h"
#include "../Common/IArchiveHandler.h"

namespace NArchive {
namespace N7z {

enum // PropID
{
  kaipidPackedSize0 = kaipidUserDefined,
  kaipidPackedSize1, 
  kaipidPackedSize2,
  kaipidPackedSize3
};

class CEnumArchiveItemProperty:
  public IEnumSTATPROPSTG,
  public CComObjectRoot
{
public:
  CRecordVector<UINT32> m_FileInfoPopIDs;
  int m_Index;

  BEGIN_COM_MAP(CEnumArchiveItemProperty)
    COM_INTERFACE_ENTRY(IEnumSTATPROPSTG)
  END_COM_MAP()
    
  DECLARE_NOT_AGGREGATABLE(CEnumArchiveItemProperty)
    
  DECLARE_NO_REGISTRY()
public:
  CEnumArchiveItemProperty(): m_Index(0) {};
  void Init(const CRecordVector<UINT32> &aFileInfoPopIDs);

  STDMETHOD(Next) (ULONG aNumItems, STATPROPSTG *anItems, ULONG *aNumFetched);
  STDMETHOD(Skip)  (ULONG aNumItems);
  STDMETHOD(Reset) ();
  STDMETHOD(Clone) (IEnumSTATPROPSTG **anEnum);
};

}}

#endif
