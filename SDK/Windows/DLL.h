// Windows/DLL.h

#ifndef __WINDOWS_DLL_H
#define __WINDOWS_DLL_H

#pragma once

namespace NWindows {
namespace NDLL {

class CLibrary
{
  unsigned int _refCount;
  bool FreeAll();
  bool LoadOperations(HMODULE newModule);
protected:
  HMODULE _module;
public:
  operator HMODULE() const { return _module; }
  CLibrary():_module(NULL), _refCount(0){};
  ~CLibrary();
  // operator HMODULE() const { return _module; };
  // bool IsLoaded() const { return (_module != NULL); };
  bool Free();
  bool LoadEx(LPCTSTR fileName, DWORD flags = LOAD_LIBRARY_AS_DATAFILE);
  bool Load(LPCTSTR fileName);
  FARPROC GetProcAddress(LPCSTR procName) const
    { return ::GetProcAddress(_module, procName); }
};

}}

#endif