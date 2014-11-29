// OpenArchive.h

#ifndef __OPEN_ARCHIVE_H
#define __OPEN_ARCHIVE_H

#include "../../../Windows/PropVariant.h"

#include "ArchiveOpenCallback.h"
#include "LoadCodecs.h"
#include "Property.h"

HRESULT Archive_GetItemBoolProp(IInArchive *arc, UInt32 index, PROPID propID, bool &result) throw();
HRESULT Archive_IsItem_Folder(IInArchive *arc, UInt32 index, bool &result) throw();
HRESULT Archive_IsItem_Aux(IInArchive *arc, UInt32 index, bool &result) throw();
HRESULT Archive_IsItem_AltStream(IInArchive *arc, UInt32 index, bool &result) throw();
HRESULT Archive_IsItem_Deleted(IInArchive *arc, UInt32 index, bool &deleted) throw();

/*
struct COptionalOpenProperties
{
  UString FormatName;
  CObjectVector<CProperty> Props;
};
*/

#ifdef _SFX
#define OPEN_PROPS_DECL
#else
#define OPEN_PROPS_DECL const CObjectVector<CProperty> *props;
// #define OPEN_PROPS_DECL , const CObjectVector<COptionalOpenProperties> *props
#endif

struct COpenSpecFlags
{
  // bool CanReturnFull;
  bool CanReturnFrontal;
  bool CanReturnTail;
  bool CanReturnMid;

  bool CanReturn_NonStart() const { return CanReturnTail || CanReturnMid; }

  COpenSpecFlags():
    // CanReturnFull(true),
    CanReturnFrontal(false),
    CanReturnTail(false),
    CanReturnMid(false)
    {}
};

struct COpenType
{
  int FormatIndex;

  COpenSpecFlags SpecForcedType;
  COpenSpecFlags SpecMainType;
  COpenSpecFlags SpecWrongExt;
  COpenSpecFlags SpecUnknownExt;

  bool Recursive;

  bool CanReturnArc;
  bool CanReturnParser;
  bool EachPos;

  // bool SkipSfxStub;
  // bool ExeAsUnknown;

  bool ZerosTailIsAllowed;

  bool MaxStartOffset_Defined;
  UInt64 MaxStartOffset;

  const COpenSpecFlags &GetSpec(bool isForced, bool isMain, bool isUnknown) const
  {
    return isForced ? SpecForcedType : (isMain ? SpecMainType : (isUnknown ? SpecUnknownExt : SpecWrongExt));
  }

  COpenType():
      FormatIndex(-1),
      Recursive(true),
      EachPos(false),
      CanReturnArc(true),
      CanReturnParser(false),
      // SkipSfxStub(true),
      // ExeAsUnknown(true),
      ZerosTailIsAllowed(false),
      MaxStartOffset_Defined(false),
      MaxStartOffset(0)
  {
    SpecForcedType.CanReturnFrontal = true;
    SpecForcedType.CanReturnTail = true;
    SpecForcedType.CanReturnMid = true;

    SpecMainType.CanReturnFrontal = true;

    SpecUnknownExt.CanReturnTail = true; // for sfx
    SpecUnknownExt.CanReturnMid = true;
    SpecUnknownExt.CanReturnFrontal = true; // for alt streams of sfx with pad

    // ZerosTailIsAllowed = true;
  }
};

struct COpenOptions
{
  CCodecs *codecs;
  COpenType openType;
  const CObjectVector<COpenType> *types;
  const CIntVector *excludedFormats;

  IInStream *stream;
  ISequentialInStream *seqStream;
  IArchiveOpenCallback *callback;
  COpenCallbackImp *callbackSpec;
  OPEN_PROPS_DECL
  // bool openOnlySpecifiedByExtension,

  bool stdInMode;
  UString filePath;

  COpenOptions():
      codecs(NULL),
      types(NULL),
      excludedFormats(NULL),
      stream(NULL),
      seqStream(NULL),
      callback(NULL),
      callbackSpec(NULL),
      stdInMode(false)
    {}

};

UInt32 GetOpenArcErrorFlags(const NWindows::NCOM::CPropVariant &prop, bool *isDefinedProp = NULL);

struct CArcErrorInfo
{
  bool ThereIsTail;
  bool UnexpecedEnd;
  bool IgnoreTail; // all are zeros
  // bool NonZerosTail;
  bool ErrorFlags_Defined;
  UInt32 ErrorFlags;
  UInt32 WarningFlags;
  int ErrorFormatIndex; // - 1 means no Error.
                        // if FormatIndex == ErrorFormatIndex, the archive is open with offset
  UInt64 TailSize;

  /* if CArc is Open OK with some format:
        - ErrorFormatIndex shows error format index, if extension is incorrect
        - other variables show message and warnings of archive that is open */
  
  UString ErrorMessage;
  UString WarningMessage;

  // call IsArc_After_NonOpen only if Open returns S_FALSE
  bool IsArc_After_NonOpen() const
  {
    return (ErrorFlags_Defined && (ErrorFlags & kpv_ErrorFlags_IsNotArc) == 0);
  }


  CArcErrorInfo():
      ThereIsTail(false),
      UnexpecedEnd(false),
      IgnoreTail(false),
      // NonZerosTail(false),
      ErrorFlags_Defined(false),
      ErrorFlags(0),
      WarningFlags(0),
      ErrorFormatIndex(-1),
      TailSize(0)
    {}

  void ClearErrors();

  void ClearErrors_Full()
  {
    ErrorFormatIndex = -1;
    ClearErrors();
  }

  bool IsThereErrorOrWarning() const
  {
    return ErrorFlags != 0
        || WarningFlags != 0
        || NeedTailWarning()
        || UnexpecedEnd
        || !ErrorMessage.IsEmpty()
        || !WarningMessage.IsEmpty();
  }

