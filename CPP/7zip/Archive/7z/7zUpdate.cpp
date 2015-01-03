// 7zUpdate.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"

#include "../../../Common/Wildcard.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/OutStreamWithCRC.h"

#include "7zDecode.h"
#include "7zEncode.h"
#include "7zFolderInStream.h"
#include "7zHandler.h"
#include "7zOut.h"
#include "7zUpdate.h"

namespace NArchive {
namespace N7z {

#ifdef MY_CPU_X86_OR_AMD64
#define USE_86_FILTER
#endif

static HRESULT WriteRange(IInStream *inStream, ISequentialOutStream *outStream,
    UInt64 position, UInt64 size, ICompressProgressInfo *progress)
{
  RINOK(inStream->Seek(position, STREAM_SEEK_SET, 0));
  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
  streamSpec->SetStream(inStream);
  streamSpec->Init(size);

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  RINOK(copyCoder->Code(inStreamLimited, outStream, NULL, NULL, progress));
  return (copyCoderSpec->TotalSize == size ? S_OK : E_FAIL);
}

static int GetReverseSlashPos(const UString &name)
{
  int slashPos = name.ReverseFind(L'/');
  #ifdef _WIN32
  int slash1Pos = name.ReverseFind(L'\\');
  slashPos = MyMax(slashPos, slash1Pos);
  #endif
  return slashPos;
}

int CUpdateItem::GetExtensionPos() const
{
  int slashPos = GetReverseSlashPos(Name);
  int dotPos = Name.ReverseFind(L'.');
  if (dotPos < 0 || (dotPos < slashPos && slashPos >= 0))
    return Name.Len();
  return dotPos + 1;
}

UString CUpdateItem::GetExtension() const
{
  return Name.Ptr(GetExtensionPos());
}

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

#define RINOZ_COMP(a, b) RINOZ(MyCompare(a, b))

/*
static int CompareBuffers(const CByteBuffer &a1, const CByteBuffer &a2)
{
  size_t c1 = a1.GetCapacity();
  size_t c2 = a2.GetCapacity();
  RINOZ_COMP(c1, c2);
  for (size_t i = 0; i < c1; i++)
    RINOZ_COMP(a1[i], a2[i]);
  return 0;
}

static int CompareCoders(const CCoderInfo &c1, const CCoderInfo &c2)
{
  RINOZ_COMP(c1.NumInStreams, c2.NumInStreams);
  RINOZ_COMP(c1.NumOutStreams, c2.NumOutStreams);
  RINOZ_COMP(c1.MethodID, c2.MethodID);
  return CompareBuffers(c1.Props, c2.Props);
}

static int CompareBindPairs(const CBindPair &b1, const CBindPair &b2)
{
  RINOZ_COMP(b1.InIndex, b2.InIndex);
  return MyCompare(b1.OutIndex, b2.OutIndex);
}

static int CompareFolders(const CFolder &f1, const CFolder &f2)
{
  int s1 = f1.Coders.Size();
  int s2 = f2.Coders.Size();
  RINOZ_COMP(s1, s2);
  int i;
  for (i = 0; i < s1; i++)
    RINOZ(CompareCoders(f1.Coders[i], f2.Coders[i]));
  s1 = f1.BindPairs.Size();
  s2 = f2.BindPairs.Size();
  RINOZ_COMP(s1, s2);
  for (i = 0; i < s1; i++)
    RINOZ(CompareBindPairs(f1.BindPairs[i], f2.BindPairs[i]));
  return 0;
}
*/

/*
static int CompareFiles(const CFileItem &f1, const CFileItem &f2)
{
  return CompareFileNames(f1.Name, f2.Name);
}
*/

struct CFolderRepack
{
  int FolderIndex;
  int Group;
  CNum NumCopyFiles;
};

static int CompareFolderRepacks(const CFolderRepack *p1, const CFolderRepack *p2, void * /* param */)
{
  RINOZ_COMP(p1->Group, p2->Group);
  int i1 = p1->FolderIndex;
  int i2 = p2->FolderIndex;
  /*
  // In that version we don't want to parse folders here, so we don't compare folders
  // probably it must be improved in future
  const CDbEx &db = *(const CDbEx *)param;
  RINOZ(CompareFolders(
      db.Folders[i1],
      db.Folders[i2]));
  */
  return MyCompare(i1, i2);
  /*
  RINOZ_COMP(
      db.NumUnpackStreamsVector[i1],
      db.NumUnpackStreamsVector[i2]);
  if (db.NumUnpackStreamsVector[i1] == 0)
    return 0;
  return CompareFiles(
      db.Files[db.FolderStartFileIndex[i1]],
      db.Files[db.FolderStartFileIndex[i2]]);
  */
}

/*
  we sort empty files and dirs in such order:
  - Dir.NonAnti   (name sorted)
  - File.NonAnti  (name sorted)
  - File.Anti     (name sorted)
  - Dir.Anti (reverse name sorted)
*/

static int CompareEmptyItems(const int *p1, const int *p2, void *param)
{
  const CObjectVector<CUpdateItem> &updateItems = *(const CObjectVector<CUpdateItem> *)param;
  const CUpdateItem &u1 = updateItems[*p1];
  const CUpdateItem &u2 = updateItems[*p2];
  // NonAnti < Anti
  if (u1.IsAnti != u2.IsAnti)
    return (u1.IsAnti ? 1 : -1);
  if (u1.IsDir != u2.IsDir)
  {
    // Dir.NonAnti < File < Dir.Anti
    if (u1.IsDir)
      return (u1.IsAnti ? 1 : -1);
    return (u2.IsAnti ? -1 : 1);
  }
  int n = CompareFileNames(u1.Name, u2.Name);
  return (u1.IsDir && u1.IsAnti) ? -n : n;
}

static const char *g_Exts =
  " lzma 7z ace arc arj bz bz2 deb lzo lzx gz pak rpm sit tgz tbz tbz2 tgz cab ha lha lzh rar zoo"
  " zip jar ear war msi"
  " 3gp avi mov mpeg mpg mpe wmv"
  " aac ape fla flac la mp3 m4a mp4 ofr ogg pac ra rm rka shn swa tta wv wma wav"
  " swf "
  " chm hxi hxs"
  " gif jpeg jpg jp2 png tiff  bmp ico psd psp"
  " awg ps eps cgm dxf svg vrml wmf emf ai md"
  " cad dwg pps key sxi"
  " max 3ds"
  " iso bin nrg mdf img pdi tar cpio xpi"
  " vfd vhd vud vmc vsv"
  " vmdk dsk nvram vmem vmsd vmsn vmss vmtm"
  " inl inc idl acf asa h hpp hxx c cpp cxx rc java cs pas bas vb cls ctl frm dlg def"
  " f77 f f90 f95"
  " asm sql manifest dep "
  " mak clw csproj vcproj sln dsp dsw "
  " class "
  " bat cmd"
  " xml xsd xsl xslt hxk hxc htm html xhtml xht mht mhtml htw asp aspx css cgi jsp shtml"
  " awk sed hta js php php3 php4 php5 phptml pl pm py pyo rb sh tcl vbs"
  " text txt tex ans asc srt reg ini doc docx mcw dot rtf hlp xls xlr xlt xlw ppt pdf"
  " sxc sxd sxi sxg sxw stc sti stw stm odt ott odg otg odp otp ods ots odf"
  " abw afp cwk lwp wpd wps wpt wrf wri"
  " abf afm bdf fon mgf otf pcf pfa snf ttf"
  " dbf mdb nsf ntf wdb db fdb gdb"
  " exe dll ocx vbx sfx sys tlb awx com obj lib out o so "
  " pdb pch idb ncb opt";

static int GetExtIndex(const char *ext)
{
  int extIndex = 1;
  const char *p = g_Exts;
  for (;;)
  {
    char c = *p++;
    if (c == 0)
      return extIndex;
    if (c == ' ')
      continue;
    int pos = 0;
    for (;;)
    {
      char c2 = ext[pos++];
      if (c2 == 0 && (c == 0 || c == ' '))
        return extIndex;
      if (c != c2)
        break;
      c = *p++;
    }
    extIndex++;
    for (;;)
    {
      if (c == 0)
        return extIndex;
      if (c == ' ')
        break;
      c = *p++;
    }
  }
}

struct CRefItem
{
  const CUpdateItem *UpdateItem;
  UInt32 Index;
  UInt32 ExtensionPos;
  UInt32 NamePos;
  unsigned ExtensionIndex;
  
