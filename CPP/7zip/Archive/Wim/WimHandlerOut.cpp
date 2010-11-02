// WimHandlerOut.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamUtils.h"

#include "../../Crypto/RandGen.h"
#include "../../Crypto/Sha1.h"

#include "WimHandler.h"

using namespace NWindows;

namespace NArchive {
namespace NWim {

struct CSha1Hash
{
  Byte Hash[kHashSize];
};

struct CHashList
{
  CRecordVector<CSha1Hash> Digests;
  CIntVector Sorted;

  int AddUnique(const CSha1Hash &h);
};

int CHashList::AddUnique(const CSha1Hash &h)
{
  int left = 0, right = Sorted.Size();
  while (left != right)
  {
    int mid = (left + right) / 2;
    int index = Sorted[mid];
    UInt32 i;
    const Byte *hash2 = Digests[index].Hash;
    for (i = 0; i < kHashSize; i++)
      if (h.Hash[i] != hash2[i])
        break;
    if (i == kHashSize)
      return index;
    if (h.Hash[i] < hash2[i])
      right = mid;
    else
      left = mid + 1;
  }
  Sorted.Insert(left, Digests.Add(h));
  return -1;
}

struct CUpdateItem
{
  UString Name;
  UInt64 Size;
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
  UInt32 Attrib;
  bool IsDir;
  int HashIndex;

  CUpdateItem(): HashIndex(-1) {}
};

struct CDir
{
  int Index;
  UString Name;
  CObjectVector<CDir> Dirs;
  CIntVector Files;
  
  CDir(): Index(-1) {}
  bool IsLeaf() const { return Index >= 0; }
  UInt64 GetNumDirs() const;
  UInt64 GetNumFiles() const;
  CDir* AddDir(CObjectVector<CUpdateItem> &items, const UString &name, int index);
};

UInt64 CDir::GetNumDirs() const
{
  UInt64 num = Dirs.Size();
  for (int i = 0; i < Dirs.Size(); i++)
    num += Dirs[i].GetNumDirs();
  return num;
}

UInt64 CDir::GetNumFiles() const
{
  UInt64 num = Files.Size();
  for (int i = 0; i < Dirs.Size(); i++)
    num += Dirs[i].GetNumFiles();
  return num;
}

CDir* CDir::AddDir(CObjectVector<CUpdateItem> &items, const UString &name, int index)
{
  int left = 0, right = Dirs.Size();
  while (left != right)
  {
    int mid = (left + right) / 2;
    CDir &d = Dirs[mid];
    int compare = name.CompareNoCase(d.IsLeaf() ? items[Dirs[mid].Index].Name : d.Name);
    if (compare == 0)
    {
      if (index >= 0)
        d.Index = index;
      return &d;
    }
    if (compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
  Dirs.Insert(left, CDir());
  CDir &d = Dirs[left];
  d.Index = index;
  if (index < 0)
    d.Name = name;
  return &d;
}


STDMETHODIMP COutHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kWindows;
  return S_OK;
}

static HRESULT GetTime(IArchiveUpdateCallback *callback, int index, PROPID propID, FILETIME &ft)
{
  ft.dwLowDateTime = ft.dwHighDateTime = 0;
  NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(index, propID, &prop));
  if (prop.vt == VT_FILETIME)
    ft = prop.filetime;
  else if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  return S_OK;
}

#define Set16(p, d) SetUi16(p, d)
#define Set32(p, d) SetUi32(p, d)
#define Set64(p, d) SetUi64(p, d)

void CResource::WriteTo(Byte *p) const
{
  Set64(p, PackSize);
  p[7] = Flags;
  Set64(p + 8, Offset);
  Set64(p + 16, UnpackSize);
}

void CHeader::WriteTo(Byte *p) const
{
  memcpy(p, kSignature, kSignatureSize);
  Set32(p + 8, kHeaderSizeMax);
  Set32(p + 0xC, Version);
  Set32(p + 0x10, Flags);
  Set32(p + 0x14, ChunkSize);
  memcpy(p + 0x18, Guid, 16);
  Set16(p + 0x28, PartNumber);
  Set16(p + 0x2A, NumParts);
  Set32(p + 0x2C, NumImages);
  OffsetResource.WriteTo(p + 0x30);
  XmlResource.WriteTo(p + 0x48);
  MetadataResource.WriteTo(p + 0x60);
  IntegrityResource.WriteTo(p + 0x7C);
  Set32(p + 0x78, BootIndex);
  memset(p + 0x94, 0, 60);
}

void CStreamInfo::WriteTo(Byte *p) const
{
  Resource.WriteTo(p);
  Set16(p + 0x18, PartNumber);
  Set32(p + 0x1A, RefCount);
  memcpy(p + 0x1E, Hash, kHashSize);
}

class CInStreamWithSha1:
  public ISequentialInStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
  NCrypto::NSha1::CContext _sha;
public:
  MY_UNKNOWN_IMP1(IInStream)
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);

