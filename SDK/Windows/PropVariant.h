// Windows/PropVariant.h

#pragma once

#ifndef __WINDOWS_PROPVARIANT_H
#define __WINDOWS_PROPVARIANT_H

namespace NWindows {
namespace NCOM {

class CPropVariant : public tagPROPVARIANT
{
public:
  CPropVariant() { vt = VT_EMPTY; }
  ~CPropVariant() { Clear(); }
  CPropVariant(const PROPVARIANT& varSrc);
  CPropVariant(const CPropVariant& varSrc);
  CPropVariant(BSTR bstrSrc);
  CPropVariant(LPCOLESTR lpszSrc);
  CPropVariant(bool bSrc) { vt = VT_BOOL; boolVal = (bSrc ? VARIANT_TRUE : VARIANT_FALSE); };
  CPropVariant(UINT32 aValue) {  vt = VT_UI4; ulVal = aValue; }
  CPropVariant(UINT64 aValue) {  vt = VT_UI8; uhVal = *(ULARGE_INTEGER*)&aValue; }
  CPropVariant(const FILETIME &aValue) {  vt = VT_FILETIME; filetime = aValue; }
  CPropVariant(int aValue) { vt = VT_I4; lVal = aValue; }
  CPropVariant(BYTE aValue) { vt = VT_UI1; bVal = aValue; }
  CPropVariant(short aValue) { vt = VT_I2; iVal = aValue; }
  CPropVariant(long aValue, VARTYPE vtSrc = VT_I4) { vt = vtSrc; lVal = aValue; }

  CPropVariant& operator=(const CPropVariant& varSrc);
  CPropVariant& operator=(const PROPVARIANT& varSrc);
  CPropVariant& operator=(BSTR bstrSrc);
  CPropVariant& operator=(LPCOLESTR lpszSrc);
  CPropVariant& operator=(bool bSrc);
  CPropVariant& operator=(UINT32 aValue);
  CPropVariant& operator=(UINT64 aValue);
  CPropVariant& operator=(const FILETIME &aValue);

  CPropVariant& operator=(int aValue);
  CPropVariant& operator=(BYTE aValue);
  CPropVariant& operator=(short aValue);
  CPropVariant& operator=(long aValue);

  HRESULT Clear();
  HRESULT Copy(const PROPVARIANT* pSrc);
  HRESULT Attach(PROPVARIANT* pSrc);
  HRESULT Detach(PROPVARIANT* pDest);
  HRESULT ChangeType(VARTYPE vtNew, const PROPVARIANT* pSrc = NULL);

  HRESULT InternalClear();
  void InternalCopy(const PROPVARIANT* pSrc);

  HRESULT WriteToStream(ISequentialStream *aStream) const;
  HRESULT ReadFromStream(ISequentialStream *aStream);
  
  int Compare(const CPropVariant &a1);
};

}}

#endif