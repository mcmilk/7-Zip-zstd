// Common/StdOutStream.h

#pragma once 

#ifndef __COMMON_STDOUTSTREAM_H
#define __COMMON_STDOUTSTREAM_H

class CStdOutStream 
{
  bool m_StreamIsOpen;
  FILE *m_Stream;
public:
  CStdOutStream (): m_StreamIsOpen(false) {};
  CStdOutStream (FILE *aStream): m_StreamIsOpen(false), m_Stream(aStream) {};
  ~CStdOutStream ();
  bool Open(LPCTSTR aFileName);
  bool Close();
 
  CStdOutStream & operator<<(CStdOutStream & (* aFunction)(CStdOutStream  &));
  CStdOutStream & operator<<(const char *aChars);
  CStdOutStream & operator<<(char aChar);
  CStdOutStream & operator<<(int aNumber);
  CStdOutStream & operator<<(UINT32 aNumber);
  CStdOutStream & operator<<(UINT64 aNumber);
};

CStdOutStream & endl(CStdOutStream & anOut);

extern CStdOutStream g_StdOut;
extern CStdOutStream g_StdErr;

#endif
