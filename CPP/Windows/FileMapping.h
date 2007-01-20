// Windows/FileMapping.h

#ifndef __WINDOWS_FILEMAPPING_H
#define __WINDOWS_FILEMAPPING_H

#include "Windows/Handle.h"
#include "Windows/Defs.h"

namespace NWindows {
// namespace NFile {
// namespace NMapping {

class CFileMapping: public CHandle
{
public:
  bool Create(HANDLE file, LPSECURITY_ATTRIBUTES attributes,
    DWORD protect, UINT64 maximumSize, LPCTSTR name)
  {
    _handle = ::CreateFileMapping(file, attributes,
      protect, DWORD(maximumSize >> 32), DWORD(maximumSize), name);
    return (_handle != NULL);
  }

  bool Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _handle = ::OpenFileMapping(desiredAccess, BoolToBOOL(inheritHandle), name);
    return (_handle != NULL);
  }

  LPVOID MapViewOfFile(DWORD desiredAccess, UINT64 fileOffset, 
      SIZE_T numberOfBytesToMap)
  {
    return ::MapViewOfFile(_handle, desiredAccess, 
        DWORD(fileOffset >> 32), DWORD(fileOffset), numberOfBytesToMap);
  }

  LPVOID MapViewOfFileEx(DWORD desiredAccess, UINT64 fileOffset, 
      SIZE_T numberOfBytesToMap, LPVOID baseAddress)
  {
    return ::MapViewOfFileEx(_handle, desiredAccess, 
      DWORD(fileOffset >> 32), DWORD(fileOffset), 
      numberOfBytesToMap, baseAddress);
  }
  

};

}

#endif
