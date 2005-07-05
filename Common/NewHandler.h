// Common/NewHandler.h

#ifndef __COMMON_NEWHANDLER_H
#define __COMMON_NEWHANDLER_H

class CNewException {};

void 
#ifdef _MSC_VER
__cdecl 
#endif
operator delete(void *p) throw();

#endif 
