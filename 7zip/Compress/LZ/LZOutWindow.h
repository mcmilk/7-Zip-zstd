// LZOutWindow.h

// #pragma once

#ifndef __LZ_OUT_WINDOW_H
#define __LZ_OUT_WINDOW_H

#include "../../IStream.h"

// m_KeepSizeBefore: how mach BYTEs must be in buffer before _pos;
// m_KeepSizeAfter: how mach BYTEs must be in buffer after _pos;
// m_KeepSizeReserv: how mach BYTEs must be in buffer for Moving Reserv; 
//                    must be >= aKeepSizeAfter; // test it

class CLZOutWindowException
{
public:
  HRESULT ErrorCode;
  CLZOutWindowException(HRESULT errorCode): ErrorCode(errorCode) {}
};

class CLZOutWindow
{
  BYTE  *_buffer;
  UINT32 _pos;
  UINT32 _windowSize;
  UINT32 _streamPos;
  ISequentialOutStream *_stream;
  void FlushWithCheck();

public:
  CLZOutWindow(): _buffer(0), _stream(0) {}
  ~CLZOutWindow();
	void Create(UINT32 windowSize);
  bool IsCreated() const { return _buffer != 0; }

  void Init(ISequentialOutStream *stream, bool solid = false);
  HRESULT Flush();
  // void ReleaseStream();
  
  // UINT32 GetCurPos() const { return _pos; }
  // const BYTE *GetPointerToCurrentPos() const { return _buffer + _pos;};

  void CopyBackBlock(UINT32 distance, UINT32 len)
  {
		UINT32 pos = _pos - distance - 1;
  	if (pos >= _windowSize)
  		pos += _windowSize;
		for(; len > 0; len--)
		{
			if (pos >= _windowSize)
				pos = 0;
			_buffer[_pos++] = _buffer[pos++];
			if (_pos >= _windowSize)
				FlushWithCheck();  
			// PutOneByte(GetOneByte(0 - distance));
		}
  }

  void PutOneByte(BYTE b)
  {
		_buffer[_pos++] = b;
		if (_pos >= _windowSize)
			FlushWithCheck();  
  }

  BYTE GetOneByte(UINT32 index) const
  {
		UINT32 pos = _pos + index;
		if (pos >= _windowSize)
			pos += _windowSize;
		return _buffer[pos]; 
  }

  // BYTE *GetBuffer() const { return _buffer; }
};

#endif
