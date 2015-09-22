// VmdkHandler.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "../../../C/CpuArch.h"

#include "../../Common/ComTry.h"
#include "../../Common/IntToString.h"

#include "../../Windows/PropVariant.h"

#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/ZlibDecoder.h"

#include "HandlerCont.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

using namespace NWindows;

namespace NArchive {
namespace NVmdk {

#define SIGNATURE { 'K', 'D', 'M', 'V' }
  
static const Byte k_Signature[] = SIGNATURE;

static const UInt32 k_Flags_NL = (UInt32)1 << 0;
static const UInt32 k_Flags_RGD = (UInt32)1 << 1;
static const UInt32 k_Flags_ZeroGrain = (UInt32)1 << 2;
static const UInt32 k_Flags_Compressed = (UInt32)1 << 16;
static const UInt32 k_Flags_Marker = (UInt32)1 << 17;

static const unsigned k_NumMidBits = 9; // num bits for index in Grain Table

struct CHeader
{
  UInt32 flags;
  UInt32 version;

  UInt64 capacity;
  UInt64 grainSize;
  UInt64 descriptorOffset;
  UInt64 descriptorSize;

  UInt32 numGTEsPerGT;
  UInt16 algo;
  // Byte uncleanShutdown;
  // UInt64 rgdOffset;
  UInt64 gdOffset;
  UInt64 overHead;

  bool Is_NL() const { return (flags & k_Flags_NL) != 0; };
  bool Is_ZeroGrain() const { return (flags & k_Flags_ZeroGrain) != 0; };
  bool Is_Compressed() const { return (flags & k_Flags_Compressed) != 0; };
  bool Is_Marker() const { return (flags & k_Flags_Marker) != 0; };

  bool Parse(const Byte *buf);

  bool IsSameImageFor(const CHeader &h) const
  {
    return flags == h.flags
        && version == h.version
        && capacity == h.capacity
        && grainSize == h.grainSize
        && algo == h.algo;
  }
};

bool CHeader::Parse(const Byte *buf)
{
  if (memcmp(buf, k_Signature, sizeof(k_Signature)) != 0)
    return false;

  version = Get32(buf + 0x4);
  flags = Get32(buf + 0x8);
  capacity = Get64(buf + 0xC);
  grainSize = Get64(buf + 0x14);
  descriptorOffset = Get64(buf + 0x1C);
  descriptorSize = Get64(buf + 0x24);
  numGTEsPerGT = Get32(buf + 0x2C);
  // rgdOffset = Get64(buf + 0x30);
  gdOffset = Get64(buf + 0x38);
  overHead = Get64(buf + 0x40);
  // uncleanShutdown = buf[0x48];
  algo = Get16(buf + 0x4D);

  if (Is_NL() && Get32(buf + 0x49) != 0x0A0D200A) // do we need Is_NL() check here?
    return false;

  return (numGTEsPerGT == (1 << k_NumMidBits)) && (version <= 3);
}


enum
{
  k_Marker_END_OF_STREAM = 0,
  k_Marker_GRAIN_TABLE   = 1,
  k_Marker_GRAIN_DIR     = 2,
  k_Marker_FOOTER        = 3
};

struct CMarker
{
  UInt64 NumSectors;
  UInt32 SpecSize; // = 0 for metadata sectors
  UInt32 Type;

  void Parse(const Byte *p)
  {
    NumSectors = Get64(p);
    SpecSize = Get32(p + 8);
    Type = Get32(p + 12);
  }
};


struct CDescriptor
{
  AString CID;
  AString parentCID;
  AString createType;

  AStringVector Extents;
  
  void Clear()
  {
    CID.Empty();
    parentCID.Empty();
    createType.Empty();
    Extents.Clear();
  }

