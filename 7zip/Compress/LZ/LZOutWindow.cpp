// LZOutWindow.cpp

#include "StdAfx.h"

#include "../../../Common/Alloc.h"
#include "LZOutWindow.h"

void CLZOutWindow::Init(bool solid)
{
  if(!solid)
  {
    _streamPos = 0;
    _limitPos = _bufferSize;
    _pos = 0;
    _processedSize = 0;
    _overDict = false;
  }
  #ifdef _NO_EXCEPTIONS
  ErrorCode = S_OK;
  #endif
}

void CLZOutWindow::FlushWithCheck()
{
  HRESULT result = FlushPart();
  if (_pos == _bufferSize)
  {
    _pos = 0;
    _overDict = true;
  }
  #ifdef _NO_EXCEPTIONS
  ErrorCode = result;
  #else
  if (result != S_OK)
    throw CLZOutWindowException(result);
  #endif
}


