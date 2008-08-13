// UpdateMain.cpp

#include "StdAfx.h"

#include "7zUpdate.h"
#include "7zFolderInStream.h"
#include "7zEncode.h"
#include "7zHandler.h"
#include "7zOut.h"

#include "../../Compress/Copy/CopyCoder.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/LimitedStreams.h"
#include "../Common/ItemNameUtils.h"

namespace NArchive {
namespace N7z {

static const wchar_t *kMatchFinderForBCJ2_LZMA = L"BT2";
static const UInt32 kDictionaryForBCJ2_LZMA = 1 << 20;
static const UInt32 kAlgorithmForBCJ2_LZMA = 1;
static const UInt32 kNumFastBytesForBCJ2_LZMA = 64;

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
    return Name.Length();
  return dotPos + 1;
}

UString CUpdateItem::GetExtension() const
{
  return Name.Mid(GetExtensionPos());
}

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareBuffers(const CByteBuffer &a1, const CByteBuffer &a2)
{
  size_t c1 = a1.GetCapacity();
  size_t c2 = a2.GetCapacity();
  RINOZ(MyCompare(c1, c2));
  for (size_t i = 0; i < c1; i++)
    RINOZ(MyCompare(a1[i], a2[i]));
  return 0;
}

static int CompareCoders(const CCoderInfo &c1, const CCoderInfo &c2)
{
  RINOZ(MyCompare(c1.NumInStreams, c2.NumInStreams));
  RINOZ(MyCompare(c1.NumOutStreams, c2.NumOutStreams));
  RINOZ(MyCompare(c1.MethodID, c2.MethodID));
  return CompareBuffers(c1.Properties, c2.Properties);
}

static int CompareBindPairs(const CBindPair &b1, const CBindPair &b2)
{
  RINOZ(MyCompare(b1.InIndex, b2.InIndex));
  return MyCompare(b1.OutIndex, b2.OutIndex);
}

static int CompareFolders(const CFolder &f1, const CFolder &f2)
{
  int s1 = f1.Coders.Size();
  int s2 = f2.Coders.Size();
  RINOZ(MyCompare(s1, s2));
  int i;
  for (i = 0; i < s1; i++)
    RINOZ(CompareCoders(f1.Coders[i], f2.Coders[i]));
  s1 = f1.BindPairs.Size();
  s2 = f2.BindPairs.Size();
  RINOZ(MyCompare(s1, s2));
  for (i = 0; i < s1; i++)
    RINOZ(CompareBindPairs(f1.BindPairs[i], f2.BindPairs[i]));
  return 0;
}

static int CompareFiles(const CFileItem &f1, const CFileItem &f2)
{
  return MyStringCompareNoCase(f1.Name, f2.Name);
}

static int CompareFolderRefs(const int *p1, const int *p2, void *param)
{
  int i1 = *p1;
  int i2 = *p2;
  const CArchiveDatabaseEx &db = *(const CArchiveDatabaseEx *)param;
  RINOZ(CompareFolders(
      db.Folders[i1],
      db.Folders[i2]));
  RINOZ(MyCompare(
      db.NumUnpackStreamsVector[i1],
      db.NumUnpackStreamsVector[i2]));
  if (db.NumUnpackStreamsVector[i1] == 0)
    return 0;
  return CompareFiles(
      db.Files[db.FolderStartFileIndex[i1]],
      db.Files[db.FolderStartFileIndex[i2]]);
}

////////////////////////////////////////////////////////////

static int CompareEmptyItems(const int *p1, const int *p2, void *param)
{
  const CObjectVector<CUpdateItem> &updateItems = *(const CObjectVector<CUpdateItem> *)param;
  const CUpdateItem &u1 = updateItems[*p1];
  const CUpdateItem &u2 = updateItems[*p2];
  if (u1.IsDir != u2.IsDir)
    return (u1.IsDir) ? 1 : -1;
  if (u1.IsDir)
  {
    if (u1.IsAnti != u2.IsAnti)
      return (u1.IsAnti ? 1 : -1);
    int n = MyStringCompareNoCase(u1.Name, u2.Name);
    return -n;
  }
  if (u1.IsAnti != u2.IsAnti)
    return (u1.IsAnti ? 1 : -1);
  return MyStringCompareNoCase(u1.Name, u2.Name);
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

int GetExtIndex(const char *ext)
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
  int ExtensionIndex;
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
      NamePos = ((slashPos >= 0) ? (slashPos + 1) : 0);
      int dotPos = ui.Name.ReverseFind(L'.');
      if (dotPos < 0 || (dotPos < slashPos && slashPos >= 0))
        ExtensionPos = ui.Name.Length();
      else
      {
        ExtensionPos = dotPos + 1;
        UString us = ui.Name.Mid(ExtensionPos);
        if (!us.IsEmpty())
        {
          us.MakeLower();
          int i;
          AString s;
          for (i = 0; i < us.Length(); i++)
          {
            wchar_t c = us[i];
            if (c >= 0x80)
              break;
            s += (char)c;
          }
          if (i == us.Length())
            ExtensionIndex = GetExtIndex(s);
          else
            ExtensionIndex = 0;
        }
      }
    }
  }
};

