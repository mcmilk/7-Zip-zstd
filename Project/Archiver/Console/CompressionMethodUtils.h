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
  CLSID ClassID;
  #else
  CSysString Name;
  #endif
  CObjectVector<CProperty> Properties;
};

#endif
