// RandGen.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "Windows/Synchronization.h"

#include "RandGen.h"

// This is not very good random number generator.
// Please use it only for salt.
// First genrated data block depends from timer.
// Other genrated data blocks depend from previous state
// Maybe it's possible to restore original timer vaue from generated value.

void CRandomGenerator::Init()
{ 
  NCrypto::NSha1::CContext hash;
  hash.Init();

  #ifdef _WIN32
  DWORD w = ::GetCurrentProcessId();
  hash.Update((const Byte *)&w, sizeof(w));
  w = ::GetCurrentThreadId();
  hash.Update((const Byte *)&w, sizeof(w));
  #endif

  for (int i = 0; i < 1000; i++)
  {
    #ifdef _WIN32
    LARGE_INTEGER v;
    if (::QueryPerformanceCounter(&v))
      hash.Update((const Byte *)&v.QuadPart, sizeof(v.QuadPart));
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
