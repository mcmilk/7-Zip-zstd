// PpmdType.h
// 2009-05-30 : Igor Pavlov : Public domain
// This code is based on Dmitry Shkarin's PPMdH code (public domain)

#ifndef __COMPRESS_PPMD_TYPE_H
#define __COMPRESS_PPMD_TYPE_H

const int kMaxOrderCompress = 32;
const int MAX_O = 255; /* maximum allowed model order */

template <class T>
inline void _PPMD_SWAP(T& t1,T& t2) { T tmp = t1; t1 = t2; t2 = tmp; }

#endif
