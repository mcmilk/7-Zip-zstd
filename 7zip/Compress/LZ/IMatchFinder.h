// MatchFinders/IMatchFinder.h

#ifndef __IMATCHFINDER_H
#define __IMATCHFINDER_H

// {23170F69-40C1-278A-0000-000200010000}
DEFINE_GUID(IID_IInWindowStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200010000")
IInWindowStream: public IUnknown
{
  STDMETHOD(Init)(ISequentialInStream *inStream) PURE;
  STDMETHOD_(void, ReleaseStream)() PURE;
  STDMETHOD(MovePos)() PURE;
  STDMETHOD_(Byte, GetIndexByte)(Int32 index) PURE;
  STDMETHOD_(UInt32, GetMatchLen)(Int32 index, UInt32 distance, UInt32 limit) PURE;
  STDMETHOD_(UInt32, GetNumAvailableBytes)() PURE;
  STDMETHOD_(const Byte *, GetPointerToCurrentPos)() PURE;
};
 
// {23170F69-40C1-278A-0000-000200020000}
DEFINE_GUID(IID_IMatchFinder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200020000")
IMatchFinder: public IInWindowStream
{
  STDMETHOD(Create)(UInt32 historySize, UInt32 keepAddBufferBefore, 
      UInt32 matchMaxLen, UInt32 keepAddBufferAfter) PURE;
  STDMETHOD_(UInt32, GetLongestMatch)(UInt32 *distances) PURE;
  STDMETHOD_(void, DummyLongestMatch)() PURE;
};

// {23170F69-40C1-278A-0000-000200020100}
DEFINE_GUID(IID_IMatchFinderCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x01, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200020100")
IMatchFinderCallback: public IUnknown
{
  STDMETHOD(BeforeChangingBufferPos)() PURE;
  STDMETHOD(AfterChangingBufferPos)() PURE;
};

// {23170F69-40C1-278A-0000-000200020200}
DEFINE_GUID(IID_IMatchFinderSetCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x02, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200020200")
IMatchFinderSetCallback: public IUnknown
{
  STDMETHOD(SetCallback)(IMatchFinderCallback *callback) PURE;
};

/*
// {23170F69-40C1-278A-0000-000200030000}
DEFINE_GUID(IID_IInitMatchFinder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200030000")
IMatchFinderInit: public IUnknown
{
  STDMETHOD(InitMatchFinder)(IMatchFinder *matchFinder) PURE;
};
*/

#endif
