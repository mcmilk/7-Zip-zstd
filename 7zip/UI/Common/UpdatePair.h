// UpdatePair.h

#pragma once

#ifndef __UPDATEPAIR_H
#define __UPDATEPAIR_H

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