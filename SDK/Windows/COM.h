// Windows/COM.h

#pragma once

#ifndef __WINDOWS_COM_H
#define __WINDOWS_COM_H

#include "Common/String.h"

namespace NWindows {
namespace NCOM {

class CComInitializer
{
public:
  CComInitializer() { CoInitialize(NULL);};
  ~CComInitializer() { CoUninitialize(); };
};

class CStgMedium
{
  STGMEDIUM m_Object;
public:
  bool m_MustBeReleased;
  CStgMedium(): m_MustBeReleased(false) {}
  ~CStgMedium() { Free(); }
  void Free() 
  { 
    if(m_MustBeReleased) 
      ReleaseStgMedium(&m_Object); 
    m_MustBeReleased = false;
  }
  const STGMEDIUM* operator->() const { return &m_Object;}
  STGMEDIUM* operator->() { return &m_Object;}
  STGMEDIUM* operator&() { return &m_Object; }
};

//////////////////////////////////
// GUID <--> String Conversions
UString GUIDToStringW(REFGUID aGUID);
AString GUIDToStringA(REFGUID aGUID);
#ifdef UNICODE
  #define GUIDToString GUIDToStringW
#else
  #define GUIDToString GUIDToStringA
#endif // !UNICODE

HRESULT StringToGUIDW(const wchar_t *aString, GUID &aClassID);
HRESULT StringToGUIDA(const char *aString, GUID &aClassID);
#ifdef UNICODE
  #define StringToGUID StringToGUIDW
#else
  #define StringToGUID StringToGUIDA
#endif // !UNICODE

  
}}

#endif
