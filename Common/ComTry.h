// ComTry.h

// #pragma once 

#ifndef __Com_Try_H
#define __Com_Try_H

#include "Exception.h"

#define COM_TRY_BEGIN try {
#define COM_TRY_END } catch(const CSystemException &e) { return e.ErrorCode; }\
    catch(...) { return E_FAIL; }

#endif
