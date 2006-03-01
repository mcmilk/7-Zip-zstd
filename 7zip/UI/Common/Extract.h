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

HRESULT DecompressArchives(
    UStringVector &archivePaths, UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback);

#endif