  CRefItem() {};
  CRefItem(UInt32 index, const CUpdateItem &ui, bool sortByType):
    UpdateItem(&ui),
    Index(index),
    ExtensionPos(0),
    NamePos(0),
    ExtensionIndex(0)
  {
    if (sortByType)
    {
      int slashPos = GetReverseSlashPos(ui.Name);
      NamePos = slashPos + 1;
      int dotPos = ui.Name.ReverseFind(L'.');
      if (dotPos < 0 || dotPos < slashPos)
        ExtensionPos = ui.Name.Len();
      else
      {
        ExtensionPos = dotPos + 1;
        if (ExtensionPos != ui.Name.Len())
        {
          AString s;
          for (unsigned pos = ExtensionPos;; pos++)
          {
            wchar_t c = ui.Name[pos];
            if (c >= 0x80)
              break;
            if (c == 0)
            {
              ExtensionIndex = GetExtIndex(s);
              break;
            }
            s += (char)MyCharLower_Ascii((char)c);
          }
        }
      }
    }
  }
};

struct CSortParam
{
  // const CObjectVector<CTreeFolder> *TreeFolders;
  bool SortByType;
};

/*
  we sort files in such order:
  - Dir.NonAnti   (name sorted)
  - alt streams
  - Dirs
  - Dir.Anti (reverse name sorted)
*/


static int CompareUpdateItems(const CRefItem *p1, const CRefItem *p2, void *param)
{
  const CRefItem &a1 = *p1;
  const CRefItem &a2 = *p2;
  const CUpdateItem &u1 = *a1.UpdateItem;
  const CUpdateItem &u2 = *a2.UpdateItem;

  /*
  if (u1.IsAltStream != u2.IsAltStream)
    return u1.IsAltStream ? 1 : -1;
  */
  
  // Actually there are no dirs that time. They were stored in other steps
  // So that code is unused?
  if (u1.IsDir != u2.IsDir)
    return u1.IsDir ? 1 : -1;
  if (u1.IsDir)
  {
    if (u1.IsAnti != u2.IsAnti)
      return (u1.IsAnti ? 1 : -1);
    int n = CompareFileNames(u1.Name, u2.Name);
    return -n;
  }
  
  // bool sortByType = *(bool *)param;
  const CSortParam *sortParam = (const CSortParam *)param;
  bool sortByType = sortParam->SortByType;
  if (sortByType)
  {
    RINOZ_COMP(a1.ExtensionIndex, a2.ExtensionIndex);
    RINOZ(CompareFileNames(u1.Name.Ptr(a1.ExtensionPos), u2.Name.Ptr(a2.ExtensionPos)));
    RINOZ(CompareFileNames(u1.Name.Ptr(a1.NamePos), u2.Name.Ptr(a2.NamePos)));
    if (!u1.MTimeDefined && u2.MTimeDefined) return 1;
    if (u1.MTimeDefined && !u2.MTimeDefined) return -1;
    if (u1.MTimeDefined && u2.MTimeDefined) RINOZ_COMP(u1.MTime, u2.MTime);
    RINOZ_COMP(u1.Size, u2.Size);
  }
  /*
  int par1 = a1.UpdateItem->ParentFolderIndex;
  int par2 = a2.UpdateItem->ParentFolderIndex;
  const CTreeFolder &tf1 = (*sortParam->TreeFolders)[par1];
  const CTreeFolder &tf2 = (*sortParam->TreeFolders)[par2];

  int b1 = tf1.SortIndex, e1 = tf1.SortIndexEnd;
  int b2 = tf2.SortIndex, e2 = tf2.SortIndexEnd;
  if (b1 < b2)
  {
    if (e1 <= b2)
      return -1;
    // p2 in p1
    int par = par2;
    for (;;)
    {
      const CTreeFolder &tf = (*sortParam->TreeFolders)[par];
      par = tf.Parent;
      if (par == par1)
      {
        RINOZ(CompareFileNames(u1.Name, tf.Name));
        break;
      }
    }
  }
  else if (b2 < b1)
  {
    if (e2 <= b1)
      return 1;
    // p1 in p2
    int par = par1;
    for (;;)
    {
      const CTreeFolder &tf = (*sortParam->TreeFolders)[par];
      par = tf.Parent;
      if (par == par2)
      {
        RINOZ(CompareFileNames(tf.Name, u2.Name));
        break;
      }
    }
  }
  */
  // RINOZ_COMP(a1.UpdateItem->ParentSortIndex, a2.UpdateItem->ParentSortIndex);
  RINOK(CompareFileNames(u1.Name, u2.Name));
  RINOZ_COMP(a1.UpdateItem->IndexInClient, a2.UpdateItem->IndexInClient);
  RINOZ_COMP(a1.UpdateItem->IndexInArchive, a2.UpdateItem->IndexInArchive);
  return 0;
}

struct CSolidGroup
{
  CRecordVector<UInt32> Indices;
};

static const wchar_t *g_ExeExts[] =
{
    L"dll"
  , L"exe"
  , L"ocx"
  , L"sfx"
  , L"sys"
};

static bool IsExeExt(const wchar_t *ext)
{
  for (int i = 0; i < ARRAY_SIZE(g_ExeExts); i++)
    if (MyStringCompareNoCase(ext, g_ExeExts[i]) == 0)
      return true;
  return false;
}


static inline void GetMethodFull(UInt64 methodID, UInt32 numInStreams, CMethodFull &m)
{
  m.Id = methodID;
  m.NumInStreams = numInStreams;
  m.NumOutStreams = 1;
}

static void AddBcj2Methods(CCompressionMethodMode &mode)
{
  CMethodFull m;
  GetMethodFull(k_LZMA, 1, m);
  
  m.AddProp32(NCoderPropID::kDictionarySize, 1 << 20);
  m.AddProp32(NCoderPropID::kNumFastBytes, 128);
  m.AddProp32(NCoderPropID::kNumThreads, 1);
  m.AddProp32(NCoderPropID::kLitPosBits, 2);
  m.AddProp32(NCoderPropID::kLitContextBits, 0);
  // m.AddPropString(NCoderPropID::kMatchFinder, L"BT2");

  mode.Methods.Add(m);
  mode.Methods.Add(m);
  
  CBind bind;
  bind.OutCoder = 0;
  bind.InStream = 0;
  bind.InCoder = 1;  bind.OutStream = 0;  mode.Binds.Add(bind);
  bind.InCoder = 2;  bind.OutStream = 1;  mode.Binds.Add(bind);
  bind.InCoder = 3;  bind.OutStream = 2;  mode.Binds.Add(bind);
}

static void MakeExeMethod(CCompressionMethodMode &mode,
    bool useFilters, bool addFilter, bool bcj2Filter)
{
  if (!mode.Binds.IsEmpty() || !useFilters || mode.Methods.Size() > 2)
    return;
  if (mode.Methods.Size() == 2)
  {
    if (mode.Methods[0].Id == k_BCJ2)
      AddBcj2Methods(mode);
    return;
  }
  if (!addFilter)
    return;
  bcj2Filter = bcj2Filter;
  #ifdef USE_86_FILTER
  if (bcj2Filter)
  {
    CMethodFull m;
    GetMethodFull(k_BCJ2, 4, m);
    mode.Methods.Insert(0, m);
    AddBcj2Methods(mode);
  }
  else
  {
    CMethodFull m;
    GetMethodFull(k_BCJ, 1, m);
    mode.Methods.Insert(0, m);
    CBind bind;
    bind.OutCoder = 0;
    bind.InStream = 0;
    bind.InCoder = 1;
    bind.OutStream = 0;
    mode.Binds.Add(bind);
  }
  #endif
}


static void FromUpdateItemToFileItem(const CUpdateItem &ui,
    CFileItem &file, CFileItem2 &file2)
{
  if (ui.AttribDefined)
    file.SetAttrib(ui.Attrib);
  
  file2.CTime = ui.CTime;  file2.CTimeDefined = ui.CTimeDefined;
  file2.ATime = ui.ATime;  file2.ATimeDefined = ui.ATimeDefined;
  file2.MTime = ui.MTime;  file2.MTimeDefined = ui.MTimeDefined;
  file2.IsAnti = ui.IsAnti;
  // file2.IsAux = false;
  file2.StartPosDefined = false;

  file.Size = ui.Size;
  file.IsDir = ui.IsDir;
  file.HasStream = ui.HasStream();
  // file.IsAltStream = ui.IsAltStream;
}

class CFolderOutStream2:
  public ISequentialOutStream,
  public CMyUnknownImp
{
  COutStreamWithCRC *_crcStreamSpec;
  CMyComPtr<ISequentialOutStream> _crcStream;
  const CDbEx *_db;
  const CBoolVector *_extractStatuses;
  CMyComPtr<ISequentialOutStream> _outStream;
  UInt32 _startIndex;
  unsigned _currentIndex;
  bool _fileIsOpen;
  UInt64 _rem;

  void OpenFile();
  void CloseFile();
  HRESULT CloseFileAndSetResult();
  HRESULT ProcessEmptyFiles();
public:
  MY_UNKNOWN_IMP
  
  CFolderOutStream2()
  {
    _crcStreamSpec = new COutStreamWithCRC;
    _crcStream = _crcStreamSpec;
  }

  HRESULT Init(const CDbEx *db, UInt32 startIndex,
      const CBoolVector *extractStatuses, ISequentialOutStream *outStream);
  void ReleaseOutStream();
  HRESULT CheckFinishedState() const { return (_currentIndex == _extractStatuses->Size()) ? S_OK: E_FAIL; }

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

HRESULT CFolderOutStream2::Init(const CDbEx *db, UInt32 startIndex,
    const CBoolVector *extractStatuses, ISequentialOutStream *outStream)
{
  _db = db;
  _startIndex = startIndex;
  _extractStatuses = extractStatuses;
  _outStream = outStream;

  _currentIndex = 0;
  _fileIsOpen = false;
  return ProcessEmptyFiles();
}

void CFolderOutStream2::ReleaseOutStream()
{
  _outStream.Release();
  _crcStreamSpec->ReleaseStream();
}

void CFolderOutStream2::OpenFile()
{
  _crcStreamSpec->SetStream((*_extractStatuses)[_currentIndex] ? _outStream : NULL);
  _crcStreamSpec->Init(true);
  _fileIsOpen = true;
  _rem = _db->Files[_startIndex + _currentIndex].Size;
}

void CFolderOutStream2::CloseFile()
{
  _crcStreamSpec->ReleaseStream();
  _fileIsOpen = false;
  _currentIndex++;
}

HRESULT CFolderOutStream2::CloseFileAndSetResult()
{
  const CFileItem &file = _db->Files[_startIndex + _currentIndex];
  CloseFile();
  return (file.IsDir || !file.CrcDefined || file.Crc == _crcStreamSpec->GetCRC()) ? S_OK: S_FALSE;
}

HRESULT CFolderOutStream2::ProcessEmptyFiles()
{
  while (_currentIndex < _extractStatuses->Size() && _db->Files[_startIndex + _currentIndex].Size == 0)
  {
    OpenFile();
    RINOK(CloseFileAndSetResult());
  }
  return S_OK;
}

STDMETHODIMP CFolderOutStream2::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize != NULL)
    *processedSize = 0;
  while (size != 0)
  {
    if (_fileIsOpen)
    {
      UInt32 cur = size < _rem ? size : (UInt32)_rem;
      RINOK(_crcStream->Write(data, cur, &cur));
      if (cur == 0)
        break;
      data = (const Byte *)data + cur;
      size -= cur;
      _rem -= cur;
      if (processedSize != NULL)
        *processedSize += cur;
      if (_rem == 0)
      {
        RINOK(CloseFileAndSetResult());
        RINOK(ProcessEmptyFiles());
        continue;
      }
    }
    else
    {
      RINOK(ProcessEmptyFiles());
      if (_currentIndex == _extractStatuses->Size())
      {
        // we don't support partial extracting
        return E_FAIL;
      }
      OpenFile();
    }
  }
  return S_OK;
}

class CThreadDecoder: public CVirtThread
{
public:
  HRESULT Result;
  CMyComPtr<IInStream> InStream;

