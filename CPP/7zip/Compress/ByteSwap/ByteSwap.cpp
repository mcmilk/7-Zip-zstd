// ByteSwap.cpp

#include "StdAfx.h"

#include "ByteSwap.h"

STDMETHODIMP CByteSwap2::Init() { return S_OK; }

STDMETHODIMP_(UInt32) CByteSwap2::Filter(Byte *data, UInt32 size)
{
  const UInt32 kStep = 2;
  UInt32 i;
  for (i = 0; i + kStep <= size; i += kStep)
  {
    Byte b = data[i];
    data[i] = data[i + 1];
    data[i + 1] = b;
  }
  return i;
}

STDMETHODIMP CByteSwap4::Init() { return S_OK; }

STDMETHODIMP_(UInt32) CByteSwap4::Filter(Byte *data, UInt32 size)
{
  const UInt32 kStep = 4;
  UInt32 i;
  for (i = 0; i + kStep <= size; i += kStep)
  {
    Byte b0 = data[i];
    Byte b1 = data[i + 1];
    data[i] = data[i + 3];
    data[i + 1] = data[i + 2];
    data[i + 2] = b1;
    data[i + 3] = b0;
  }
  return i;
}
