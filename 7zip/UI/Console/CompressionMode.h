// CompressionMethodUtils.h

#pragma once

#ifndef __COMPRESSIONMETHODUTILS_H
#define __COMPRESSIONMETHODUTILS_H

struct CProperty
{
  UString Name;
  UString Value;
};

struct CCompressionMethodMode
{
  #ifndef EXCLUDE_COM
  CSysString FilePath;
  CLSID ClassID1;
  #else
  UString Name;
  #endif
  CObjectVector<CProperty> Properties;
  bool PasswordIsDefined;
  bool AskPassword;
  UString Password;
};

#endif