  void Parse(const Byte *p, size_t size);
};

static bool Str_to_ValName(const AString &s, AString &name, AString &val)
{
  name.Empty();
  val.Empty();
  int qu = s.Find('"');
  int eq = s.Find('=');
  if (eq < 0 || (qu >= 0 && eq > qu))
    return false;
  name = s.Left(eq);
  name.Trim();
  val = s.Ptr(eq + 1);
  val.Trim();
  return true;
}

void CDescriptor::Parse(const Byte *p, size_t size)
{
  Clear();

  AString s;
  AString name;
  AString val;
  
  for (size_t i = 0;; i++)
  {
    char c = p[i];
    if (i == size || c == 0 || c == 0xA || c == 0xD)
    {
      if (!s.IsEmpty() && s[0] != '#')
      {
        if (Str_to_ValName(s, name, val))
        {
          if (name.IsEqualTo_Ascii_NoCase("CID"))
            CID = val;
          else if (name.IsEqualTo_Ascii_NoCase("parentCID"))
            parentCID = val;
          else if (name.IsEqualTo_Ascii_NoCase("createType"))
            createType = val;
        }
        else
          Extents.Add(s);
      }
      s.Empty();
      if (c == 0 || i >= size)
        break;
    }
    else
      s += (char)c;
  }
}


class CHandler: public CHandlerImg
{
  unsigned _clusterBits;

  CObjectVector<CByteBuffer> _tables;
  UInt64 _cacheCluster;
  CByteBuffer _cache;
  CByteBuffer _cacheCompressed;

  UInt64 _phySize;

  UInt32 _zeroSector;
  bool _needDeflate;
  bool _isArc;
  bool _unsupported;
  // bool _headerError;

  CBufInStream *_bufInStreamSpec;
  CMyComPtr<ISequentialInStream> _bufInStream;

  CBufPtrSeqOutStream *_bufOutStreamSpec;
  CMyComPtr<ISequentialOutStream> _bufOutStream;

  NCompress::NZlib::CDecoder *_zlibDecoderSpec;
  CMyComPtr<ICompressCoder> _zlibDecoder;

  CByteBuffer _descriptorBuf;
  CDescriptor _descriptor;
  
  CHeader h;

  
  HRESULT Seek(UInt64 offset)
  {
    _posInArc = offset;
    return Stream->Seek(offset, STREAM_SEEK_SET, NULL);
  }

  HRESULT InitAndSeek()
  {
    _virtPos = 0;
    return Seek(0);
  }

