// RandGen.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "Windows/Synchronization.h"

#include "RandGen.h"

#ifndef _WIN32
#include <unistd.h>
#define USE_POSIX_TIME
#define USE_POSIX_TIME2
#endif

#ifdef USE_POSIX_TIME
#include <time.h>
#ifdef USE_POSIX_TIME2
#include <sys/time.h>
#endif
#endif

// This is not very good random number generator.
// Please use it only for salt.
// First generated data block depends from timer and processID.
// Other generated data blocks depend from previous state
// Maybe it's possible to restore original timer value from generated value.

void CRandomGenerator::Init()
{ 
  NCrypto::NSha1::CContext hash;
  hash.Init();

  #ifdef _WIN32
  DWORD w = ::GetCurrentProcessId();
  hash.Update((const Byte *)&w, sizeof(w));
  w = ::GetCurrentThreadId();
  hash.Update((const Byte *)&w, sizeof(w));
  #else
  pid_t pid = getpid();
  hash.Update((const Byte *)&pid, sizeof(pid));
  pid = getppid();
  hash.Update((const Byte *)&pid, sizeof(pid));
  #endif

  for (int i = 0; i < 1000; i++)
  {
    #ifdef _WIN32
    LARGE_INTEGER v;
    if (::QueryPerformanceCounter(&v))
      hash.Update((const Byte *)&v.QuadPart, sizeof(v.QuadPart));
    #endif

    #ifdef USE_POSIX_TIME
    #ifdef USE_POSIX_TIME2
    timeval v;
    if (gettimeofday(&v, 0) == 0)
    {
      hash.Update((const Byte *)&v.tv_sec, sizeof(v.tv_sec));
      hash.Update((const Byte *)&v.tv_usec, sizeof(v.tv_usec));
    }
    #endif
    time_t v2 = time(NULL);
    hash.Update((const Byte *)&v2, sizeof(v2));
    #endif

    DWORD tickCount = ::GetTickCount();
    hash.Update((const Byte *)&tickCount, sizeof(tickCount));
    
    for (int j = 0; j < 100; j++)
    {
      hash.Final(_buff);
      hash.Init();
      hash.Update(_buff, NCrypto::NSha1::kDigestSize);
    }
  }
  hash.Final(_buff);
  _needInit = false;
}

static NWindows::NSynchronization::CCriticalSection g_CriticalSection;

void CRandomGenerator::Generate(Byte *data, unsigned int size)
{ 
  g_CriticalSection.Enter();
  if (_needInit)
    Init();
  while (size > 0)
  {
    NCrypto::NSha1::CContext hash;
    
    hash.Init();
    hash.Update(_buff, NCrypto::NSha1::kDigestSize);
    hash.Final(_buff);
    
    hash.Init();
    UInt32 salt = 0xF672ABD1;
    hash.Update((const Byte *)&salt, sizeof(salt));
    hash.Update(_buff, NCrypto::NSha1::kDigestSize);
    Byte buff[NCrypto::NSha1::kDigestSize];
    hash.Final(buff);
    for (unsigned int i = 0; i < NCrypto::NSha1::kDigestSize && size > 0; i++, size--)
      *data++ = buff[i];
  }
  g_CriticalSection.Leave();
}

CRandomGenerator g_RandomGenerator;
