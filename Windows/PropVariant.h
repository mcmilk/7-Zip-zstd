// Windows/PropVariant.h

// #pragma once

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
  CPropVariant(UINT32 value) {  vt = VT_UI4; ulVal = value; }
  CPropVariant(UINT64 value) {  vt = VT_UI8; uhVal = *(ULARGE_INTEGER*)&value; }
  CPropVariant(const FILETIME &value) {  vt = VT_FILETIME; filetime = value; }
  CPropVariant(int value) { vt = VT_I4; lVal = value; }
  CPropVariant(BYTE value) { vt = VT_UI1; bVal = value; }
  CPropVariant(short value) { vt = VT_I2; iVal = value; }
  CPropVariant(long value, VARTYPE vtSrc = VT_I4) { vt = vtSrc; lVal = value; }

  CPropVariant& operator=(const CPropVariant& varSrc);
  CPropVariant& operator=(const PROPVARIANT& varSrc);
  CPropVariant& operator=(BSTR bstrSrc);
  CPropVariant& operator=(LPCOLESTR lpszSrc);
  CPropVariant& operator=(bool bSrc);
  CPropVariant& operator=(UINT32 value);
  CPropVariant& operator=(UINT64 value);
  CPropVariant& operator=(const FILETIME &value);

  CPropVariant& operator=(int value);
  CPropVariant& operator=(BYTE value);
  CPropVariant& operator=(short value);
  CPropVariant& operator=(long value);

  HRESULT Clear();
  HRESULT Copy(const PROPVARIANT* pSrc);
  HRESULT Attach(PROPVARIANT* pSrc);
  HRESULT Detach(PROPVARIANT* pDest);
  HRESULT ChangeType(VARTYPE vtNew, const PROPVARIANT* pSrc = NULL);

  HRESULT InternalClear();
  void InternalCopy(const PROPVARIANT* pSrc);

  HRESULT WriteToStream(ISequentialStream *stream) const;
  HRESULT ReadFromStream(ISequentialStream *stream);
  
  int Compare(const CPropVariant &a1);
};

}}

#endif
