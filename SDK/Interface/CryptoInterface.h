// CryptoInterface.h

#pragma once

#ifndef __CRYPTOINTERFACE_H
#define __CRYPTOINTERFACE_H

#include "Common/Types.h"

// {23170F69-40C1-278A-0000-000200270000}
DEFINE_GUID(IID_ICryptoGetTextPassword, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x27, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200270000")
ICryptoGetTextPassword: public IUnknown
{
  STDMETHOD(CryptoGetTextPassword)(BSTR *password) PURE;
};

// {23170F69-40C1-278A-0000-000200270200}
DEFINE_GUID(IID_ICryptoGetTextPassword2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x27, 0x02, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200270200")
ICryptoGetTextPassword2: public IUnknown
{
  STDMETHOD(CryptoGetTextPassword2)(INT32 *passwordIsDefined, BSTR *password) PURE;
};

#endif

