// Common/Random.cpp

#include "StdAfx.h"

#include <time.h>

#include "Common/Random.h"

void CRandom::Init(unsigned int anInit)
{
  srand(anInit);
}

void CRandom::Init()
{
  Init(time(NULL));
}

int CRandom::Generate() const
{
  return rand();
}
