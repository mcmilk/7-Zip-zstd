// NewHandler.cpp
 
#include "StdAfx.h"

#include "NewHandler.h"

int MemErrorVC(size_t)
{
  throw CNewException();
  // return 1;
}

CNewHandlerSetter::CNewHandlerSetter()
{
  MemErrorOldVCFunction = _set_new_handler(MemErrorVC);
}

CNewHandlerSetter::~CNewHandlerSetter()
{
  _set_new_handler(MemErrorOldVCFunction);
}
