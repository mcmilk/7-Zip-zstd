// UpdateProduce.h

#ifndef __UPDATE_PRODUCE_H
#define __UPDATE_PRODUCE_H

#include "UpdatePair.h"

struct CUpdatePair2
{
  // bool OperationIsCompress;
  bool NewData;
  bool NewProperties;

  bool ExistInArchive;
  bool ExistOnDisk;
  bool IsAnti;
  int ArchiveItemIndex;
  int DirItemIndex;

  bool NewNameIsDefined;
  UString NewName;

  CUpdatePair2(): NewNameIsDefined(false) {}
};

void UpdateProduce(
    const CObjectVector<CUpdatePair> &updatePairs,
    const NUpdateArchive::CActionSet &actionSet,
    CObjectVector<CUpdatePair2> &operationChain);

#endif
