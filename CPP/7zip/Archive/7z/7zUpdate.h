// 7zUpdate.h

#ifndef __7Z_UPDATE_H
#define __7Z_UPDATE_H

#include "../IArchive.h"

// #include "../../Common/UniqBlocks.h"

#include "7zCompressionMode.h"
#include "7zIn.h"
#include "7zOut.h"

namespace NArchive {
namespace N7z {

/*
struct CTreeFolder
{
  UString Name;
  int Parent;
  CIntVector SubFolders;
  int UpdateItemIndex;
  int SortIndex;
  int SortIndexEnd;

  CTreeFolder(): UpdateItemIndex(-1) {}
};
*/

struct CUpdateItem
{
  int IndexInArchive;
  int IndexInClient;
  
  UInt64 CTime;
  UInt64 ATime;
  UInt64 MTime;

  UInt64 Size;
  UString Name;
  /*
  bool IsAltStream;
  int ParentFolderIndex;
  int TreeFolderIndex;
  */

  // that code is not used in 9.26
  // int ParentSortIndex;
  // int ParentSortIndexEnd;

  UInt32 Attrib;
  
  bool NewData;
  bool NewProps;

  bool IsAnti;
  bool IsDir;

  bool AttribDefined;
  bool CTimeDefined;
  bool ATimeDefined;
  bool MTimeDefined;

  // int SecureIndex; // 0 means (no_security)

  bool HasStream() const { return !IsDir && !IsAnti && Size != 0; }

  CUpdateItem():
      // ParentSortIndex(-1),
      // IsAltStream(false),
      IsAnti(false),
      IsDir(false),
      AttribDefined(false),
      CTimeDefined(false),
      ATimeDefined(false),
      MTimeDefined(false)
      // SecureIndex(0)
      {}
  void SetDirStatusFromAttrib() { IsDir = ((Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0); };

  int GetExtensionPos() const;
  UString GetExtension() const;
};

struct CUpdateOptions
{
  const CCompressionMethodMode *Method;
  const CCompressionMethodMode *HeaderMethod;
  bool UseFilters;
  bool MaxFilter;

  CHeaderOptions HeaderOptions;

  UInt64 NumSolidFiles;
  UInt64 NumSolidBytes;
  bool SolidExtension;
  bool RemoveSfxBlock;
  bool VolumeMode;
};

HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    const CDbEx *db,
    const CObjectVector<CUpdateItem> &updateItems,
    // const CObjectVector<CTreeFolder> &treeFolders, // treeFolders[0] is root
    // const CUniqBlocks &secureBlocks,
    COutArchive &archive,
    CArchiveDatabaseOut &newDatabase,
    ISequentialOutStream *seqOutStream,
    IArchiveUpdateCallback *updateCallback,
    const CUpdateOptions &options
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getDecoderPassword
    #endif
    );
}}

#endif