  void SetStream(ISequentialInStream *stream) { _stream = stream;  }
  void Init()
  {
    _size = 0;
    _sha.Init();
  }
  void ReleaseStream() { _stream.Release(); }
  UInt64 GetSize() const { return _size; }
  void Final(Byte *digest) { _sha.Final(digest); }
};

STDMETHODIMP CInStreamWithSha1::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  _sha.Update((const Byte *)data, realProcessedSize);
  if (processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}

static void SetFileTimeToMem(Byte *p, const FILETIME &ft)
{
  Set32(p, ft.dwLowDateTime);
  Set32(p + 4, ft.dwHighDateTime);
}

static size_t WriteItem(const CUpdateItem &item, Byte *p, const Byte *hash)
{
  int fileNameLen = item.Name.Length() * 2;
  int fileNameLen2 = (fileNameLen == 0 ? fileNameLen : fileNameLen + 2);

  size_t totalLen = ((kDirRecordSize + fileNameLen2 + 6) & ~7);
  if (p)
  {
    memset(p, 0, totalLen);
    Set64(p, totalLen);
    Set64(p + 8, item.Attrib);
    Set32(p + 0xC, (UInt32)(Int32)-1); // item.SecurityId
    // Set64(p + 0x10, 0); // subdirOffset
    SetFileTimeToMem(p + 0x28, item.CTime);
    SetFileTimeToMem(p + 0x30, item.ATime);
    SetFileTimeToMem(p + 0x38, item.MTime);
    if (hash)
      memcpy(p + 0x40, hash, kHashSize);
    /*
    else
      memset(p + 0x40, 0, kHashSize);
    */
    // Set16(p + 98, 0); // shortNameLen
    Set16(p + 100, (UInt16)fileNameLen);
    for (int i = 0; i * 2 < fileNameLen; i++)
      Set16(p + kDirRecordSize + i * 2, item.Name[i]);
  }
  return totalLen;
}

static void WriteTree(const CDir &tree, CRecordVector<CSha1Hash> &digests,
    CUpdateItem &defaultDirItem,
    CObjectVector<CUpdateItem> &updateItems, Byte *dest, size_t &pos)
{
  int i;
  for (i = 0; i < tree.Files.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[tree.Files[i]];
    pos += WriteItem(ui, dest ? dest + pos : NULL,
        ui.HashIndex >= 0 ? digests[ui.HashIndex].Hash : NULL);
  }

  size_t posStart = pos;
  for (i = 0; i < tree.Dirs.Size(); i++)
  {
    const CDir &subfolder = tree.Dirs[i];
    CUpdateItem *item = &defaultDirItem;
    if (subfolder.IsLeaf())
      item = &updateItems[subfolder.Index];
    else
      defaultDirItem.Name = subfolder.Name;
    pos += WriteItem(*item, NULL, NULL);
  }

  if (dest)
    Set64(dest + pos, 0);

  pos += 8;

  for (i = 0; i < tree.Dirs.Size(); i++)
  {
    const CDir &subfolder = tree.Dirs[i];
    if (dest)
    {
      CUpdateItem *item = &defaultDirItem;
      if (subfolder.IsLeaf())
        item = &updateItems[subfolder.Index];
      else
        defaultDirItem.Name = subfolder.Name;
      size_t len = WriteItem(*item, dest + posStart, NULL);
      Set64(dest + posStart + 0x10, pos);
      posStart += len;
    }
    WriteTree(subfolder, digests, defaultDirItem, updateItems, dest, pos);
  }
}

