// IMyUnknown.h

// #pragma once

#ifndef __MYUNKNOWN_H
#define __MYUNKNOWN_H

#ifdef WIN32

// #include <guiddef.h>
#include <basetyps.h>

#else 

#define HRESULT LONG
#define STDMETHODCALLTYPE __stdcall 
#define STDMETHOD_(t, f) virtual t STDMETHODCALLTYPE f
#define STDMETHOD(f) STDMETHOD_(HRESULT, f)
#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
#define STDMETHODIMP STDMETHODIMP_(HRESULT)

#define PURE = 0;

typedef struct {
  unsigned long  Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char Data4[8];
} GUID;

#ifdef __cplusplus
    #define MY_EXTERN_C    extern "C"
#else
    #define MY_EXTERN_C    extern
#endif

#ifdef INITGUID
  #define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
      MY_EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
  #define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
      MY_EXTERN_C const GUID name
#endif

#ifdef __cplusplus
#define REFGUID const GUID &
#else
#define REFGUID const GUID * __MIDL_CONST
#endif

#define MIDL_INTERFACE(x) struct 
inline int operator==(REFGUID g1, REFGUID g2)
{ 
  for (int i = 0; i < sizeof(g1); i++)
    if (((unsigned char *)&g1)[i] != ((unsigned char *)&g2)[i])
      return false;
  return true;
}
inline int operator!=(REFGUID &g1, REFGUID &g2)
  { return !(g1 == g2); }

struct IUnknown
{
  STDMETHOD(QueryInterface) (const GUID *iid, void **outObject) PURE;
  STDMETHOD_(ULONG, AddRef)() PURE;
  STDMETHOD_(ULONG, Release)() PURE;
};

#endif
  
#endif
