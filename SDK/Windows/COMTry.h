// COMTRY.h

#pragma once 

#ifndef __COMTRY_H
#define __COMTRY_H

#include "Common/NewHandler.h"

#define COM_TRY_BEGIN   try {

#define COM_TRY_END  } catch(CNewException) {  return E_OUTOFMEMORY; }\
  catch(...) { return E_FAIL; }

#endif
