// Common/StdOutStream.cpp

#include "StdAfx.h"

#include "StdOutStream.h"

static const char kNewLineChar =  '\n';

static const char *kOneStringFormat = "%s";
static const char *kOneIntFormat = "%d";
static const char *kOneUINT32Format = "%u";
static const char *kOneUINT64Format = "%I64u";

static LPCTSTR kFileOpenMode = _T("wt");

CStdOutStream  g_StdOut(stdout);
CStdOutStream  g_StdErr(stderr);

bool CStdOutStream::Open(LPCTSTR fileName)
{
  Close();
  _stream = _tfopen(fileName, kFileOpenMode);
  _streamIsOpen = (_stream != 0);
  return _streamIsOpen;
}

bool CStdOutStream::Close()
{
  if(!_streamIsOpen)
    return true;
  _streamIsOpen = (fclose(_stream) != 0);
  return !_streamIsOpen;
}

CStdOutStream ::~CStdOutStream ()
{
  Close();
}


CStdOutStream & CStdOutStream ::operator<<(CStdOutStream & (*aFunction)(CStdOutStream  &))
{
  (*aFunction)(*this);    
  return *this;
}

CStdOutStream & endl(CStdOutStream & outStream)
{
  return outStream << kNewLineChar;
}

CStdOutStream & CStdOutStream::operator<<(const char *string)
{
  fprintf(_stream, kOneStringFormat, string);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(char c)
{
  putc(c, _stream);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(int number)
{
  fprintf(_stream, kOneIntFormat, number);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(UINT32 number)
{
  fprintf(_stream, kOneUINT32Format, number);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(UINT64 number)
{
  fprintf(_stream, kOneUINT64Format, number);
  return *this;
}