  HRESULT ReadForHeader(IInStream *stream, UInt64 sector, void *data, size_t numSectors);
  virtual HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openCallback);

public:
  INTERFACE_IInArchive_Img(;)

  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};


STDMETHODIMP CHandler::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= _size)
    return S_OK;
  {
    UInt64 rem = _size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
    if (size == 0)
      return S_OK;
  }
 
  for (;;)
  {
    const UInt64 cluster = _virtPos >> _clusterBits;
    const size_t clusterSize = (size_t)1 << _clusterBits;
    const size_t lowBits = (size_t)_virtPos & (clusterSize - 1);
    {
      size_t rem = clusterSize - lowBits;
      if (size > rem)
        size = (UInt32)rem;
    }

    if (cluster == _cacheCluster)
    {
      memcpy(data, _cache + lowBits, size);
      _virtPos += size;
      if (processedSize)
        *processedSize = size;
      return S_OK;
    }
    
    const UInt64 high = cluster >> k_NumMidBits;
 
    if (high < _tables.Size())
    {
      const CByteBuffer &table = _tables[(unsigned)high];
    
      if (table.Size() != 0)
      {
        const size_t midBits = (size_t)cluster & ((1 << k_NumMidBits) - 1);
        const Byte *p = (const Byte *)table + (midBits << 2);
        const UInt32 v = Get32(p);
        
        if (v != 0 && v != _zeroSector)
        {
          UInt64 offset = (UInt64)v << 9;
          if (_needDeflate)
          {
            if (offset != _posInArc)
            {
              // printf("\n%12x %12x\n", (unsigned)offset, (unsigned)(offset - _posInArc));
              RINOK(Seek(offset));
            }
            
            const size_t kStartSize = 1 << 9;
            {
              size_t curSize = kStartSize;
              HRESULT res = ReadStream(Stream, _cacheCompressed, &curSize);
              _posInArc += curSize;
              RINOK(res);
              if (curSize != kStartSize)
                return S_FALSE;
            }

            if (Get64(_cacheCompressed) != (cluster << (_clusterBits - 9)))
              return S_FALSE;

            UInt32 dataSize = Get32(_cacheCompressed + 8);
            if (dataSize > ((UInt32)1 << 31))
              return S_FALSE;

            size_t dataSize2 = (size_t)dataSize + 12;
            
            if (dataSize2 > kStartSize)
            {
              dataSize2 = (dataSize2 + 511) & ~(size_t)511;
              if (dataSize2 > _cacheCompressed.Size())
                return S_FALSE;
              size_t curSize = dataSize2 - kStartSize;
              const size_t curSize2 = curSize;
              HRESULT res = ReadStream(Stream, _cacheCompressed + kStartSize, &curSize);
              _posInArc += curSize;
              RINOK(res);
              if (curSize != curSize2)
                return S_FALSE;
            }

            _bufInStreamSpec->Init(_cacheCompressed + 12, dataSize);
            
            _cacheCluster = (UInt64)(Int64)-1;
            if (_cache.Size() < clusterSize)
              return E_FAIL;
            _bufOutStreamSpec->Init(_cache, clusterSize);
            
            // Do we need to use smaller block than clusterSize for last cluster?
            UInt64 blockSize64 = clusterSize;
            HRESULT res = _zlibDecoderSpec->Code(_bufInStream, _bufOutStream, NULL, &blockSize64, NULL);

            // if (_bufOutStreamSpec->GetPos() != clusterSize)
            //   memset(_cache + _bufOutStreamSpec->GetPos(), 0, clusterSize - _bufOutStreamSpec->GetPos());

            if (res == S_OK)
              if (_bufOutStreamSpec->GetPos() != clusterSize
                  || _zlibDecoderSpec->GetInputProcessedSize() != dataSize)
                res = S_FALSE;

            RINOK(res);
            _cacheCluster = cluster;
            
            continue;
            /*
            memcpy(data, _cache + lowBits, size);
            _virtPos += size;
            if (processedSize)
              *processedSize = size;
            return S_OK;
            */
          }
          {
            offset += lowBits;
            if (offset != _posInArc)
            {
              // printf("\n%12x %12x\n", (unsigned)offset, (unsigned)(offset - _posInArc));
              RINOK(Seek(offset));
            }
            HRESULT res = Stream->Read(data, size, &size);
            _posInArc += size;
            _virtPos += size;
            if (processedSize)
              *processedSize = size;
            return res;
          }
        }
      }
    }
    
    memset(data, 0, size);
    _virtPos += size;
    if (processedSize)
      *processedSize = size;
    return S_OK;
  }
}


static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidClusterSize,
  kpidHeadersSize,
  kpidMethod,
  kpidId,
  kpidName,
  kpidComment
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps


STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  
  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidPhySize: if (_phySize != 0) prop = _phySize; break;
    case kpidClusterSize: prop = (UInt32)1 << _clusterBits; break;
    case kpidHeadersSize: prop = (h.overHead << 9); break;
    case kpidMethod:
    {
      AString s;

      if (!_descriptor.createType.IsEmpty())
        s = _descriptor.createType;
      
      if (h.algo != 0)
      {
        s.Add_Space_if_NotEmpty();
        if (h.algo == 1)
          s += "zlib";
        else
        {
          char temp[16];
          ConvertUInt32ToString(h.algo, temp);
          s += temp;
        }
      }
      
      if (h.Is_Marker())
      {
        s.Add_Space_if_NotEmpty();
        s += "Marker";
      }
        
      if (!s.IsEmpty())
        prop = s;
      break;
    }
    
    case kpidComment:
    {
      if (_descriptorBuf.Size() != 0)
      {
        AString s;
        s.SetFrom_CalcLen((const char *)(const Byte *)_descriptorBuf, (unsigned)_descriptorBuf.Size());
        if (!s.IsEmpty() && s.Len() <= (1 << 16))
          prop = s;
      }
      break;
    }
    
    case kpidId:
      if (!_descriptor.CID.IsEmpty())
      {
        prop = _descriptor.CID;
        break;
      }

    case kpidName:
    {
      if (_descriptor.Extents.Size() == 1)
      {
        const AString &s = _descriptor.Extents[0];
        if (!s.IsEmpty())
        {
          if (s.Back() == '"')
          {
            AString s2 = s;
            s2.DeleteBack();
            if (s2.Len() > 5 && StringsAreEqualNoCase_Ascii(s2.RightPtr(5), ".vmdk"))
            {
              int pos = s2.ReverseFind('"');
              if (pos >= 0)
              {
                s2.DeleteFrontal(pos + 1);
                prop = s2;
              }
            }
          }
        }
      }
      break;
    }
    
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;;
      if (_unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
      // if (_headerError) v |= kpv_ErrorFlags_HeadersError;
      if (!Stream && v == 0 && _isArc)
        v = kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }
  }
  
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidSize: prop = _size; break;
    case kpidPackSize:
    {
      UInt64 ov = (h.overHead << 9);
      if (_phySize >= ov)
        prop = _phySize - ov;
      break;
    }
    case kpidExtension: prop = (_imgExt ? _imgExt : "img"); break;
  }
  
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


static int inline GetLog(UInt64 num)
{
  for (int i = 0; i < 64; i++)
    if (((UInt64)1 << i) == num)
      return i;
  return -1;
}


