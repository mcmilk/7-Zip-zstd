// IInOutStreams.h

#pragma once

#ifndef __IINOUTSTREAMS_H
#define __IINOUTSTREAMS_H

// {23170F69-40C1-278A-0000-000000010000}
DEFINE_GUID(IID_ISequentialInStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000010000")
ISequentialInStream : public IUnknown
{
public:
  // out: if (aProcessedSize == 0) then there are no more bytes
  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize) = 0;
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize) = 0;
};

// {23170F69-40C1-278A-0000-000000020000}
DEFINE_GUID(IID_ISequentialOutStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000020000")
ISequentialOutStream : public IUnknown
{
public:
  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize) = 0;
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize) = 0;
};

// {23170F69-40C1-278A-0000-000000030000}
DEFINE_GUID(IID_IInStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000030000")
IInStream : public ISequentialInStream
{
public:
  STDMETHOD(Seek)(INT64 anOffset, UINT32 aSeekOrigin, UINT64 *aNewPosition) = 0;
};

// {23170F69-40C1-278A-0000-000000040000}
DEFINE_GUID(IID_IOutStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000040000")
IOutStream : public ISequentialOutStream
{
public:
  STDMETHOD(Seek)(INT64 anOffset, UINT32 aSeekOrigin, UINT64 *aNewPosition) = 0;
  STDMETHOD(SetSize)(INT64 aNewSize) = 0;
};

#endif