  CFolderOutStream2 *FosSpec;
  CMyComPtr<ISequentialOutStream> Fos;

  UInt64 StartPos;
  const CFolders *Folders;
  int FolderIndex;
  #ifndef _NO_CRYPTO
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  #endif

  DECL_EXTERNAL_CODECS_LOC_VARS2;
  CDecoder Decoder;

  #ifndef _7ZIP_ST
  bool MtMode;
  UInt32 NumThreads;
  #endif

  CThreadDecoder():
    Decoder(true)
  {
    #ifndef _7ZIP_ST
    MtMode = false;
    NumThreads = 1;
    #endif
    FosSpec = new CFolderOutStream2;
    Fos = FosSpec;
    Result = E_FAIL;
  }
  ~CThreadDecoder() { CVirtThread::WaitThreadFinish(); }
  virtual void Execute();
};

void CThreadDecoder::Execute()
{
  try
  {
    #ifndef _NO_CRYPTO
      bool isEncrypted = false;
      bool passwordIsDefined = false;
    #endif
    
    Result = Decoder.Decode(
      EXTERNAL_CODECS_LOC_VARS
      InStream,
      StartPos,
      *Folders, FolderIndex,
      Fos,
      NULL
      _7Z_DECODER_CRYPRO_VARS
      #ifndef _7ZIP_ST
        , MtMode, NumThreads
      #endif
      );
  }
  catch(...)
  {
    Result = E_FAIL;
  }
  if (Result == S_OK)
    Result = FosSpec->CheckFinishedState();
  FosSpec->ReleaseOutStream();
}

bool static Is86FilteredFolder(const CFolder &f)
{
  FOR_VECTOR(i, f.Coders)
  {
    CMethodId m = f.Coders[i].MethodID;
    if (m == k_BCJ || m == k_BCJ2)
      return true;
  }
  return false;
}

#ifndef _NO_CRYPTO

class CCryptoGetTextPassword:
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  UString Password;

