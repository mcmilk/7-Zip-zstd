// ArchiveExports.cpp

#include "StdAfx.h"

#include "../../Common/ComTry.h"
#include "../../Common/Types.h"
#include "../../Windows/PropVariant.h"
#include "../Common/RegisterArc.h"

#include "IArchive.h"
#include "../ICoder.h"
#include "../IPassword.h"

static const unsigned int kNumArcsMax = 32;
static unsigned int g_NumArcs = 0;
static const CArcInfo *g_Arcs[kNumArcsMax]; 
void RegisterArc(const CArcInfo *arcInfo) 
{ 
  if (g_NumArcs < kNumArcsMax)
    g_Arcs[g_NumArcs++] = arcInfo; 
}

DEFINE_GUID(CLSID_CArchiveHandler, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00);

#define CLS_ARC_ID_ITEM(cls) ((cls).Data4[5])

static inline HRESULT SetPropString(const char *s, unsigned int size, PROPVARIANT *value)
{
  if ((value->bstrVal = ::SysAllocStringByteLen(s, size)) != 0)
    value->vt = VT_BSTR;
  return S_OK;
}

static inline HRESULT SetPropGUID(const GUID &guid, PROPVARIANT *value)
{
  return SetPropString((const char *)&guid, sizeof(GUID), value);
}

int FindFormatCalssId(const GUID *clsID)
{
  GUID cls = *clsID;
  CLS_ARC_ID_ITEM(cls) = 0;
  if (cls != CLSID_CArchiveHandler)
    return -1;
  Byte id = CLS_ARC_ID_ITEM(*clsID);
  for (UInt32 i = 0; i < g_NumArcs; i++)
    if (g_Arcs[i]->ClassId == id)
      return i;
  return -1;
}

STDAPI CreateArchiver(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  {
    int needIn = (*iid == IID_IInArchive);
    int needOut = (*iid == IID_IOutArchive);
    if (!needIn && !needOut)
      return E_NOINTERFACE;
    int formatIndex = FindFormatCalssId(clsid);
    if (formatIndex < 0)
      return CLASS_E_CLASSNOTAVAILABLE;
    
    const CArcInfo &arc = *g_Arcs[formatIndex];
    if (needIn)
    {
      *outObject = arc.CreateInArchive();
      ((IInArchive *)*outObject)->AddRef();
    }
    else
    {
      if (!arc.CreateOutArchive)
        return CLASS_E_CLASSNOTAVAILABLE;
      *outObject = arc.CreateOutArchive();
      ((IOutArchive *)*outObject)->AddRef();
    }
  }
  COM_TRY_END
  return S_OK;
}

STDAPI GetHandlerProperty2(UInt32 formatIndex, PROPID propID, PROPVARIANT *value)
{
  if (formatIndex >= g_NumArcs)
    return E_INVALIDARG;
  const CArcInfo &arc = *g_Arcs[formatIndex];
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case NArchive::kName:
      prop = arc.Name;
      break;
    case NArchive::kClassID:
    {
      GUID clsId = CLSID_CArchiveHandler;
      CLS_ARC_ID_ITEM(clsId) = arc.ClassId;
      return SetPropGUID(clsId, value);
    }
    case NArchive::kExtension:
      if (arc.Ext != 0)
        prop = arc.Ext;
      break;
    case NArchive::kAddExtension:
      if (arc.AddExt != 0)
        prop = arc.AddExt;
      break;
    case NArchive::kUpdate:
      prop = (bool)(arc.CreateOutArchive != 0);
      break;
    case NArchive::kKeepName:
      prop = arc.KeepName;
      break;
    case NArchive::kStartSignature:
      return SetPropString((const char *)arc.Signature, arc.SignatureSize, value);
  }
  prop.Detach(value);
  return S_OK;
}

STDAPI GetHandlerProperty(PROPID propID, PROPVARIANT *value)
{
  return GetHandlerProperty2(0, propID, value);
}

STDAPI GetNumberOfFormats(UINT32 *numFormats)
{
  *numFormats = g_NumArcs;
  return S_OK;
}