static int CompareUpdateItems(const CRefItem *p1, const CRefItem *p2, void *param)
{
  const CRefItem &a1 = *p1;
  const CRefItem &a2 = *p2;
  const CUpdateItem &u1 = *a1.UpdateItem;
  const CUpdateItem &u2 = *a2.UpdateItem;
  int n;
  if (u1.IsDir != u2.IsDir)
    return (u1.IsDir) ? 1 : -1;
  if (u1.IsDir)
  {
    if (u1.IsAnti != u2.IsAnti)
      return (u1.IsAnti ? 1 : -1);
    n = MyStringCompareNoCase(u1.Name, u2.Name);
    return -n;
  }
  bool sortByType = *(bool *)param;
  if (sortByType)
  {
    RINOZ(MyCompare(a1.ExtensionIndex, a2.ExtensionIndex))
    RINOZ(MyStringCompareNoCase(u1.Name + a1.ExtensionPos, u2.Name + a2.ExtensionPos));
    RINOZ(MyStringCompareNoCase(u1.Name + a1.NamePos, u2.Name + a2.NamePos));
    if (!u1.MTimeDefined && u2.MTimeDefined) return 1;
    if (u1.MTimeDefined && !u2.MTimeDefined) return -1;
    if (u1.MTimeDefined && u2.MTimeDefined) RINOZ(MyCompare(u1.MTime, u2.MTime));
    RINOZ(MyCompare(u1.Size, u2.Size))
  }
  return MyStringCompareNoCase(u1.Name, u2.Name);
}

struct CSolidGroup
{
  CCompressionMethodMode Method;
  CRecordVector<UInt32> Indices;
};

static wchar_t *g_ExeExts[] =
{
  L"dll",
  L"exe",
  L"ocx",
  L"sfx",
  L"sys"
};

static bool IsExeFile(const UString &ext)
{
  for (int i = 0; i < sizeof(g_ExeExts) / sizeof(g_ExeExts[0]); i++)
    if (ext.CompareNoCase(g_ExeExts[i]) == 0)
      return true;
  return false;
}

static const UInt64 k_LZMA  = 0x030101;
static const UInt64 k_BCJ   = 0x03030103;
static const UInt64 k_BCJ2  = 0x0303011B;

static bool GetMethodFull(UInt64 methodID,
    UInt32 numInStreams, CMethodFull &methodResult)
{
  methodResult.Id = methodID;
  methodResult.NumInStreams = numInStreams;
  methodResult.NumOutStreams = 1;
  return true;
}

static bool MakeExeMethod(const CCompressionMethodMode &method,
    bool bcj2Filter, CCompressionMethodMode &exeMethod)
{
  exeMethod = method;
  if (bcj2Filter)
  {
    CMethodFull methodFull;
    if (!GetMethodFull(k_BCJ2, 4, methodFull))
      return false;
    exeMethod.Methods.Insert(0, methodFull);
    if (!GetMethodFull(k_LZMA, 1, methodFull))
      return false;
    {
      CProp property;
      property.Id = NCoderPropID::kAlgorithm;
      property.Value = kAlgorithmForBCJ2_LZMA;
      methodFull.Properties.Add(property);
    }
    {
      CProp property;
      property.Id = NCoderPropID::kMatchFinder;
      property.Value = kMatchFinderForBCJ2_LZMA;
      methodFull.Properties.Add(property);
    }
    {
      CProp property;
      property.Id = NCoderPropID::kDictionarySize;
      property.Value = kDictionaryForBCJ2_LZMA;
      methodFull.Properties.Add(property);
    }
    {
      CProp property;
      property.Id = NCoderPropID::kNumFastBytes;
      property.Value = kNumFastBytesForBCJ2_LZMA;
      methodFull.Properties.Add(property);
    }

    exeMethod.Methods.Add(methodFull);
    exeMethod.Methods.Add(methodFull);
    CBind bind;

    bind.OutCoder = 0;
    bind.InStream = 0;

    bind.InCoder = 1;
    bind.OutStream = 0;
    exeMethod.Binds.Add(bind);

    bind.InCoder = 2;
    bind.OutStream = 1;
    exeMethod.Binds.Add(bind);

    bind.InCoder = 3;
    bind.OutStream = 2;
    exeMethod.Binds.Add(bind);
  }
  else
  {
    CMethodFull methodFull;
    if (!GetMethodFull(k_BCJ, 1, methodFull))
      return false;
    exeMethod.Methods.Insert(0, methodFull);
    CBind bind;
    bind.OutCoder = 0;
    bind.InStream = 0;
    bind.InCoder = 1;
    bind.OutStream = 0;
    exeMethod.Binds.Add(bind);
  }
  return true;
}

