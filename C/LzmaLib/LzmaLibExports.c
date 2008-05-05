/* LzmaLibExports.c  -- LZMA library DLL Entry point
2008-03-26
Igor Pavlov
Public domain */

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
  hInstance = hInstance;
  dwReason = dwReason;
  lpReserved = lpReserved;
  return TRUE;
}
