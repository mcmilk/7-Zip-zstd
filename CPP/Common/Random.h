// Common/Random.h

#ifndef ZIP7_INC_COMMON_RANDOM_H
#define ZIP7_INC_COMMON_RANDOM_H

class CRandom
{
public:
  void Init();
  void Init(unsigned int seed);
  int Generate() const;
};

#endif
