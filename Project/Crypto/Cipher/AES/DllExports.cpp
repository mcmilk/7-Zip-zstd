// DLLExports.cpp : Implementation of DLL Exports.

#include "StdAfx.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_UUIDOF

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>

#include <initguid.h>

#include "MyAES.h"

CComModule _Module;

/*
#include "Interface/ICoder.h"

#include "Alien/Crypto/CryptoPP/crc.h"
#include "Alien/Crypto/CryptoPP/sha.h"
#include "Alien/Crypto/CryptoPP/md2.h"
#include "Alien/Crypto/CryptoPP/md5.h"
#include "Alien/Crypto/CryptoPP/ripemd.h"
#include "Alien/Crypto/CryptoPP/haval.h"
// #include "Alien/Crypto/CryptoPP/tiger.h"


using namespace CryptoPP;

#define CLSIDName(Name) CLSID_CCryptoHash ## Name
#define ClassName(Name) aClass ## Name

// {23170F69-40C1-278B-17??-000000000000}
#define MyClassID(Name, anID) DEFINE_GUID(CLSIDName(Name), \
  0x23170F69, 0x40C1, 0x278B, 0x17, anID, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

#define Pair(Name, anID) MyClassID(Name, anID)\
typedef CHash<Name, &CLSIDName(Name)> ClassName(Name);

Pair(CRC32, 1)
Pair(SHA1, 2)
Pair(SHA256, 3)
Pair(SHA384, 4)
Pair(SHA512, 5)
Pair(MD2, 6)
Pair(MD5, 7)
Pair(RIPEMD160, 8)
Pair(HAVAL, 9)
// Pair(Tiger, 18)

#define My_OBJECT_ENTRY(ID) OBJECT_ENTRY(CLSIDName(ID), ClassName(ID))

BEGIN_OBJECT_MAP(ObjectMap)
  My_OBJECT_ENTRY(CRC32)
  My_OBJECT_ENTRY(SHA1)
  My_OBJECT_ENTRY(SHA256)
  My_OBJECT_ENTRY(SHA384)
  My_OBJECT_ENTRY(SHA512)
  My_OBJECT_ENTRY(MD2)
  My_OBJECT_ENTRY(MD5)
  My_OBJECT_ENTRY(RIPEMD160)
  My_OBJECT_ENTRY(HAVAL)
//  My_OBJECT_ENTRY(Tiger)
END_OBJECT_MAP()
*/

#define MyOBJECT_ENTRY(Name) \
  OBJECT_ENTRY(CLSID_CCrypto ## Name ## _Encoder, C ## Name ## _Encoder) \
  OBJECT_ENTRY(CLSID_CCrypto ## Name ## _Decoder, C ## Name ## _Decoder) \

BEGIN_OBJECT_MAP(ObjectMap)
  MyOBJECT_ENTRY(_AES128_CBC)
  MyOBJECT_ENTRY(_AES256_CBC)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		//DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
  return _Module.RegisterServer(FALSE);
}

STDAPI DllUnregisterServer(void)
{
  return _Module.UnregisterServer();
}
