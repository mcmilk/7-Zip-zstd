// LoadCodecs.h

#ifndef __LOAD_CODECS_H
#define __LOAD_CODECS_H

#include "../../../Common/MyBuffer.h"
#include "../../../Common/MyCom.h"
#include "../../../Common/MyString.h"
#include "../../../Common/ComTry.h"

#include "../../ICoder.h"

#ifdef EXTERNAL_CODECS
#include "../../../Windows/DLL.h"
#endif

struct CDllCodecInfo
{
  CLSID Encoder;
  CLSID Decoder;
  bool EncoderIsAssigned;
  bool DecoderIsAssigned;
  int LibIndex;
  UInt32 CodecIndex;
};

struct CDllHasherInfo
{
  int LibIndex;
  UInt32 HasherIndex;
};

#include "../../Archive/IArchive.h"

struct CArcExtInfo
{
  UString Ext;
  UString AddExt;
  
  CArcExtInfo() {}
  CArcExtInfo(const UString &ext): Ext(ext) {}
  CArcExtInfo(const UString &ext, const UString &addExt): Ext(ext), AddExt(addExt) {}
};


struct CArcInfoEx
{
  UInt32 Flags;
  
  Func_CreateInArchive CreateInArchive;
  Func_IsArc IsArcFunc;

  UString Name;
  CObjectVector<CArcExtInfo> Exts;
  
  #ifndef _SFX
    Func_CreateOutArchive CreateOutArchive;
    bool UpdateEnabled;
    bool NewInterface;
    // UInt32 Version;
    UInt32 SignatureOffset;
    CObjectVector<CByteBuffer> Signatures;
    #ifdef NEW_FOLDER_INTERFACE
      UStringVector AssociateExts;
    #endif
  #endif
  
  #ifdef EXTERNAL_CODECS
    int LibIndex;
    UInt32 FormatIndex;
    CLSID ClassID;
  #endif

  bool Flags_KeepName() const { return (Flags & NArcInfoFlags::kKeepName) != 0; }
  bool Flags_FindSignature() const { return (Flags & NArcInfoFlags::kFindSignature) != 0; }

  bool Flags_AltStreams() const { return (Flags & NArcInfoFlags::kAltStreams) != 0; }
  bool Flags_NtSecure() const { return (Flags & NArcInfoFlags::kNtSecure) != 0; }
  bool Flags_SymLinks() const { return (Flags & NArcInfoFlags::kSymLinks) != 0; }
  bool Flags_HardLinks() const { return (Flags & NArcInfoFlags::kHardLinks) != 0; }

  bool Flags_UseGlobalOffset() const { return (Flags & NArcInfoFlags::kUseGlobalOffset) != 0; }
  bool Flags_StartOpen() const { return (Flags & NArcInfoFlags::kStartOpen) != 0; }
  bool Flags_BackwardOpen() const { return (Flags & NArcInfoFlags::kBackwardOpen) != 0; }
  bool Flags_PreArc() const { return (Flags & NArcInfoFlags::kPreArc) != 0; }
  bool Flags_PureStartOpen() const { return (Flags & NArcInfoFlags::kPureStartOpen) != 0; }
  
  UString GetMainExt() const
  {
    if (Exts.IsEmpty())
      return UString();
    return Exts[0].Ext;
  }
  int FindExtension(const UString &ext) const;
  
  /*
  UString GetAllExtensions() const
  {
    UString s;
    for (int i = 0; i < Exts.Size(); i++)
    {
      if (i > 0)
        s += ' ';
      s += Exts[i].Ext;
    }
    return s;
  }
  */

  void AddExts(const UString &ext, const UString &addExt);

  bool IsSplit() const { return StringsAreEqualNoCase_Ascii(Name, "Split"); }
  // bool IsRar() const { return StringsAreEqualNoCase_Ascii(Name, "Rar"); }

  CArcInfoEx():
      Flags(0),
      CreateInArchive(NULL),
      IsArcFunc(NULL)
      #ifndef _SFX
      , CreateOutArchive(NULL)
      , UpdateEnabled(false)
      , NewInterface(false)
      // , Version(0)
      , SignatureOffset(0)
      #endif
      #ifdef EXTERNAL_CODECS
      , LibIndex(-1)
      #endif
  {}
};

#ifdef EXTERNAL_CODECS

#ifdef NEW_FOLDER_INTERFACE
struct CCodecIcons
{
  struct CIconPair
  {
    UString Ext;
    int IconIndex;
  };
  CObjectVector<CIconPair> IconPairs;
  void LoadIcons(HMODULE m);
  bool FindIconIndex(const UString &ext, int &iconIndex) const;
};
#endif

