// Common/Exception.h

#pragma once

#ifndef __COMMON_EXCEPTION_H
#define __COMMON_EXCEPTION_H

struct CCException
{
  CCException() {}
  virtual ~CCException() {}
};

struct CSystemException
{
  DWORD ErrorCode;
  CSystemException(): ErrorCode(::GetLastError()) {}
  CSystemException(DWORD errorCode): ErrorCode(errorCode) {}
};

#endif