HRESULT CHandler::ReadForHeader(IInStream *stream, UInt64 sector, void *data, size_t numSectors)
{
  sector <<= 9;
  RINOK(stream->Seek(sector, STREAM_SEEK_SET, NULL));
  size_t size = numSectors << 9;
  RINOK(ReadStream_FALSE(stream, data, size));
  UInt64 end = sector + size;
  if (_phySize < end)
    _phySize = end;
  return S_OK;
}


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *openCallback)
{
  const unsigned kSectoreSize = 512;
  Byte buf[kSectoreSize];
  size_t headerSize = kSectoreSize;
  RINOK(ReadStream(stream, buf, &headerSize));

  if (headerSize < sizeof(k_Signature))
    return S_FALSE;
  if (memcmp(buf, k_Signature, sizeof(k_Signature)) != 0)
  {
    const char *kSignature_Descriptor = "# Disk DescriptorFile";
    size_t k_SigDesc_Size = strlen(kSignature_Descriptor);
    if (headerSize >= k_SigDesc_Size)
      if (memcmp(buf, kSignature_Descriptor, k_SigDesc_Size) == 0)
      {
        _unsupported = true;
        _isArc = true;
        // return E_NOTIMPL;
      }
    return S_FALSE;
  }

  if (headerSize != kSectoreSize)
    return S_FALSE;

  // CHeader h;

  if (!h.Parse(buf))
    return S_FALSE;

  if (h.descriptorSize != 0)
  {
    if (h.descriptorOffset < 1)
      return S_FALSE;
    if (h.descriptorSize > (1 << 20))
      return S_FALSE;
    size_t numBytes = (size_t)h.descriptorSize << 9;
    _descriptorBuf.Alloc(numBytes);
    RINOK(ReadForHeader(stream, h.descriptorOffset, _descriptorBuf, (size_t)h.descriptorSize));
    if (h.descriptorOffset == 1 && h.Is_Marker() && Get64(_descriptorBuf) == 0)
    {
      // We check data as end marker.
      // and if probably it's footer's copy of header, we don't want to open it.
      return S_FALSE;
    }
  }

  if (h.gdOffset == (UInt64)(Int64)-1)
  {
    // Grain Dir is at end of file
    UInt64 endPos;
    RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
    if ((endPos & 511) != 0)
      return S_FALSE;

    const size_t kEndSize = 512 * 3;
    Byte buf2[kEndSize];
    if (endPos < kEndSize)
      return S_FALSE;
    RINOK(stream->Seek(endPos - kEndSize, STREAM_SEEK_SET, NULL));
    RINOK(ReadStream_FALSE(stream, buf2, kEndSize));
    
    CHeader h2;
    if (!h2.Parse(buf2 + 512))
      return S_FALSE;
    if (!h.IsSameImageFor(h2))
      return S_FALSE;

    h = h2;

    CMarker m;
    m.Parse(buf2);
    if (m.NumSectors != 1 || m.SpecSize != 0 || m.Type != k_Marker_FOOTER)
      return S_FALSE;
    m.Parse(buf2 + 512 * 2);
    if (m.NumSectors != 0 || m.SpecSize != 0 || m.Type != k_Marker_END_OF_STREAM)
      return S_FALSE;
    _phySize = endPos;
  }

  int grainSize_Log = GetLog(h.grainSize);
  if (grainSize_Log < 3 || grainSize_Log > 30 - 9) // grain size must be >= 4 KB
    return S_FALSE;
  if (h.capacity >= ((UInt64)1 << (63 - 9)))
    return S_FALSE;
  if (h.overHead >= ((UInt64)1 << (63 - 9)))
    return S_FALSE;

  _isArc = true;
  _clusterBits = (9 + grainSize_Log);
  _size = h.capacity << 9;
  _needDeflate = (h.algo >= 1);

  if (h.Is_Compressed() ? (h.algo > 1 || !h.Is_Marker()) : (h.algo != 0))
  {
    _unsupported = true;
    _phySize = 0;
    return S_FALSE;
  }

  {
    UInt64 overHeadBytes = h.overHead << 9;
    if (_phySize < overHeadBytes)
      _phySize = overHeadBytes;
  }

  _zeroSector = 0;
  if (h.Is_ZeroGrain())
    _zeroSector = 1;

  const UInt64 numSectorsPerGde = (UInt64)1 << (grainSize_Log + k_NumMidBits);
  const UInt64 numGdeEntries = (h.capacity + numSectorsPerGde - 1) >> (grainSize_Log + k_NumMidBits);
  CByteBuffer table;
  
  if (numGdeEntries != 0)
  {
    if (h.gdOffset == 0)
      return S_FALSE;

    size_t numSectors = (size_t)((numGdeEntries + ((1 << (9 - 2)) - 1)) >> (9 - 2));
    size_t t1SizeBytes = numSectors << 9;
    if ((t1SizeBytes >> 2) < numGdeEntries)
      return S_FALSE;
    table.Alloc(t1SizeBytes);

    if (h.Is_Marker())
    {
      Byte buf2[1 << 9];
      if (ReadForHeader(stream, h.gdOffset - 1, buf2, 1) != S_OK)
        return S_FALSE;
      {
        CMarker m;
        m.Parse(buf2);
        if (m.Type != k_Marker_GRAIN_DIR
            || m.NumSectors != numSectors
            || m.SpecSize != 0)
          return S_FALSE;
      }
    }

    RINOK(ReadForHeader(stream, h.gdOffset, table, numSectors));
  }

  size_t clusterSize = (size_t)1 << _clusterBits;

  if (openCallback)
  {
    UInt64 totalBytes = (UInt64)numGdeEntries << (k_NumMidBits + 2);
    RINOK(openCallback->SetTotal(NULL, &totalBytes));
  }

  UInt64 lastSector = 0;
  UInt64 lastVirtCluster = 0;
  size_t numProcessed_Prev = 0;

  for (size_t i = 0; i < numGdeEntries; i++)
  {
    UInt32 v = Get32((const Byte *)table + (size_t)i * 4);
    CByteBuffer &buf = _tables.AddNew();
    if (v == 0 || v == _zeroSector)
      continue;

    if (openCallback && ((i - numProcessed_Prev) & 0xFFF) == 0)
    {
      UInt64 numBytes = (UInt64)i << (k_NumMidBits + 2);
      RINOK(openCallback->SetCompleted(NULL, &numBytes));
      numProcessed_Prev = i;
    }

    const size_t k_NumSectors = (size_t)1 << (k_NumMidBits - 9 + 2);
    
    if (h.Is_Marker())
    {
      Byte buf2[1 << 9];
      if (ReadForHeader(stream, v - 1, buf2, 1) != S_OK)
        return S_FALSE;
      {
        CMarker m;
        m.Parse(buf2);
        if (m.Type != k_Marker_GRAIN_TABLE
            || m.NumSectors != k_NumSectors
            || m.SpecSize != 0)
          return S_FALSE;
      }
    }

    const size_t k_NumMidItems = (size_t)1 << k_NumMidBits;

    buf.Alloc(k_NumMidItems * 4);
    RINOK(ReadForHeader(stream, v, buf, k_NumSectors));

    for (size_t k = 0; k < k_NumMidItems; k++)
    {
      UInt32 v = Get32((const Byte *)buf + (size_t)k * 4);
      if (v == 0 || v == _zeroSector)
        continue;
      if (v < h.overHead)
        return S_FALSE;
      if (lastSector < v)
      {
        lastSector = v;
        if (_needDeflate)
          lastVirtCluster = ((UInt64)i << k_NumMidBits) + k;
      }
    }
  }


  if (!_needDeflate)
  {
    UInt64 end = ((UInt64)lastSector << 9) + clusterSize;
    if (_phySize < end)
      _phySize = end;
  }
  else if (lastSector != 0)
  {
    Byte buf[1 << 9];
    if (ReadForHeader(stream, lastSector, buf, 1) == S_OK)
    {
      UInt64 lba = Get64(buf);
      if (lba == (lastVirtCluster << (_clusterBits - 9)))
      {
        UInt32 dataSize = Get32(buf + 8);
        size_t dataSize2 = (size_t)dataSize + 12;
        dataSize2 = (dataSize2 + 511) & ~(size_t)511;
        UInt64 end = ((UInt64)lastSector << 9) + dataSize2;
        if (_phySize < end)
          _phySize = end;
      }
    }
  }

  if (_descriptorBuf.Size() != 0)
  {
    _descriptor.Parse(_descriptorBuf, _descriptorBuf.Size());
    if (!_descriptor.parentCID.IsEmpty())
      if (!_descriptor.parentCID.IsEqualTo_Ascii_NoCase("ffffffff"))
        _unsupported = true;
  }

  Stream = stream;
  return S_OK;
}