static void SplitFilesToGroups(
    const CCompressionMethodMode &method,
    bool useFilters, bool maxFilter,
    const CObjectVector<CUpdateItem> &updateItems,
    CObjectVector<CSolidGroup> &groups)
{
  if (method.Methods.Size() != 1 || method.Binds.Size() != 0)
    useFilters = false;
  groups.Clear();
  groups.Add(CSolidGroup());
  groups.Add(CSolidGroup());
  CSolidGroup &generalGroup = groups[0];
  CSolidGroup &exeGroup = groups[1];
  generalGroup.Method = method;
  int i;
  for (i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (!ui.NewData)
      continue;
    if (!ui.HasStream())
      continue;
    if (useFilters)
    {
      const UString name = ui.Name;
      int dotPos = name.ReverseFind(L'.');
      if (dotPos >= 0)
      {
        UString ext = name.Mid(dotPos + 1);
        if (IsExeFile(ext))
        {
          exeGroup.Indices.Add(i);
          continue;
        }
      }
    }
    generalGroup.Indices.Add(i);
  }
  if (exeGroup.Indices.Size() > 0)
    if (!MakeExeMethod(method, maxFilter, exeGroup.Method))
      exeGroup.Method = method;
  for (i = 0; i < groups.Size();)
    if (groups[i].Indices.Size() == 0)
      groups.Delete(i);
    else
      i++;
}

static void FromUpdateItemToFileItem(const CUpdateItem &ui,
    CFileItem &file, CFileItem2 &file2)
{
  file.Name = NItemName::MakeLegalName(ui.Name);
  if (ui.AttribDefined)
    file.SetAttrib(ui.Attrib);
  
  file2.CTime = ui.CTime;  file2.CTimeDefined = ui.CTimeDefined;
  file2.ATime = ui.ATime;  file2.ATimeDefined = ui.ATimeDefined;
  file2.MTime = ui.MTime;  file2.MTimeDefined = ui.MTimeDefined;
  file2.IsAnti = ui.IsAnti;
  file2.StartPosDefined = false;

  file.Size = ui.Size;
  file.IsDir = ui.IsDir;
  file.HasStream = ui.HasStream();
}