struct CCodecLib
  #ifdef NEW_FOLDER_INTERFACE
    : public CCodecIcons
  #endif
{
  NWindows::NDLL::CLibrary Lib;
  FString Path;
  Func_GetMethodProperty GetMethodProperty;
  Func_CreateObject CreateObject;
  CMyComPtr<IHashers> Hashers;
  
  #ifdef NEW_FOLDER_INTERFACE
  void LoadIcons() { CCodecIcons::LoadIcons((HMODULE)Lib); }
  #endif
  
  CCodecLib(): GetMethodProperty(NULL) {}
};
#endif

class CCodecs:
  #ifdef EXTERNAL_CODECS
  public ICompressCodecsInfo,
  public IHashers,
  #else
  public IUnknown,
  #endif
  public CMyUnknownImp
{
public:
  #ifdef EXTERNAL_CODECS
  CObjectVector<CCodecLib> Libs;
  CRecordVector<CDllCodecInfo> Codecs;
  CRecordVector<CDllHasherInfo> Hashers;

  #ifdef NEW_FOLDER_INTERFACE
  CCodecIcons InternalIcons;
  #endif

  HRESULT LoadCodecs();
  HRESULT LoadFormats();
  HRESULT LoadDll(const FString &path, bool needCheckDll);
  HRESULT LoadDllsFromFolder(const FString &folderPrefix);

  HRESULT CreateArchiveHandler(const CArcInfoEx &ai, void **archive, bool outHandler) const
  {
    return Libs[ai.LibIndex].CreateObject(&ai.ClassID, outHandler ? &IID_IOutArchive : &IID_IInArchive, (void **)archive);
  }
  #endif

public:
  CObjectVector<CArcInfoEx> Formats;
  bool CaseSensitiveChange;
  bool CaseSensitive;

  CCodecs(): CaseSensitiveChange(false), CaseSensitive(false) {}
 
  const wchar_t *GetFormatNamePtr(int formatIndex)
  {
    return formatIndex < 0 ? L"#" : (const wchar_t *)Formats[formatIndex].Name;
  }

  HRESULT Load();
  
  #ifndef _SFX
  int FindFormatForArchiveName(const UString &arcPath) const;
  int FindFormatForExtension(const UString &ext) const;
  int FindFormatForArchiveType(const UString &arcType) const;
  bool FindFormatForArchiveType(const UString &arcType, CIntVector &formatIndices) const;
  #endif

  #ifdef EXTERNAL_CODECS

  MY_UNKNOWN_IMP2(ICompressCodecsInfo, IHashers)
    
  STDMETHOD(GetNumberOfMethods)(UInt32 *numMethods);
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(CreateDecoder)(UInt32 index, const GUID *interfaceID, void **coder);
  STDMETHOD(CreateEncoder)(UInt32 index, const GUID *interfaceID, void **coder);

  STDMETHOD_(UInt32, GetNumHashers)();
  STDMETHOD(GetHasherProp)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(CreateHasher)(UInt32 index, IHasher **hasher);

  #else

  MY_UNKNOWN_IMP

  #endif // EXTERNAL_CODECS

  #ifdef EXTERNAL_CODECS

  int GetCodecLibIndex(UInt32 index);
  bool GetCodecEncoderIsAssigned(UInt32 index);
  HRESULT GetCodecId(UInt32 index, UInt64 &id);
  UString GetCodecName(UInt32 index);

  int GetHasherLibIndex(UInt32 index);
  UInt64 GetHasherId(UInt32 index);
  UString GetHasherName(UInt32 index);
  UInt32 GetHasherDigestSize(UInt32 index);

  #endif

  HRESULT CreateInArchive(unsigned formatIndex, CMyComPtr<IInArchive> &archive) const
  {
    const CArcInfoEx &ai = Formats[formatIndex];
    #ifdef EXTERNAL_CODECS
    if (ai.LibIndex < 0)
    #endif
    {
      COM_TRY_BEGIN
      archive = ai.CreateInArchive();
      return S_OK;
      COM_TRY_END
    }
    #ifdef EXTERNAL_CODECS
    return CreateArchiveHandler(ai, (void **)&archive, false);
    #endif
  }
  
  #ifndef _SFX

  HRESULT CreateOutArchive(unsigned formatIndex, CMyComPtr<IOutArchive> &archive) const
  {
    const CArcInfoEx &ai = Formats[formatIndex];
    #ifdef EXTERNAL_CODECS
    if (ai.LibIndex < 0)
    #endif
    {
      COM_TRY_BEGIN
      archive = ai.CreateOutArchive();
      return S_OK;
      COM_TRY_END
    }
    #ifdef EXTERNAL_CODECS
    return CreateArchiveHandler(ai, (void **)&archive, true);
    #endif
  }
  
  int FindOutFormatFromName(const UString &name) const
  {
    FOR_VECTOR (i, Formats)
    {
      const CArcInfoEx &arc = Formats[i];
      if (!arc.UpdateEnabled)
        continue;
      if (arc.Name.IsEqualToNoCase(name))
        return i;
    }
    return -1;
  }

  #endif // _SFX
};

#endif
