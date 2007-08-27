// Extract.h

#ifndef __EXTRACT_H
#define __EXTRACT_H

#include "Common/Wildcard.h"
#include "Windows/FileFind.h"

#include "../../Archive/IArchive.h"

#include "ArchiveExtractCallback.h"
#include "ArchiveOpenCallback.h"
#include "ExtractMode.h"
#include "Property.h"

#include "../Common/LoadCodecs.h"

class CExtractOptions
{
public:
  bool StdOutMode;
  bool TestMode;
  NExtract::NPathMode::EEnum PathMode;

  UString OutputDir;
  bool YesToAll;
  UString DefaultItemName;
  NWindows::NFile::NFind::CFileInfoW ArchiveFileInfo;
  
  // bool ShowDialog;
  // bool PasswordEnabled;
  // UString Password;
  #ifdef COMPRESS_MT
  CObjectVector<CProperty> Properties;
  #endif

  NExtract::NOverwriteMode::EEnum OverwriteMode;

  #ifdef EXTERNAL_CODECS
  CCodecs *Codecs;
  #endif

  CExtractOptions(): 
      StdOutMode(false), 
      YesToAll(false), 
      TestMode(false),
      PathMode(NExtract::NPathMode::kFullPathnames),
      OverwriteMode(NExtract::NOverwriteMode::kAskBefore)
      {}

  /*
    bool FullPathMode() const { return (ExtractMode == NExtractMode::kTest) || 
    (ExtractMode == NExtractMode::kFullPath); }
  */
};

struct CDecompressStat
{
  UInt64 NumArchives;
  UInt64 UnpackSize;
  UInt64 PackSize;
  UInt64 NumFolders;
  UInt64 NumFiles;
  void Clear() { NumArchives = PackSize = UnpackSize = NumFolders = NumFiles = 0; }
};

HRESULT DecompressArchives(
    CCodecs *codecs,
    UStringVector &archivePaths, UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback,
    UString &errorMessage, 
    CDecompressStat &stat);

#endif