STDMETHODIMP CHandler::Close()
{
  _phySize = 0;
  _size = 0;
  _cacheCluster = (UInt64)(Int64)-1;
  _zeroSector = 0;
  _clusterBits = 0;

  _needDeflate = false;
  _isArc = false;
  _unsupported = false;
  // _headerError = false;

  _tables.Clear();
  _descriptorBuf.Free();
  _descriptor.Clear();

  _imgExt = NULL;
  Stream.Release();
  return S_OK;
}


STDMETHODIMP CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  *stream = 0;

  if (_unsupported)
    return S_FALSE;


  if (_needDeflate)
  {
    if (!_bufInStream)
    {
      _bufInStreamSpec = new CBufInStream;
      _bufInStream = _bufInStreamSpec;
    }
    
    if (!_bufOutStream)
    {
      _bufOutStreamSpec = new CBufPtrSeqOutStream();
      _bufOutStream = _bufOutStreamSpec;
    }

    if (!_zlibDecoder)
    {
      _zlibDecoderSpec = new NCompress::NZlib::CDecoder;
      _zlibDecoder = _zlibDecoderSpec;
    }
    
    size_t clusterSize = (size_t)1 << _clusterBits;
    _cache.AllocAtLeast(clusterSize);
    _cacheCompressed.AllocAtLeast(clusterSize * 2);
  }

  CMyComPtr<ISequentialInStream> streamTemp = this;
  RINOK(InitAndSeek());
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}


REGISTER_ARC_I(
  "VMDK", "vmdk", NULL, 0xC8,
  k_Signature,
  0,
  0,
  NULL)

}}
