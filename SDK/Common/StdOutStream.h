// Common/StdOutStream.h

#pragma once 

#ifndef __COMMON_STDOUTSTREAM_H
#define __COMMON_STDOUTSTREAM_H

class CStdOutStream 
{
  bool _streamIsOpen;
  FILE *_stream;
public:
  CStdOutStream (): _streamIsOpen(false) {};
  CStdOutStream (FILE *stream): _streamIsOpen(false), _stream(stream) {};
  ~CStdOutStream ();
  bool Open(LPCTSTR fileName);
  bool Close();
 
  CStdOutStream & operator<<(CStdOutStream & (* aFunction)(CStdOutStream  &));
  CStdOutStream & operator<<(const char *string);
  CStdOutStream & operator<<(char c);
  CStdOutStream & operator<<(int number);
  CStdOutStream & operator<<(UINT32 number);
  CStdOutStream & operator<<(UINT64 number);
};

CStdOutStream & endl(CStdOutStream & outStream);

extern CStdOutStream g_StdOut;
extern CStdOutStream g_StdErr;

#endif
