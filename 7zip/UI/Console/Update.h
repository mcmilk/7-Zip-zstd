// Update.h

#pragma once

#ifndef __UPDATE_H
#define __UPDATE_H

#include "Common/Wildcard.h"
#include "../Common/UpdateAction.h"
// #include "ProxyHandler.h"
#include "Windows/FileFind.h"
#include "../../Archive/IArchive.h"
#include "CompressionMode.h"

struct CUpdateArchiveCommand
{
  CSysString ArchivePath;
  NUpdateArchive::CActionSet ActionSet;
};

struct CUpdateArchiveOptions
{
  CObjectVector<CUpdateArchiveCommand> Commands;
  bool UpdateArchiveItself;

  bool SfxMode;
  CSysString SfxModule;

  CSysString ArchivePath;
  CCompressionMethodMode MethodMode;
};


HRESULT UpdateArchiveStdMain(const NWildcard::CCensor &censor, 
    CUpdateArchiveOptions &options, const CSysString &workingDir,
    IInArchive *archive,
    const UString *defaultItemName,
    const NWindows::NFile::NFind::CFileInfo *archiveFileInfo,
    bool enablePercents);

#endif
