// CipherInterface.h

#pragma once

#ifndef __CIPHERINTERFACE_H
#define __CIPHERINTERFACE_H

#include "Common/Types.h"

// {23170F69-40C1-278A-0000-000200250000}
DEFINE_GUID(IID_ICryptoSetPassword, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200250000")
ICryptoSetPassword: public IUnknown
{
  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size) PURE;
};

// {23170F69-40C1-278A-0000-000200251000}
DEFINE_GUID(IID_ICryptoSetCRC, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x10, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200251000")
ICryptoSetCRC: public IUnknown
{
  STDMETHOD(CryptoSetCRC)(UINT32 crc) PURE;
};

#endif