static HRESULT Update2(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    const CArchiveDatabaseEx *db,
    const CObjectVector<CUpdateItem> &updateItems,
    COutArchive &archive,
    CArchiveDatabase &newDatabase,
    ISequentialOutStream *seqOutStream,
    IArchiveUpdateCallback *updateCallback,
    const CUpdateOptions &options)
{
  UInt64 numSolidFiles = options.NumSolidFiles;
  if (numSolidFiles == 0)
    numSolidFiles = 1;
  /*
  CMyComPtr<IOutStream> outStream;
  RINOK(seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStream));
  if (!outStream)
    return E_NOTIMPL;
  */

  UInt64 startBlockSize = db != 0 ? db->ArchiveInfo.StartPosition: 0;
  if (startBlockSize > 0 && !options.RemoveSfxBlock)
  {
    RINOK(WriteRange(inStream, seqOutStream, 0, startBlockSize, NULL));
  }

  CRecordVector<int> fileIndexToUpdateIndexMap;
  if (db != 0)
  {
    fileIndexToUpdateIndexMap.Reserve(db->Files.Size());
    for (int i = 0; i < db->Files.Size(); i++)
      fileIndexToUpdateIndexMap.Add(-1);
  }
  int i;
  for(i = 0; i < updateItems.Size(); i++)
  {
    int index = updateItems[i].IndexInArchive;
    if (index != -1)
      fileIndexToUpdateIndexMap[index] = i;
  }

  CRecordVector<int> folderRefs;
  if (db != 0)
  {
    for(i = 0; i < db->Folders.Size(); i++)
    {
      CNum indexInFolder = 0;
      CNum numCopyItems = 0;
      CNum numUnpackStreams = db->NumUnpackStreamsVector[i];
      for (CNum fileIndex = db->FolderStartFileIndex[i];
      indexInFolder < numUnpackStreams; fileIndex++)
      {
        if (db->Files[fileIndex].HasStream)
        {
          indexInFolder++;
          int updateIndex = fileIndexToUpdateIndexMap[fileIndex];
          if (updateIndex >= 0)
            if (!updateItems[updateIndex].NewData)
              numCopyItems++;
        }
      }
      if (numCopyItems != numUnpackStreams && numCopyItems != 0)
        return E_NOTIMPL; // It needs repacking !!!
      if (numCopyItems > 0)
        folderRefs.Add(i);
    }
    folderRefs.Sort(CompareFolderRefs, (void *)db);
  }

  ////////////////////////////

  RINOK(archive.Create(seqOutStream, false));
  RINOK(archive.SkeepPrefixArchiveHeader());
  UInt64 complexity = 0;
  for(i = 0; i < folderRefs.Size(); i++)
    complexity += db->GetFolderFullPackSize(folderRefs[i]);
  UInt64 inSizeForReduce = 0;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (ui.NewData)
    {
      complexity += ui.Size;
      if (numSolidFiles == 1)
      {
        if (ui.Size > inSizeForReduce)
          inSizeForReduce = ui.Size;
      }
      else
        inSizeForReduce += ui.Size;
    }
  }
  RINOK(updateCallback->SetTotal(complexity));
  complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));


  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);

  /////////////////////////////////////////
  // Write Copy Items

  for(i = 0; i < folderRefs.Size(); i++)
  {
    int folderIndex = folderRefs[i];
    
    lps->ProgressOffset = complexity;
    UInt64 packSize = db->GetFolderFullPackSize(folderIndex);
    RINOK(WriteRange(inStream, archive.SeqStream,
        db->GetFolderStreamPos(folderIndex, 0), packSize, progress));
    complexity += packSize;
    
    const CFolder &folder = db->Folders[folderIndex];
    CNum startIndex = db->FolderStartPackStreamIndex[folderIndex];
    for (int j = 0; j < folder.PackStreams.Size(); j++)
    {
      newDatabase.PackSizes.Add(db->PackSizes[startIndex + j]);
      // newDatabase.PackCRCsDefined.Add(db.PackCRCsDefined[startIndex + j]);
      // newDatabase.PackCRCs.Add(db.PackCRCs[startIndex + j]);
    }
    newDatabase.Folders.Add(folder);

    CNum numUnpackStreams = db->NumUnpackStreamsVector[folderIndex];
    newDatabase.NumUnpackStreamsVector.Add(numUnpackStreams);

    CNum indexInFolder = 0;
    for (CNum fi = db->FolderStartFileIndex[folderIndex];
        indexInFolder < numUnpackStreams; fi++)
    {
      CFileItem file;
      CFileItem2 file2;
      db->GetFile(fi, file, file2);
      if (file.HasStream)
      {
        indexInFolder++;
        int updateIndex = fileIndexToUpdateIndexMap[fi];
        if (updateIndex >= 0)
        {
          const CUpdateItem &ui = updateItems[updateIndex];
          if (ui.NewProperties)
          {
            CFileItem uf;
            FromUpdateItemToFileItem(ui, uf, file2);
            uf.Size = file.Size;
            uf.Crc = file.Crc;
            uf.CrcDefined = file.CrcDefined;
            uf.HasStream = file.HasStream;
            file = uf;
          }
        }
        newDatabase.AddFile(file, file2);
      }
    }
  }

  folderRefs.ClearAndFree();
  fileIndexToUpdateIndexMap.ClearAndFree();

  /////////////////////////////////////////
  // Compress New Files

  CObjectVector<CSolidGroup> groups;
  SplitFilesToGroups(*options.Method, options.UseFilters, options.MaxFilter,
      updateItems, groups);

  const UInt32 kMinReduceSize = (1 << 16);
  if (inSizeForReduce < kMinReduceSize)
    inSizeForReduce = kMinReduceSize;

  for (int groupIndex = 0; groupIndex < groups.Size(); groupIndex++)
  {
    const CSolidGroup &group = groups[groupIndex];
    int numFiles = group.Indices.Size();
    if (numFiles == 0)
      continue;
    CRecordVector<CRefItem> refItems;
    refItems.Reserve(numFiles);
    bool sortByType = (numSolidFiles > 1);
    for (i = 0; i < numFiles; i++)
      refItems.Add(CRefItem(group.Indices[i], updateItems[group.Indices[i]], sortByType));
    refItems.Sort(CompareUpdateItems, (void *)&sortByType);
    
    CRecordVector<UInt32> indices;
    indices.Reserve(numFiles);

    for (i = 0; i < numFiles; i++)
    {
      UInt32 index = refItems[i].Index;
      indices.Add(index);
      /*
      const CUpdateItem &ui = updateItems[index];
      CFileItem file;
      if (ui.NewProperties)
        FromUpdateItemToFileItem(ui, file);
      else
        file = db.Files[ui.IndexInArchive];
      if (file.IsAnti || file.IsDir)
        return E_FAIL;
      newDatabase.Files.Add(file);
      */
    }
    
    CEncoder encoder(group.Method);

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
            if (ext.CompareNoCase(prevExtension) != 0)
              break;
        }
      }
      if (numSubFiles < 1)
        numSubFiles = 1;

      CFolderInStream *inStreamSpec = new CFolderInStream;
      CMyComPtr<ISequentialInStream> solidInStream(inStreamSpec);
      inStreamSpec->Init(updateCallback, &indices[i], numSubFiles);
      
      CFolder folderItem;

      int startPackIndex = newDatabase.PackSizes.Size();
      RINOK(encoder.Encode(
          EXTERNAL_CODECS_LOC_VARS
          solidInStream, NULL, &inSizeForReduce, folderItem,
          archive.SeqStream, newDatabase.PackSizes, progress));

      for (; startPackIndex < newDatabase.PackSizes.Size(); startPackIndex++)
        lps->OutSize += newDatabase.PackSizes[startPackIndex];

      lps->InSize += folderItem.GetUnpackSize();
      // for()
      // newDatabase.PackCRCsDefined.Add(false);
      // newDatabase.PackCRCs.Add(0);
      
      newDatabase.Folders.Add(folderItem);
      
      CNum numUnpackStreams = 0;
      for (int subIndex = 0; subIndex < numSubFiles; subIndex++)
      {
        const CUpdateItem &ui = updateItems[indices[i + subIndex]];
        CFileItem file;
        CFileItem2 file2;
        if (ui.NewProperties)
          FromUpdateItemToFileItem(ui, file, file2);
        else
          db->GetFile(ui.IndexInArchive, file, file2);
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
        newDatabase.AddFile(file, file2);
      }
      // numUnpackStreams = 0 is very bad case for locked files
      // v3.13 doesn't understand it.
      newDatabase.NumUnpackStreamsVector.Add(numUnpackStreams);
      i += numSubFiles;
    }
  }

  groups.ClearAndFree();

  {
    /////////////////////////////////////////
    // Write Empty Files & Folders
    
    CRecordVector<int> emptyRefs;
    for(i = 0; i < updateItems.Size(); i++)
    {
      const CUpdateItem &ui = updateItems[i];
      if (ui.NewData)
      {
        if (ui.HasStream())
          continue;
      }
      else
        if (ui.IndexInArchive != -1)
          if (db->Files[ui.IndexInArchive].HasStream)
            continue;
      emptyRefs.Add(i);
    }
    emptyRefs.Sort(CompareEmptyItems, (void *)&updateItems);
    for (i = 0; i < emptyRefs.Size(); i++)
    {
      const CUpdateItem &ui = updateItems[emptyRefs[i]];
      CFileItem file;
      CFileItem2 file2;
      if (ui.NewProperties)
        FromUpdateItemToFileItem(ui, file, file2);
      else
        db->GetFile(ui.IndexInArchive, file, file2);
      newDatabase.AddFile(file, file2);
    }
  }
    
  newDatabase.ReserveDown();
  return S_OK;
}

HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    const CArchiveDatabaseEx *db,
    const CObjectVector<CUpdateItem> &updateItems,
    COutArchive &archive,
    CArchiveDatabase &newDatabase,
    ISequentialOutStream *seqOutStream,
    IArchiveUpdateCallback *updateCallback,
    const CUpdateOptions &options)
{
  return Update2(
        EXTERNAL_CODECS_LOC_VARS
        inStream, db, updateItems,
        archive, newDatabase, seqOutStream, updateCallback, options);
}

}}
