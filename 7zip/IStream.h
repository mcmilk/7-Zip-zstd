// IStream.h

#ifndef __ISTREAMS_H
#define __ISTREAMS_H

#include "../Common/MyUnknown.h"
#include "../Common/Types.h"

// {23170F69-40C1-278A-0000-000000010000}
DEFINE_GUID(IID_ISequentialInStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000010000")
ISequentialInStream : public IUnknown
{
public:
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize) = 0;
  STDMETHOD(ReadPart)(void *data, UInt32 size, UInt32 *processedSize) = 0;
  
  // For both functions Out: if (*processedSize == 0) then 
  //   there are no more bytes in stream.
  // Read function always tries to read "size" bytes from stream. It
  // can read less only if it reaches end of stream.
  // ReadPart function can read X bytes: (0<=X<="size") and X can 
  // be less than number of remaining bytes in stream.
};

// {23170F69-40C1-278A-0000-000000020000}
DEFINE_GUID(IID_ISequentialOutStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000020000")
ISequentialOutStream : public IUnknown
{
public:
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize) = 0;
  STDMETHOD(WritePart)(const void *data, UInt32 size, UInt32 *processedSize) = 0;
};

// {23170F69-40C1-278A-0000-000000030000}
DEFINE_GUID(IID_IInStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000030000")
IInStream : public ISequentialInStream
{
public:
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) = 0;
};

// {23170F69-40C1-278A-0000-000000040000}
DEFINE_GUID(IID_IOutStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000040000")
IOutStream : public ISequentialOutStream
{
public:
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) = 0;
  STDMETHOD(SetSize)(Int64 newSize) = 0;
};

// {23170F69-40C1-278A-0000-000000060000}
DEFINE_GUID(IID_IStreamGetSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000060000")
IStreamGetSize : public IUnknown
{
public:
  STDMETHOD(GetSize)(UInt64 *size) = 0;
};

// {23170F69-40C1-278A-0000-000000070000}
DEFINE_GUID(IID_IOutStreamFlush, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000070000")
IOutStreamFlush : public IUnknown
{
public:
  STDMETHOD(Flush)() = 0;
};

#endif
