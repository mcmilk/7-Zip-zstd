// ListArchive.h

#pragma once

#ifndef __LISTARCHIVE_H
#define __LISTARCHIVE_H

#include "../Format/Common/ArchiveInterface.h"
#include "Common/Wildcard.h"
#include "Windows/FileFind.h"

/*
namespace NListMode
{
  enum EEnum
  {
    kDefault,
    kAdd,
    kAll
  };
}
*/

HRESULT ListArchive(IInArchive *archive, 
    const UString &defaultItemName,
    const NWindows::NFile::NFind::CFileInfo &srchiveFileInfo,
    const NWildcard::CCensor &wildcardCensor/*, bool fullPathMode, 
    NListMode::EEnum mode*/);

#endif

