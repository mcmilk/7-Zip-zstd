// Windows/PropVariant.cpp

#include "StdAfx.h"

#include "PropVariant.h"

#include "../Common/Defs.h"

namespace NWindows {
namespace NCOM {

CPropVariant::CPropVariant(const PROPVARIANT& varSrc)
{
  vt = VT_EMPTY;
  InternalCopy(&varSrc);
}

CPropVariant::CPropVariant(const CPropVariant& varSrc)
{
  vt = VT_EMPTY;
  InternalCopy(&varSrc);
}

CPropVariant::CPropVariant(BSTR bstrSrc)
{
  vt = VT_EMPTY;
  *this = bstrSrc;
}

CPropVariant::CPropVariant(LPCOLESTR lpszSrc)
{
  vt = VT_EMPTY;
  *this = lpszSrc;
}

///////////////////////////
// Assignment Operators
CPropVariant& CPropVariant::operator=(const CPropVariant& varSrc)
{
  InternalCopy(&varSrc);
  return *this;
}
CPropVariant& CPropVariant::operator=(const PROPVARIANT& varSrc)
{
  InternalCopy(&varSrc);
  return *this;
}

CPropVariant& CPropVariant::operator=(BSTR bstrSrc)
{
  InternalClear();
  vt = VT_BSTR;
  bstrVal = ::SysAllocString(bstrSrc);
  if (bstrVal == NULL && bstrSrc != NULL)
  {
    vt = VT_ERROR;
    scode = E_OUTOFMEMORY;
  }
  return *this;
}

CPropVariant& CPropVariant::operator=(LPCOLESTR lpszSrc)
{
  InternalClear();
  vt = VT_BSTR;
  bstrVal = ::SysAllocString(lpszSrc);
  
  if (bstrVal == NULL && lpszSrc != NULL)
  {
    vt = VT_ERROR;
    scode = E_OUTOFMEMORY;
  }
  return *this;
}


CPropVariant& CPropVariant::operator=(bool bSrc)
{
  if (vt != VT_BOOL)
  {
    InternalClear();
    vt = VT_BOOL;
  }
  boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
  return *this;
}

CPropVariant& CPropVariant::operator=(UINT32 value)
{
  if (vt != VT_UI4)
  {
    InternalClear();
    vt = VT_UI4;
  }
  ulVal = value;
  return *this;
}

CPropVariant& CPropVariant::operator=(UINT64 value)
{
  if (vt != VT_UI8)
  {
    InternalClear();
    vt = VT_UI8;
  }
  uhVal = *(ULARGE_INTEGER*)&value;
  return *this;
}

CPropVariant& CPropVariant::operator=(const FILETIME &value)
{
  if (vt != VT_FILETIME)
  {
    InternalClear();
    vt = VT_FILETIME;
  }
  filetime = value;
  return *this;
}

CPropVariant& CPropVariant::operator=(int value)
{
  if (vt != VT_I4)
  {
    InternalClear();
    vt = VT_I4;
  }
  lVal = value;
  
  return *this;
}

CPropVariant& CPropVariant::operator=(BYTE value)
{
  if (vt != VT_UI1)
  {
    InternalClear();
    vt = VT_UI1;
  }
  bVal = value;
  return *this;
}

CPropVariant& CPropVariant::operator=(short value)
{
  if (vt != VT_I2)
  {
    InternalClear();
    vt = VT_I2;
  }
  iVal = value;
  return *this;
}

CPropVariant& CPropVariant::operator=(long value)
{
  if (vt != VT_I4)
  {
    InternalClear();
    vt = VT_I4;
  }
  lVal = value;
  return *this;
}

static HRESULT MyPropVariantClear(PROPVARIANT *aPropVariant) 
{ 
  switch(aPropVariant->vt)
  {
    case VT_UI1:
    case VT_I1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
    case VT_FILETIME:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
      aPropVariant->vt = VT_EMPTY;
      return S_OK;
  }
  return ::VariantClear((tagVARIANT *)aPropVariant); 
}

HRESULT CPropVariant::Clear() 
{ 
  return MyPropVariantClear(this);
}

HRESULT CPropVariant::Copy(const PROPVARIANT* pSrc) 
{ 
  ::VariantClear((tagVARIANT *)this); 
  switch(pSrc->vt)
  {
    case VT_UI1:
    case VT_I1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
    case VT_FILETIME:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
      MoveMemory((PROPVARIANT*)this, pSrc, sizeof(PROPVARIANT));
      return S_OK;
  }
  return ::VariantCopy((tagVARIANT *)this, (tagVARIANT *)(pSrc)); 
}


HRESULT CPropVariant::Attach(PROPVARIANT* pSrc)
{
  // Clear out the variant
  HRESULT hr = Clear();
  if (!FAILED(hr))
  {
    // Copy the contents and give control to CPropVariant
    memcpy(this, pSrc, sizeof(PROPVARIANT));
    pSrc->vt = VT_EMPTY;
    hr = S_OK;
  }
  return hr;
}

HRESULT CPropVariant::Detach(PROPVARIANT* pDest)
{
  // Clear out the variant
  HRESULT hr = MyPropVariantClear(pDest);
  // HRESULT hr = ::VariantClear((VARIANT* )pDest);
  if (!FAILED(hr))
  {
    // Copy the contents and remove control from CPropVariant
    memcpy(pDest, this, sizeof(PROPVARIANT));
    vt = VT_EMPTY;
    hr = S_OK;
  }
  return hr;
}

HRESULT CPropVariant::ChangeType(VARTYPE vtNew, const PROPVARIANT* pSrc)
{
  PROPVARIANT* pVar = const_cast<PROPVARIANT*>(pSrc);
  // Convert in place if pSrc is NULL
  if (pVar == NULL)
    pVar = this;
  // Do nothing if doing in place convert and vts not different
  return ::VariantChangeType((VARIANT *)this, (VARIANT *)pVar, 0, vtNew);
}

HRESULT CPropVariant::InternalClear()
{
  HRESULT hr = Clear();
  if (FAILED(hr))
  {
    vt = VT_ERROR;
    scode = hr;
  }
  return hr;
}

void CPropVariant::InternalCopy(const PROPVARIANT* pSrc)
{
  HRESULT hr = Copy(pSrc);
  if (FAILED(hr))
  {
    vt = VT_ERROR;
    scode = hr;
  }
}

HRESULT CPropVariant::WriteToStream(ISequentialStream *stream) const
{
  HRESULT aResult = stream->Write(&vt, sizeof(vt), NULL);
  if (FAILED(aResult))
    return aResult;

  if (vt == VT_EMPTY)
    return S_OK;

  int aNumBytes = 0;
  switch (vt)
  {
  case VT_UI1:
  case VT_I1:
    aNumBytes = sizeof(BYTE);
    break;
  case VT_I2:
  case VT_UI2:
  case VT_BOOL:
    aNumBytes = sizeof(short);
    break;
  case VT_I4:
  case VT_UI4:
  case VT_R4:
  case VT_INT:
  case VT_UINT:
  case VT_ERROR:
    aNumBytes = sizeof(long);
    break;
  case VT_FILETIME:
  case VT_UI8:
  case VT_R8:
  case VT_CY:
  case VT_DATE:
    aNumBytes = sizeof(double);
    break;
  default:
    break;
  }
  if (aNumBytes != 0)
    return stream->Write(&bVal, aNumBytes, NULL);

  if (vt == VT_BSTR)
  {
    UINT32 aLen = 0;
    if(bstrVal != NULL)
      aLen = SysStringLen(bstrVal);
    HRESULT aResult = stream->Write(&aLen, sizeof(UINT32), NULL);
    if (FAILED(aResult))
      return aResult;
    if(bstrVal == NULL)
      return S_OK;
    if(aLen == 0)
      return S_OK;
    return stream->Write(bstrVal, aLen * sizeof(wchar_t), NULL);
  }
  else
  {
    return E_FAIL;
    /*
    CPropVariant varBSTR;
    HRESULT hr = VariantChangeType(&varBSTR, this, VARIANT_NOVALUEPROP, VT_BSTR);
    if (FAILED(hr))
      return;
    MoveMemory(aMemoryPointer, varBSTR.bstrVal, SysStringLen(varBSTR.bstrVal));
    */
  }
}

HRESULT CPropVariant::ReadFromStream(ISequentialStream *stream)
{
  HRESULT hr = Clear();
  if (FAILED(hr))
    return hr;

  VARTYPE vtRead;
  hr = stream->Read(&vtRead, sizeof(VARTYPE), NULL);
  if (hr == S_FALSE)
    hr = E_FAIL;
  if (FAILED(hr))
    return hr;

  vt = vtRead;
  if (vt == VT_EMPTY)
    return S_OK;
  int aNumBytes = 0;
  switch (vt)
  {
    case VT_UI1:
    case VT_I1:
      aNumBytes = sizeof(BYTE);
      break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
      aNumBytes = sizeof(short);
      break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
      aNumBytes = sizeof(long);
      break;
    case VT_FILETIME:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
      aNumBytes = sizeof(double);
      break;
    default:
      break;
  }
  if (aNumBytes != 0)
  {
    hr = stream->Read(&bVal, aNumBytes, NULL);
    if (hr == S_FALSE)
      hr = E_FAIL;
    return hr;
  }

  if (vt == VT_BSTR)
  {
    bstrVal = NULL;
    UINT32 aLen = 0;
    hr = stream->Read(&aLen, sizeof(UINT32), NULL);
    if (hr != S_OK)
      return E_FAIL;
    bstrVal = SysAllocStringLen(NULL, aLen);
    if(bstrVal == NULL)
      return E_OUTOFMEMORY;
    hr = stream->Read(bstrVal, aLen * sizeof(wchar_t), NULL);
    if (hr == S_FALSE)
      hr = E_FAIL;
    return hr;
  }
  else
    return E_FAIL;
}

int CPropVariant::Compare(const CPropVariant &a)
{
  if(vt != a.vt)
    return 0; // it's mean some bug
  switch (vt)
  {
    case VT_EMPTY:
      return 0;
    
    /*
    case VT_I1:
      return MyCompare(cVal, a.cVal);
    */
    case VT_UI1:
      return MyCompare(bVal, a.bVal);

    case VT_I2:
      return MyCompare(iVal, a.iVal);
    case VT_UI2:
      return MyCompare(uiVal, a.uiVal);
    
    case VT_I4:
      return MyCompare(lVal, a.lVal);
    /*
    case VT_INT:
      return MyCompare(intVal, a.intVal);
    */
    case VT_UI4:
      return MyCompare(ulVal, a.ulVal);
    /*
    case VT_UINT:
      return MyCompare(uintVal, a.uintVal);
    */
    case VT_I8:
      return MyCompare(INT64(*(const INT64 *)&hVal), INT64(*(const INT64 *)&a.hVal));
    case VT_UI8:
      return MyCompare(UINT64(*(const UINT64 *)&uhVal), UINT64(*(const UINT64 *)&a.uhVal));

    case VT_BOOL:    
      return -MyCompare(boolVal, a.boolVal); // Test it

    case VT_FILETIME:
      return ::CompareFileTime(&filetime, &a.filetime);
    case VT_BSTR:
      return 0; // Not implemented 
      // return MyCompare(aPropVarint.cVal);

    default:
      return 0;
  }
}

}}