static void AddTag(AString &s, const char *name, const AString &value)
{
  s += "<";
  s += name;
  s += ">";
  s += value;
  s += "</";
  s += name;
  s += ">";
}

static void AddTagUInt64(AString &s, const char *name, UInt64 value)
{
  char temp[32];
  ConvertUInt64ToString(value, temp);
  AddTag(s, name, temp);
}

static AString TimeToXml(FILETIME &ft)
{
  AString res;
  char temp[16] = { '0', 'x' };
  ConvertUInt32ToHexWithZeros(ft.dwHighDateTime, temp + 2);
  AddTag(res, "HIGHPART", temp);
  ConvertUInt32ToHexWithZeros(ft.dwLowDateTime, temp + 2);
  AddTag(res, "LOWPART", temp);
  return res;
}

void CHeader::SetDefaultFields(bool useLZX)
{
  Version = kWimVersion;
  Flags = NHeaderFlags::kRpFix;
  ChunkSize = 0;
  if (useLZX)
  {
    Flags |= NHeaderFlags::kCompression | NHeaderFlags::kLZX;
    ChunkSize = kChunkSize;
  }
  g_RandomGenerator.Generate(Guid, 16);
  PartNumber = 1;
  NumParts = 1;
  NumImages = 1;
  BootIndex = 0;
  OffsetResource.Clear();
  XmlResource.Clear();
  MetadataResource.Clear();
  IntegrityResource.Clear();
}

