/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              
 
 *  modified by Igor Pavlov (2004-08-29).
 ****************************************************************************/
#ifndef __PPMD_TYPE_H
#define __PPMD_TYPE_H

const int kMaxOrderCompress = 32;
const int MAX_O = 255; /* maximum allowed model order */

template <class T>
inline void _PPMD_SWAP(T& t1,T& t2) { T tmp = t1; t1 = t2; t2 = tmp; }

#endif
