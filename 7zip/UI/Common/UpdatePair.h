// UpdatePair.h

#ifndef __UPDATE_PAIR_H
#define __UPDATE_PAIR_H

#include "DirItem.h"
#include "UpdateAction.h"

#include "../../Archive/IArchive.h"

struct CUpdatePair
{
  NUpdateArchive::NPairState::EEnum State;
  int ArchiveItemIndex;
  int DirItemIndex;
};

void GetUpdatePairInfoList(
    const CObjectVector<CDirItem> &dirItems,
    const CObjectVector<CArchiveItem> &archiveItems,
    NFileTimeType::EEnum fileTimeType,
    CObjectVector<CUpdatePair> &updatePairs);

#endif
