// Common/Exception.h

#ifndef __COMMON_EXCEPTION_H
#define __COMMON_EXCEPTION_H

struct CSystemException
{
  DWORD ErrorCode;
  CSystemException(): ErrorCode(::GetLastError()) {}
  CSystemException(DWORD errorCode): ErrorCode(errorCode) {}
};

#endif
