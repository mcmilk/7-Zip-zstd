// Interface/EnumStatProp.h

#pragma once

#ifndef __ENUMSTATPROP_H
#define __ENUMSTATPROP_H

class CStatPropEnumerator:
  public IEnumSTATPROPSTG,
  public CComObjectRoot
{
  const STATPROPSTG *_properties;
  UINT32 _size;
  UINT32 _index;
public:
  void Init(const STATPROPSTG *properties, UINT32 size)
  {
    _properties = properties;
    _size = size;
    _index = 0;
  }

  BEGIN_COM_MAP(CStatPropEnumerator)
    COM_INTERFACE_ENTRY(IEnumSTATPROPSTG)
  END_COM_MAP()
    
  DECLARE_NOT_AGGREGATABLE(CStatPropEnumerator)
    
  DECLARE_NO_REGISTRY()
public:
  CStatPropEnumerator(): _index(0), _size(0) {};

  STDMETHOD(Next) (ULONG numItems, STATPROPSTG *items, ULONG *numFetched);
  STDMETHOD(Skip)  (ULONG numItems);
  STDMETHOD(Reset) ();
  STDMETHOD(Clone) (IEnumSTATPROPSTG **enumerator);

  static HRESULT CreateEnumerator(const STATPROPSTG *properties, UINT32 size, 
      IEnumSTATPROPSTG **enumerator);
 
};

#endif