  bool AreThereErrors() const { return ErrorFlags != 0 || UnexpecedEnd; }
  bool AreThereWarnings() const { return WarningFlags != 0 || NeedTailWarning(); }

  bool NeedTailWarning() const { return !IgnoreTail && ThereIsTail; }

  UInt32 GetWarningFlags() const
  {
    UInt32 a = WarningFlags;
    if (NeedTailWarning() && (ErrorFlags & kpv_ErrorFlags_DataAfterEnd) == 0)
      a |= kpv_ErrorFlags_DataAfterEnd;
    return a;
  }

  UInt32 GetErrorFlags() const
  {
    UInt32 a = ErrorFlags;
    if (UnexpecedEnd)
      a |= kpv_ErrorFlags_UnexpectedEnd;
    return a;
  }
};

class CArc
{
  HRESULT PrepareToOpen(const COpenOptions &op, unsigned formatIndex, CMyComPtr<IInArchive> &archive);
  HRESULT CheckZerosTail(const COpenOptions &op, UInt64 offset);
  HRESULT OpenStream2(const COpenOptions &options);

public:
  CMyComPtr<IInArchive> Archive;
  CMyComPtr<IInStream> InStream;
          // we use InStream in 2 cases (ArcStreamOffset != 0):
          // 1) if we use additional cache stream
          // 2) we reopen sfx archive with CTailInStream
  
  CMyComPtr<IArchiveGetRawProps> GetRawProps;
  CMyComPtr<IArchiveGetRootProps> GetRootProps;

  CArcErrorInfo ErrorInfo; // for OK archives
  CArcErrorInfo NonOpen_ErrorInfo; // ErrorInfo for mainArchive (false OPEN)

  UString Path;
  UString filePath;
  UString DefaultName;
  int FormatIndex; // - 1 means Parser.
  int SubfileIndex;
  FILETIME MTime;
  bool MTimeDefined;
  
  Int64 Offset; // it's offset of start of archive inside stream that is open by Archive Handler
  UInt64 PhySize;
  // UInt64 OkPhySize;
  bool PhySizeDefined;
  // bool OkPhySize_Defined;
  UInt64 FileSize;
  UInt64 AvailPhySize; // PhySize, but it's reduced if exceed end of file
  // bool offsetDefined;

  UInt64 ArcStreamOffset; // offset of stream that is open by Archive Handler
  Int64 GetGlobalOffset() const { return ArcStreamOffset + Offset; } // it's global offset of archive

  // AString ErrorFlagsText;

  bool IsParseArc;

  bool IsTree;
  
  bool Ask_Deleted;
  bool Ask_AltStream;
  bool Ask_Aux;
  bool Ask_INode;

  bool IgnoreSplit; // don't try split handler

  // void Set_ErrorFlagsText();

  CArc():
    MTimeDefined(false),
    IsTree(false),
    Ask_Deleted(false),
    Ask_AltStream(false),
    Ask_Aux(false),
    Ask_INode(false),
    IgnoreSplit(false)
    {}

  HRESULT ReadBasicProps(IInArchive *archive, UInt64 startPos, HRESULT openRes);

  // ~CArc();

  HRESULT Close()
  {
    InStream.Release();
    return Archive->Close();
  }

  // AltStream's name is concatenated with base file name in one string in parts.Back()
  HRESULT GetItemPathToParent(UInt32 index, UInt32 parent, UStringVector &parts) const;

  HRESULT GetItemPath(UInt32 index, UString &result) const;
  
  // GetItemPath2 adds [DELETED] dir prefix for deleted items.
  HRESULT GetItemPath2(UInt32 index, UString &result) const;
  
  HRESULT GetItemSize(UInt32 index, UInt64 &size, bool &defined) const;
  HRESULT GetItemMTime(UInt32 index, FILETIME &ft, bool &defined) const;
  HRESULT IsItemAnti(UInt32 index, bool &result) const
    { return Archive_GetItemBoolProp(Archive, index, kpidIsAnti, result); }


  HRESULT OpenStream(const COpenOptions &options);
  HRESULT OpenStreamOrFile(COpenOptions &options);

  HRESULT ReOpen(const COpenOptions &options);
  
  HRESULT CreateNewTailStream(CMyComPtr<IInStream> &stream);
};

struct CArchiveLink
{
  CObjectVector<CArc> Arcs;
  UStringVector VolumePaths;
  UInt64 VolumesSize;
  bool IsOpen;

  // int NonOpenErrorFormatIndex; // - 1 means no Error.
  UString NonOpen_ArcPath;

  CArcErrorInfo NonOpen_ErrorInfo;

  // UString ErrorsText;
  // void Set_ErrorsText();

  CArchiveLink(): VolumesSize(0), IsOpen(false) {}
  void KeepModeForNextOpen();
  HRESULT Close();
  void Release();
  ~CArchiveLink() { Release(); }

  const CArc *GetArc() const { return &Arcs.Back(); }
  IInArchive *GetArchive() const { return Arcs.Back().Archive; }
  IArchiveGetRawProps *GetArchiveGetRawProps() const { return Arcs.Back().GetRawProps; }
  IArchiveGetRootProps *GetArchiveGetRootProps() const { return Arcs.Back().GetRootProps; }

  HRESULT Open(COpenOptions &options);

  HRESULT Open2(COpenOptions &options, IOpenCallbackUI *callbackUI);

  HRESULT ReOpen(COpenOptions &options);
};

bool ParseOpenTypes(CCodecs &codecs, const UString &s, CObjectVector<COpenType> &types);

#endif
