// Extract.h

#pragma once

#ifndef __EXTRACT_H
#define __EXTRACT_H

#include "Common/Wildcard.h"
#include "Windows/FileFind.h"

#include "../../Archive/IArchive.h"
#include "../Common/ZipRegistry.h"

namespace NExtractMode {

enum EEnum
{
  kTest,
  kFullPath,
  kExtractToOne
};


}

class CExtractOptions
{
public:
  NExtractMode::EEnum ExtractMode;
  CSysString OutputBaseDir;
  bool YesToAll;
  UString DefaultItemName;
  NWindows::NFile::NFind::CFileInfo ArchiveFileInfo;
  bool PasswordEnabled;
  UString Password;

  NExtraction::NOverwriteMode::EEnum OverwriteMode;

  
  CExtractOptions(NExtractMode::EEnum extractMode, const CSysString &outputBaseDir,
      bool yesToAll, bool passwordEnabled, const UString &password,
      NExtraction::NOverwriteMode::EEnum overwriteMode):
    ExtractMode(extractMode),
    OutputBaseDir(outputBaseDir),
    YesToAll(yesToAll),
    PasswordEnabled(passwordEnabled), 
    Password(password),
    OverwriteMode(overwriteMode)
    {}

  bool TestMode() const {  return (ExtractMode == NExtractMode::kTest); }
  bool FullPathMode() const { return (ExtractMode == NExtractMode::kTest) || 
    (ExtractMode == NExtractMode::kFullPath); }
};

HRESULT DeCompressArchiveSTD(IInArchive *archive,
    const NWildcard::CCensor &wildcardCensor,
    const CExtractOptions &options);

/*
bool DeCompressArchiveSTD(TTWildCardInputArchive &anArchive, 
    const TTExtractOptions &anOptions);
*/

#endif
