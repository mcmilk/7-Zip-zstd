// Common/WindowsTypes.h

// #pragma once

#ifndef __COMMON_WindowsTypes_H
#define __COMMON_WindowsTypes_H

  // typedef unsigned long UINT32;
  typedef unsigned __int64 UINT64;
  typedef UINT32 UINT;
  typedef UINT32 DWORD;
  #define CP_ACP 0
  #define CP_OEMCP 1
  #define CP_UTF8 65001
  typedef const char *LPCTSTR;
  typedef const wchar_t *LPCWSTR;
  #define TEXT 
  typedef struct {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[ 8 ];
  } GUID;
  typedef GUID CLSID;
  typedef char TCHAR;
  typedef char CHAR;
  typedef unsigned char UCHAR;
  typedef short SHORT;
  typedef unsigned short USHORT;
  typedef int INT;
  typedef UINT UINT_PTR;
  typedef long BOOL;
  typedef long LONG;
  typedef unsigned long ULONG;
  #define FALSE 0
  #define TRUE 1
  typedef short VARIANT_BOOL;
  #define VARIANT_TRUE ((VARIANT_BOOL)-1)
  #define VARIANT_FALSE ((VARIANT_BOOL)0)

  #define FILE_ATTRIBUTE_READONLY             0x00000001  
  #define FILE_ATTRIBUTE_HIDDEN               0x00000002  
  #define FILE_ATTRIBUTE_SYSTEM               0x00000004  
  #define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
  #define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
  #define FILE_ATTRIBUTE_DEVICE               0x00000040  
  #define FILE_ATTRIBUTE_NORMAL               0x00000080  
  #define FILE_ATTRIBUTE_TEMPORARY            0x00000100  
  #define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  
  #define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  
  #define FILE_ATTRIBUTE_COMPRESSED           0x00000800  
  #define FILE_ATTRIBUTE_OFFLINE              0x00001000  
  #define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  
  #define FILE_ATTRIBUTE_ENCRYPTED            0x00004000  
  typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
  } FILETIME, *PFILETIME, *LPFILETIME;
  typedef void *HANDLE;

  typedef __int64 LONGLONG;
  typedef unsigned __int64 ULONGLONG;

  struct LARGE_INTEGER  { LONGLONG QuadPart; };
  struct ULARGE_INTEGER { ULONGLONG QuadPart;  };
  typedef float FLOAT;
  typedef double DOUBLE;
  typedef VARIANT_BOOL _VARIANT_BOOL;
  typedef LONG SCODE;
  struct CY { LONGLONG int64; };
  typedef double DATE;
  typedef wchar_t *BSTR;

#endif