static HRESULT UpdateArchive(ISequentialOutStream *seqOutStream,
    CDir &rootFolder,
    CObjectVector<CUpdateItem> &updateItems,
    IArchiveUpdateCallback *callback)
{
  CMyComPtr<IOutStream> outStream;
  RINOK(seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStream));
  if (!outStream)
    return E_NOTIMPL;

  UInt64 complexity = 0;

  int i;
  for (i = 0; i < updateItems.Size(); i++)
    complexity += updateItems[i].Size;

  RINOK(callback->SetTotal(complexity));

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(callback, true);

  complexity = 0;

  bool useCompression = false;

  CHeader header;
  header.SetDefaultFields(useCompression);
  Byte buf[kHeaderSizeMax];
  header.WriteTo(buf);
  RINOK(WriteStream(outStream, buf, kHeaderSizeMax));

  CHashList hashes;
  CObjectVector<CStreamInfo> streams;

  UInt64 curPos = kHeaderSizeMax;
  UInt64 unpackTotalSize = 0;
  for (i = 0; i < updateItems.Size(); i++)
  {
    lps->InSize = lps->OutSize = complexity;
    RINOK(lps->SetCur());

    CUpdateItem &ui = updateItems[i];
    if (ui.IsDir || ui.Size == 0)
      continue;

    CInStreamWithSha1 *inShaStreamSpec = new CInStreamWithSha1;
    CMyComPtr<ISequentialInStream> inShaStream = inShaStreamSpec;

    {
      CMyComPtr<ISequentialInStream> fileInStream;
      HRESULT res = callback->GetStream(i, &fileInStream);
      if (res != S_FALSE)
      {
        RINOK(res);
        inShaStreamSpec->SetStream(fileInStream);
        fileInStream.Release();
        inShaStreamSpec->Init();
        UInt64 offsetBlockSize = 0;
        if (useCompression)
        {
          for (UInt64 t = kChunkSize; t < ui.Size; t += kChunkSize)
          {
            Byte buf[8];
            SetUi32(buf, (UInt32)t);
            RINOK(WriteStream(outStream, buf, 4));
            offsetBlockSize += 4;
          }
        }

        RINOK(copyCoder->Code(inShaStream, outStream, NULL, NULL, progress));
        ui.Size = copyCoderSpec->TotalSize;

        CSha1Hash hash;
        unpackTotalSize += ui.Size;
        UInt64 packSize = offsetBlockSize + ui.Size;
        inShaStreamSpec->Final(hash.Hash);
        int index = hashes.AddUnique(hash);
        if (index >= 0)
        {
          ui.HashIndex = index;
          streams[index].RefCount++;
          outStream->Seek(-(Int64)packSize, STREAM_SEEK_CUR, &curPos);
          outStream->SetSize(curPos);
        }
        else
        {
          ui.HashIndex = hashes.Digests.Size() - 1;
          CStreamInfo s;
          s.Resource.PackSize = packSize;
          s.Resource.Offset = curPos;
          s.Resource.UnpackSize = ui.Size;
          s.Resource.Flags = 0;
          if (useCompression)
            s.Resource.Flags = NResourceFlags::Compressed;
          s.PartNumber = 1;
          s.RefCount = 1;
          memcpy(s.Hash, hash.Hash, kHashSize);
          streams.Add(s);
          curPos += packSize;
        }
      }
      fileInStream.Release();
      complexity += ui.Size;
      RINOK(callback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
    }
  }


  CUpdateItem ri;
  FILETIME ft;
  NTime::GetCurUtcFileTime(ft);
  ri.MTime = ri.ATime = ri.CTime = ft;
  ri.Attrib = FILE_ATTRIBUTE_DIRECTORY;

  const UInt32 kSecuritySize = 8;
  size_t pos = kSecuritySize;
  WriteTree(rootFolder, hashes.Digests, ri, updateItems, NULL, pos);
  
  CByteBuffer meta;
  meta.SetCapacity(pos);
  
  // we can write 0 here only if there is no security data, imageX does it,
  // but some programs expect size = 8
  Set32((Byte *)meta, 8); // size of security data
  Set32((Byte *)meta + 4, 0); // num security entries
  
  pos = kSecuritySize;
  WriteTree(rootFolder, hashes.Digests, ri, updateItems, (Byte *)meta, pos);

  {
    NCrypto::NSha1::CContext sha;
    sha.Init();
    sha.Update((const Byte *)meta, pos);
    CSha1Hash digest;
    sha.Final(digest.Hash);

    CStreamInfo s;
    s.Resource.PackSize = pos;
    s.Resource.Offset = curPos;
    s.Resource.UnpackSize = pos;
    s.Resource.Flags = NResourceFlags::kMetadata;
    s.PartNumber = 1;
    s.RefCount = 1;
    memcpy(s.Hash, digest.Hash, kHashSize);
    streams.Add(s);
    RINOK(WriteStream(outStream, (const Byte *)meta, pos));
    meta.Free();
    curPos += pos;
  }


  header.OffsetResource.UnpackSize = header.OffsetResource.PackSize = (UInt64)streams.Size() * kStreamInfoSize;
  header.OffsetResource.Offset = curPos;
  header.OffsetResource.Flags = NResourceFlags::kMetadata;

  for (i = 0; i < streams.Size(); i++)
  {
    Byte buf[kStreamInfoSize];
    streams[i].WriteTo(buf);
    RINOK(WriteStream(outStream, buf, kStreamInfoSize));
    curPos += kStreamInfoSize;
  }

  AString xml = "<WIM>";
  AddTagUInt64(xml, "TOTALBYTES", curPos);
  xml += "<IMAGE INDEX=\"1\"><NAME>1</NAME>";
  AddTagUInt64(xml, "DIRCOUNT", rootFolder.GetNumDirs());
  AddTagUInt64(xml, "FILECOUNT", rootFolder.GetNumFiles());
  AddTagUInt64(xml, "TOTALBYTES", unpackTotalSize);
  NTime::GetCurUtcFileTime(ft);
  AddTag(xml, "CREATIONTIME", TimeToXml(ft));
  AddTag(xml, "LASTMODIFICATIONTIME", TimeToXml(ft));
  xml += "</IMAGE></WIM>";

  size_t xmlSize = (xml.Length() + 1) * 2;
  meta.SetCapacity(xmlSize);
  Set16((Byte *)meta, 0xFEFF);
  for (i = 0; i < xml.Length(); i++)
    Set16((Byte *)meta + 2 + i * 2, xml[i]);
  RINOK(WriteStream(outStream, (const Byte *)meta, xmlSize));
  meta.Free();
  
  header.XmlResource.UnpackSize = header.XmlResource.PackSize = xmlSize;
  header.XmlResource.Offset = curPos;
  header.XmlResource.Flags = NResourceFlags::kMetadata;

  outStream->Seek(0, STREAM_SEEK_SET, NULL);
  header.WriteTo(buf);
  return WriteStream(outStream, buf, kHeaderSizeMax);
}

