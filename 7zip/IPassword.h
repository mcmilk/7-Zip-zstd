// IPassword.h

#ifndef __IPASSWORD_H
#define __IPASSWORD_H

#include "../Common/MyUnknown.h"
#include "../Common/Types.h"

// {23170F69-40C1-278A-0000-000200250000}
DEFINE_GUID(IID_ICryptoSetPassword, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200250000")
ICryptoSetPassword: public IUnknown
{
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size) PURE;
};

// {23170F69-40C1-278A-0000-000200251000}
DEFINE_GUID(IID_ICryptoSetCRC, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x10, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200251000")
ICryptoSetCRC: public IUnknown
{
  STDMETHOD(CryptoSetCRC)(UInt32 crc) PURE;
};

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
  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password) PURE;
};

#endif

