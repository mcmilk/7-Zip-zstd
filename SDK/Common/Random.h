// Common/Random.h

#pragma once

#ifndef __COMMON_RANDOM_H
#define __COMMON_RANDOM_H

class CRandom
{
public:
  void Init();
  void Init(unsigned int anInit);
  int Generate() const;
};

#endif


