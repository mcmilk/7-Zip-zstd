// Common/NewHandler.h

#pragma once 

#ifndef __COMMON_NEWHANDLER_H
#define __COMMON_NEWHANDLER_H

class CNewException{};

class CNewHandlerSetter
{
  _PNH MemErrorOldVCFunction;
public:
  CNewHandlerSetter();
  ~CNewHandlerSetter();
};

#endif 