STDMETHODIMP COutHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *callback)
{
  COM_TRY_BEGIN
  CObjectVector<CUpdateItem> updateItems;
  CDir tree;
  tree.Dirs.Add(CDir());
  CDir &rootFolder = tree.Dirs.Back();

  for (UInt32 i = 0; i < numItems; i++)
  {
    CUpdateItem ui;
    Int32 newData, newProps;
    UInt32 indexInArchive;
    if (!callback)
      return E_FAIL;
    RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArchive));

    {
      NCOM::CPropVariant prop;
      RINOK(callback->GetProperty(i, kpidIsDir, &prop));
      if (prop.vt == VT_EMPTY)
        ui.IsDir = false;
      else if (prop.vt != VT_BOOL)
        return E_INVALIDARG;
      else
        ui.IsDir = (prop.boolVal != VARIANT_FALSE);
    }
    
    {
      NCOM::CPropVariant prop;
      RINOK(callback->GetProperty(i, kpidAttrib, &prop));
      if (prop.vt == VT_EMPTY)
        ui.Attrib = (ui.IsDir ? FILE_ATTRIBUTE_DIRECTORY : 0);
      else if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      else
        ui.Attrib = prop.ulVal;
    }
    
    RINOK(GetTime(callback, i, kpidCTime, ui.CTime));
    RINOK(GetTime(callback, i, kpidATime, ui.ATime));
    RINOK(GetTime(callback, i, kpidMTime, ui.MTime));

    {
      NCOM::CPropVariant prop;
      RINOK(callback->GetProperty(i, kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      ui.Size = prop.uhVal.QuadPart;
    }

    UString path;
    NCOM::CPropVariant prop;
    RINOK(callback->GetProperty(i, kpidPath, &prop));
    if (prop.vt == VT_BSTR)
      path = prop.bstrVal;
    else if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;

    CDir *curItem = &rootFolder;
    int len = path.Length();
    UString fileName;
    for (int j = 0; j < len; j++)
    {
      wchar_t c = path[j];
      if (c == WCHAR_PATH_SEPARATOR || c == L'/')
      {
        curItem = curItem->AddDir(updateItems, fileName, -1);
        fileName.Empty();
      }
      else
        fileName += c;
    }

    ui.Name = fileName;
    updateItems.Add(ui);
    if (ui.IsDir)
      curItem->AddDir(updateItems, fileName, (int)i);
    else
      curItem->Files.Add(i);
  }
  return UpdateArchive(outStream, tree, updateItems, callback);
  COM_TRY_END
}

}}