  MY_UNKNOWN_IMP
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
};

STDMETHODIMP CCryptoGetTextPassword::CryptoGetTextPassword(BSTR *password)
{
  return StringToBstr(Password, password);
}

#endif

static const int kNumGroupsMax = 4;

static bool Is86Group(int group) { return (group & 1) != 0; }
static bool IsEncryptedGroup(int group) { return (group & 2) != 0; }
static int GetGroupIndex(bool encrypted, int bcjFiltered)
  { return (encrypted ? 2 : 0) + (bcjFiltered ? 1 : 0); }

static void GetFile(const CDatabase &inDb, int index, CFileItem &file, CFileItem2 &file2)
{
  file = inDb.Files[index];
  file2.CTimeDefined = inDb.CTime.GetItem(index, file2.CTime);
  file2.ATimeDefined = inDb.ATime.GetItem(index, file2.ATime);
  file2.MTimeDefined = inDb.MTime.GetItem(index, file2.MTime);
  file2.StartPosDefined = inDb.StartPos.GetItem(index, file2.StartPos);
  file2.IsAnti = inDb.IsItemAnti(index);
  // file2.IsAux = inDb.IsItemAux(index);
}

HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    const CDbEx *db,
    const CObjectVector<CUpdateItem> &updateItems,
    // const CObjectVector<CTreeFolder> &treeFolders,
    // const CUniqBlocks &secureBlocks,
    COutArchive &archive,
    CArchiveDatabaseOut &newDatabase,
    ISequentialOutStream *seqOutStream,
    IArchiveUpdateCallback *updateCallback,
    const CUpdateOptions &options
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getDecoderPassword
    #endif
    )
{
  UInt64 numSolidFiles = options.NumSolidFiles;
  if (numSolidFiles == 0)
    numSolidFiles = 1;

  // size_t totalSecureDataSize = (size_t)secureBlocks.GetTotalSizeInBytes();

  /*
  CMyComPtr<IOutStream> outStream;
  RINOK(seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStream));
  if (!outStream)
    return E_NOTIMPL;
  */

  UInt64 startBlockSize = db != 0 ? db->ArcInfo.StartPosition: 0;
  if (startBlockSize > 0 && !options.RemoveSfxBlock)
  {
    RINOK(WriteRange(inStream, seqOutStream, 0, startBlockSize, NULL));
  }

  CIntArr fileIndexToUpdateIndexMap;
  CRecordVector<CFolderRepack> folderRefs;
  UInt64 complexity = 0;
  UInt64 inSizeForReduce2 = 0;
  bool needEncryptedRepack = false;
  if (db != 0)
  {
    fileIndexToUpdateIndexMap.Alloc(db->Files.Size());
    unsigned i;
    for (i = 0; i < db->Files.Size(); i++)
      fileIndexToUpdateIndexMap[i] = -1;

    for (i = 0; i < updateItems.Size(); i++)
    {
      int index = updateItems[i].IndexInArchive;
      if (index != -1)
        fileIndexToUpdateIndexMap[index] = i;
    }

    for (i = 0; i < (int)db->NumFolders; i++)
    {
      CNum indexInFolder = 0;
      CNum numCopyItems = 0;
      CNum numUnpackStreams = db->NumUnpackStreamsVector[i];
      UInt64 repackSize = 0;
      for (CNum fi = db->FolderStartFileIndex[i]; indexInFolder < numUnpackStreams; fi++)
      {
        const CFileItem &file = db->Files[fi];
        if (file.HasStream)
        {
          indexInFolder++;
          int updateIndex = fileIndexToUpdateIndexMap[fi];
          if (updateIndex >= 0 && !updateItems[updateIndex].NewData)
          {
            numCopyItems++;
            repackSize += file.Size;
          }
        }
      }

      if (numCopyItems == 0)
        continue;

      CFolderRepack rep;
      rep.FolderIndex = i;
      rep.NumCopyFiles = numCopyItems;
      CFolder f;
      db->ParseFolderInfo(i, f);
      bool isEncrypted = f.IsEncrypted();
      rep.Group = GetGroupIndex(isEncrypted, Is86FilteredFolder(f));
      folderRefs.Add(rep);
      if (numCopyItems == numUnpackStreams)
        complexity += db->GetFolderFullPackSize(i);
      else
      {
        complexity += repackSize;
        if (repackSize > inSizeForReduce2)
          inSizeForReduce2 = repackSize;
        if (isEncrypted)
          needEncryptedRepack = true;
      }
    }
    folderRefs.Sort(CompareFolderRepacks, (void *)db);
  }

  UInt64 inSizeForReduce = 0;
  unsigned i;
  for (i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (ui.NewData)
    {
      complexity += ui.Size;
      if (numSolidFiles != 1)
        inSizeForReduce += ui.Size;
      else if (ui.Size > inSizeForReduce)
        inSizeForReduce = ui.Size;
    }
  }

  if (inSizeForReduce2 > inSizeForReduce)
    inSizeForReduce = inSizeForReduce2;

  RINOK(updateCallback->SetTotal(complexity));

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);

  CStreamBinder sb;
  RINOK(sb.CreateEvents());

  CThreadDecoder threadDecoder;
  if (!folderRefs.IsEmpty())
  {
    #ifdef EXTERNAL_CODECS
    threadDecoder.__externalCodecs = __externalCodecs;
    #endif
    RINOK(threadDecoder.Create());
  }

  CObjectVector<CSolidGroup> groups;
  for (i = 0; i < kNumGroupsMax; i++)
    groups.AddNew();

  {
    // ---------- Split files to groups ----------

    bool useFilters = options.UseFilters;
    const CCompressionMethodMode &method = *options.Method;
    if (method.Methods.Size() != 1 || method.Binds.Size() != 0)
      useFilters = false;
    for (i = 0; i < updateItems.Size(); i++)
    {
      const CUpdateItem &ui = updateItems[i];
      if (!ui.NewData || !ui.HasStream())
        continue;
      bool filteredGroup = false;
      if (useFilters)
      {
        int dotPos = ui.Name.ReverseFind(L'.');
        if (dotPos >= 0)
          filteredGroup = IsExeExt(ui.Name.Ptr(dotPos + 1));
      }
      groups[GetGroupIndex(method.PasswordIsDefined, filteredGroup)].Indices.Add(i);
    }
  }

  #ifndef _NO_CRYPTO

  CCryptoGetTextPassword *getPasswordSpec = NULL;
  if (needEncryptedRepack)
  {
    getPasswordSpec = new CCryptoGetTextPassword;
    threadDecoder.getTextPassword = getPasswordSpec;

    if (options.Method->PasswordIsDefined)
      getPasswordSpec->Password = options.Method->Password;
    else
    {
      if (!getDecoderPassword)
        return E_NOTIMPL;
      CMyComBSTR password;
      RINOK(getDecoderPassword->CryptoGetTextPassword(&password));
      if ((BSTR)password)
        getPasswordSpec->Password = password;
    }
  }

  #endif

  
  // ---------- Compress ----------

  RINOK(archive.Create(seqOutStream, false));
  RINOK(archive.SkipPrefixArchiveHeader());

  /*
  CIntVector treeFolderToArcIndex;
  treeFolderToArcIndex.Reserve(treeFolders.Size());
  for (i = 0; i < treeFolders.Size(); i++)
    treeFolderToArcIndex.Add(-1);
  // ---------- Write Tree (only AUX dirs) ----------
  for (i = 1; i < treeFolders.Size(); i++)
  {
    const CTreeFolder &treeFolder = treeFolders[i];
    CFileItem file;
    CFileItem2 file2;
    file2.Init();
    int secureID = 0;
    if (treeFolder.UpdateItemIndex < 0)
    {
      // we can store virtual dir item wuthout attrib, but we want all items have attrib.
      file.SetAttrib(FILE_ATTRIBUTE_DIRECTORY);
      file2.IsAux = true;
    }
    else
    {
      const CUpdateItem &ui = updateItems[treeFolder.UpdateItemIndex];
      // if item is not dir, then it's parent for alt streams.
      // we will write such items later
      if (!ui.IsDir)
        continue;
      secureID = ui.SecureIndex;
      if (ui.NewProps)
        FromUpdateItemToFileItem(ui, file, file2);
      else
        GetFile(*db, ui.IndexInArchive, file, file2);
    }
    file.Size = 0;
    file.HasStream = false;
    file.IsDir = true;
    file.Parent = treeFolder.Parent;
    
    treeFolderToArcIndex[i] = newDatabase.Files.Size();
    newDatabase.AddFile(file, file2, treeFolder.Name);
    
    if (totalSecureDataSize != 0)
      newDatabase.SecureIDs.Add(secureID);
  }
  */

  {
    /* ---------- Write non-AUX dirs and Empty files ---------- */
    CRecordVector<int> emptyRefs;
    for (i = 0; i < updateItems.Size(); i++)
    {
      const CUpdateItem &ui = updateItems[i];
      if (ui.NewData)
      {
        if (ui.HasStream())
          continue;
      }
      else if (ui.IndexInArchive != -1 && db->Files[ui.IndexInArchive].HasStream)
        continue;
      /*
      if (ui.TreeFolderIndex >= 0)
        continue;
      */
      emptyRefs.Add(i);
    }
    emptyRefs.Sort(CompareEmptyItems, (void *)&updateItems);
    for (i = 0; i < emptyRefs.Size(); i++)
    {
      const CUpdateItem &ui = updateItems[emptyRefs[i]];
      CFileItem file;
      CFileItem2 file2;
      UString name;
      if (ui.NewProps)
      {
        FromUpdateItemToFileItem(ui, file, file2);
        name = ui.Name;
      }
      else
      {
        GetFile(*db, ui.IndexInArchive, file, file2);
        db->GetPath(ui.IndexInArchive, name);
      }
      
      /*
      if (totalSecureDataSize != 0)
        newDatabase.SecureIDs.Add(ui.SecureIndex);
      file.Parent = ui.ParentFolderIndex;
      */
      newDatabase.AddFile(file, file2, name);
    }
  }

  unsigned folderRefIndex = 0;
  lps->ProgressOffset = 0;

  for (int groupIndex = 0; groupIndex < kNumGroupsMax; groupIndex++)
  {
    const CSolidGroup &group = groups[groupIndex];

    CCompressionMethodMode method = *options.Method;
    MakeExeMethod(method, options.UseFilters, Is86Group(groupIndex), options.MaxFilter);

    if (IsEncryptedGroup(groupIndex))
    {
      if (!method.PasswordIsDefined)
      {
        #ifndef _NO_CRYPTO
        if (getPasswordSpec)
          method.Password = getPasswordSpec->Password;
        #endif
        method.PasswordIsDefined = true;
      }
    }
    else
    {
      method.PasswordIsDefined = false;
      method.Password.Empty();
    }

    CEncoder encoder(method);

    for (; folderRefIndex < folderRefs.Size(); folderRefIndex++)
    {
      const CFolderRepack &rep = folderRefs[folderRefIndex];
      if (rep.Group != groupIndex)
        break;
      int folderIndex = rep.FolderIndex;
      
      if (rep.NumCopyFiles == db->NumUnpackStreamsVector[folderIndex])
      {
        UInt64 packSize = db->GetFolderFullPackSize(folderIndex);
        RINOK(WriteRange(inStream, archive.SeqStream,
          db->GetFolderStreamPos(folderIndex, 0), packSize, progress));
        lps->ProgressOffset += packSize;
        
        CFolder &folder = newDatabase.Folders.AddNew();
        db->ParseFolderInfo(folderIndex, folder);
        CNum startIndex = db->FoStartPackStreamIndex[folderIndex];
        for (unsigned j = 0; j < folder.PackStreams.Size(); j++)
        {
          newDatabase.PackSizes.Add(db->GetStreamPackSize(startIndex + j));
          // newDatabase.PackCRCsDefined.Add(db.PackCRCsDefined[startIndex + j]);
          // newDatabase.PackCRCs.Add(db.PackCRCs[startIndex + j]);
        }

        UInt32 indexStart = db->FoToCoderUnpackSizes[folderIndex];
        UInt32 indexEnd = db->FoToCoderUnpackSizes[folderIndex + 1];
        for (; indexStart < indexEnd; indexStart++)
          newDatabase.CoderUnpackSizes.Add(db->CoderUnpackSizes[indexStart]);
      }
      else
      {
        CBoolVector extractStatuses;
        
        CNum numUnpackStreams = db->NumUnpackStreamsVector[folderIndex];
        CNum indexInFolder = 0;
        
        for (CNum fi = db->FolderStartFileIndex[folderIndex]; indexInFolder < numUnpackStreams; fi++)
        {
          bool needExtract = false;
          if (db->Files[fi].HasStream)
          {
            indexInFolder++;
            int updateIndex = fileIndexToUpdateIndexMap[fi];
            if (updateIndex >= 0 && !updateItems[updateIndex].NewData)
              needExtract = true;
          }
          extractStatuses.Add(needExtract);
        }

        unsigned startPackIndex = newDatabase.PackSizes.Size();
        UInt64 curUnpackSize;
        {
          CMyComPtr<ISequentialInStream> sbInStream;
          {
            CMyComPtr<ISequentialOutStream> sbOutStream;
            sb.CreateStreams(&sbInStream, &sbOutStream);
            sb.ReInit();
            RINOK(threadDecoder.FosSpec->Init(db, db->FolderStartFileIndex[folderIndex], &extractStatuses, sbOutStream));
          }
          
          threadDecoder.InStream = inStream;
          threadDecoder.Folders = (const CFolders *)db;
          threadDecoder.FolderIndex = folderIndex;
          threadDecoder.StartPos = db->ArcInfo.DataStartPosition; // db->GetFolderStreamPos(folderIndex, 0);
          
          threadDecoder.Start();
          
          RINOK(encoder.Encode(
              EXTERNAL_CODECS_LOC_VARS
              sbInStream, NULL, &inSizeForReduce,
              newDatabase.Folders.AddNew(), newDatabase.CoderUnpackSizes, curUnpackSize,
              archive.SeqStream, newDatabase.PackSizes, progress));
          
          threadDecoder.WaitExecuteFinish();
        }

        RINOK(threadDecoder.Result);

        for (; startPackIndex < newDatabase.PackSizes.Size(); startPackIndex++)
          lps->OutSize += newDatabase.PackSizes[startPackIndex];
        lps->InSize += curUnpackSize;
      }
      
      newDatabase.NumUnpackStreamsVector.Add(rep.NumCopyFiles);
      
      CNum numUnpackStreams = db->NumUnpackStreamsVector[folderIndex];
      
      CNum indexInFolder = 0;
      for (CNum fi = db->FolderStartFileIndex[folderIndex]; indexInFolder < numUnpackStreams; fi++)
      {
        CFileItem file;
        CFileItem2 file2;
        GetFile(*db, fi, file, file2);
        UString name;
        db->GetPath(fi, name);
        if (file.HasStream)
        {
          indexInFolder++;
          int updateIndex = fileIndexToUpdateIndexMap[fi];
          if (updateIndex >= 0)
          {
            const CUpdateItem &ui = updateItems[updateIndex];
            if (ui.NewData)
              continue;
            if (ui.NewProps)
            {
              CFileItem uf;
              FromUpdateItemToFileItem(ui, uf, file2);
              uf.Size = file.Size;
              uf.Crc = file.Crc;
              uf.CrcDefined = file.CrcDefined;
              uf.HasStream = file.HasStream;
              file = uf;
              name = ui.Name;
            }
            /*
            file.Parent = ui.ParentFolderIndex;
            if (ui.TreeFolderIndex >= 0)
              treeFolderToArcIndex[ui.TreeFolderIndex] = newDatabase.Files.Size();
            if (totalSecureDataSize != 0)
              newDatabase.SecureIDs.Add(ui.SecureIndex);
            */
            newDatabase.AddFile(file, file2, name);
          }
        }
      }
    }

    unsigned numFiles = group.Indices.Size();
    if (numFiles == 0)
      continue;
    CRecordVector<CRefItem> refItems;
    refItems.ClearAndSetSize(numFiles);
    bool sortByType = (numSolidFiles > 1);
    for (i = 0; i < numFiles; i++)
      refItems[i] = CRefItem(group.Indices[i], updateItems[group.Indices[i]], sortByType);
    CSortParam sortParam;
    // sortParam.TreeFolders = &treeFolders;
    sortParam.SortByType = sortByType;
    refItems.Sort(CompareUpdateItems, (void *)&sortParam);
    
    CObjArray<UInt32> indices(numFiles);

    for (i = 0; i < numFiles; i++)
    {
      UInt32 index = refItems[i].Index;
      indices[i] = index;
      /*
      const CUpdateItem &ui = updateItems[index];
      CFileItem file;
      if (ui.NewProps)
        FromUpdateItemToFileItem(ui, file);
      else
        file = db.Files[ui.IndexInArchive];
      if (file.IsAnti || file.IsDir)
        return E_FAIL;
      newDatabase.Files.Add(file);
      */
    }
    
    for (i = 0; i < numFiles;)
    {
      UInt64 totalSize = 0;
      int numSubFiles;
      UString prevExtension;
      for (numSubFiles = 0; i + numSubFiles < numFiles &&
          numSubFiles < numSolidFiles; numSubFiles++)
      {
        const CUpdateItem &ui = updateItems[indices[i + numSubFiles]];
        totalSize += ui.Size;
        if (totalSize > options.NumSolidBytes)
          break;
        if (options.SolidExtension)
        {
          UString ext = ui.GetExtension();
          if (numSubFiles == 0)
            prevExtension = ext;
          else
            if (!ext.IsEqualToNoCase(prevExtension))
              break;
        }
      }
      if (numSubFiles < 1)
        numSubFiles = 1;

      CFolderInStream *inStreamSpec = new CFolderInStream;
      CMyComPtr<ISequentialInStream> solidInStream(inStreamSpec);
      inStreamSpec->Init(updateCallback, &indices[i], numSubFiles);
      
      unsigned startPackIndex = newDatabase.PackSizes.Size();
      UInt64 curFolderUnpackSize;
      RINOK(encoder.Encode(
          EXTERNAL_CODECS_LOC_VARS
          solidInStream, NULL, &inSizeForReduce,
          newDatabase.Folders.AddNew(), newDatabase.CoderUnpackSizes, curFolderUnpackSize,
          archive.SeqStream, newDatabase.PackSizes, progress));

      for (; startPackIndex < newDatabase.PackSizes.Size(); startPackIndex++)
        lps->OutSize += newDatabase.PackSizes[startPackIndex];

      lps->InSize += curFolderUnpackSize;
      // for ()
      // newDatabase.PackCRCsDefined.Add(false);
      // newDatabase.PackCRCs.Add(0);
      
      CNum numUnpackStreams = 0;
      for (int subIndex = 0; subIndex < numSubFiles; subIndex++)
      {
        const CUpdateItem &ui = updateItems[indices[i + subIndex]];
        CFileItem file;
        CFileItem2 file2;
        UString name;
        if (ui.NewProps)
        {
          FromUpdateItemToFileItem(ui, file, file2);
          name = ui.Name;
        }
        else
        {
          GetFile(*db, ui.IndexInArchive, file, file2);
          db->GetPath(ui.IndexInArchive, name);
        }
        if (file2.IsAnti || file.IsDir)
          return E_FAIL;
        
        /*
        CFileItem &file = newDatabase.Files[
              startFileIndexInDatabase + i + subIndex];
        */
        if (!inStreamSpec->Processed[subIndex])
        {
          continue;
          // file.Name += L".locked";
        }

        file.Crc = inStreamSpec->CRCs[subIndex];
        file.Size = inStreamSpec->Sizes[subIndex];
        if (file.Size != 0)
        {
          file.CrcDefined = true;
          file.HasStream = true;
          numUnpackStreams++;
        }
        else
        {
          file.CrcDefined = false;
          file.HasStream = false;
        }
        /*
        file.Parent = ui.ParentFolderIndex;
        if (ui.TreeFolderIndex >= 0)
          treeFolderToArcIndex[ui.TreeFolderIndex] = newDatabase.Files.Size();
        if (totalSecureDataSize != 0)
          newDatabase.SecureIDs.Add(ui.SecureIndex);
        */
        newDatabase.AddFile(file, file2, name);
      }
      // numUnpackStreams = 0 is very bad case for locked files
      // v3.13 doesn't understand it.
      newDatabase.NumUnpackStreamsVector.Add(numUnpackStreams);
      i += numSubFiles;
    }
  }

  if (folderRefIndex != folderRefs.Size())
    return E_FAIL;

  RINOK(lps->SetCur());

  /*
  folderRefs.ClearAndFree();
  fileIndexToUpdateIndexMap.ClearAndFree();
  groups.ClearAndFree();
  */

  /*
  for (i = 0; i < newDatabase.Files.Size(); i++)
  {
    CFileItem &file = newDatabase.Files[i];
    file.Parent = treeFolderToArcIndex[file.Parent];
  }

  if (totalSecureDataSize != 0)
  {
    newDatabase.SecureBuf.SetCapacity(totalSecureDataSize);
    size_t pos = 0;
    newDatabase.SecureSizes.Reserve(secureBlocks.Sorted.Size());
    for (i = 0; i < secureBlocks.Sorted.Size(); i++)
    {
      const CByteBuffer &buf = secureBlocks.Bufs[secureBlocks.Sorted[i]];
      size_t size = buf.GetCapacity();
      memcpy(newDatabase.SecureBuf + pos, buf, size);
      newDatabase.SecureSizes.Add((UInt32)size);
      pos += size;
    }
  }
  */
  newDatabase.ReserveDown();
  return S_OK;
}

}}
