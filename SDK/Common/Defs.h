// Common/Defs.h

#pragma once

#ifndef __COMMON_DEFS_H
#define __COMMON_DEFS_H

template <class T> inline T MyMin(T a, T b)
  {  return a < b ? a : b; }
template <class T> inline T MyMax(T a, T b)
  {  return a > b ? a : b; }

template <class T> inline int MyCompare(T a, T b)
  {  return a < b ? -1 : (a == b ? 0 : 1); }

#endif
