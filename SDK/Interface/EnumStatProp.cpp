// Interface/EnumStatProp.cpp

#include "StdAfx.h"

#include "Interface/EnumStatProp.h"

STDMETHODIMP CStatPropEnumerator::Reset()
{
  _index = 0;
  return S_OK;
}

STDMETHODIMP CStatPropEnumerator::Next(ULONG numItems, 
    STATPROPSTG *items, ULONG *numFetched)
{
  HRESULT result = S_OK;
  if(numItems > 1 && !numFetched)
    return E_INVALIDARG;

  for(UINT32 index = 0; index < numItems; index++, _index++)
  {
    if(_index >= _size)
    {
      result =  S_FALSE;
      break;
    }
    const STATPROPSTG &srcItem = _properties[_index];
    STATPROPSTG &destItem = items[index];
    destItem.propid = srcItem.propid;
    destItem.vt = srcItem.vt;
    if(srcItem.lpwstrName != NULL)
    {
      destItem.lpwstrName = (wchar_t *)CoTaskMemAlloc((wcslen(srcItem.lpwstrName) + 1) * sizeof(wchar_t));
      wcscpy(destItem.lpwstrName, srcItem.lpwstrName);
    }
    else
      destItem.lpwstrName = srcItem.lpwstrName;
  }
  if (numFetched)
    *numFetched = index;
  return result;
}

STDMETHODIMP CStatPropEnumerator::Skip(ULONG numItems)
  {  return E_NOTIMPL; }

STDMETHODIMP CStatPropEnumerator::Clone(IEnumSTATPROPSTG **enumerator)
  {  return E_NOTIMPL; }

HRESULT CStatPropEnumerator::CreateEnumerator(const STATPROPSTG *properties, UINT32 size, 
    IEnumSTATPROPSTG **enumerator)
{
  CComObjectNoLock<CStatPropEnumerator> *enumeratorSpec = 
      new CComObjectNoLock<CStatPropEnumerator>;
  CComPtr<IEnumSTATPROPSTG> enumeratorTemp(enumeratorSpec);
  enumeratorSpec->Init(properties, size);
  *enumerator = enumeratorTemp.Detach();
  return S_OK;
}
