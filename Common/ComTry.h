// ComTry.h

#ifndef __COM_TRY_H
#define __COM_TRY_H

#include "Exception.h"

#define COM_TRY_BEGIN try {
#define COM_TRY_END } catch(const CSystemException &e) { return e.ErrorCode; }\
    catch(...) { return E_FAIL; }

#endif
