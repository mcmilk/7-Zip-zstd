// 7zProperties.h

#pragma once

#ifndef __7Z_PROPERTIES_H
#define __7Z_PROPERTIES_H

#include "../../PropID.h"

namespace NArchive {
namespace N7z {

enum // PropID
{
  kpidPackedSize0 = kpidUserDefined,
  kpidPackedSize1, 
  kpidPackedSize2,
  kpidPackedSize3,
  kpidPackedSize4,
};

/*
class CEnumArchiveItemProperty:
  public IEnumSTATPROPSTG,
  public CComObjectRoot
{
  CRecordVector<UINT32> _fileInfoPopIDs;
  int _index;
public:

  BEGIN_COM_MAP(CEnumArchiveItemProperty)
    COM_INTERFACE_ENTRY(IEnumSTATPROPSTG)
  END_COM_MAP()
    
  DECLARE_NOT_AGGREGATABLE(CEnumArchiveItemProperty)
    
  DECLARE_NO_REGISTRY()
public:
  CEnumArchiveItemProperty(): _index(0) {};
  void Init(const CRecordVector<UINT32> &fileInfoPopIDs);

  STDMETHOD(Next) (ULONG numItems, STATPROPSTG *items, ULONG *numFetched);
  STDMETHOD(Skip)  (ULONG numItems);
  STDMETHOD(Reset) ();
  STDMETHOD(Clone) (IEnumSTATPROPSTG **enumerator);
};
*/

}}

#endif
