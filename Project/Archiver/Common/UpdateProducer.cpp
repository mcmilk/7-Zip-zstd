// UpdateProducer.cpp

#include "StdAfx.h"

#include "UpdateProducer.h"

using namespace NUpdateArchive;

static const char *kUpdateActionSetCollision =
    "Internal collision in update action set";

void UpdateProduce(
      const CArchiveStyleDirItemInfoVector &aDirItems, 
      const CArchiveItemInfoVector &anArchiveItems, 
      const CUpdatePairInfoVector &anUpdatePairs,
      const CActionSet &anActionSet,
      CUpdatePairInfo2Vector &anOperationChain)
{
  for(int i = 0; i < anUpdatePairs.Size(); i++)
  {
    // CUpdateArchiveRange aRange;
    const CUpdatePairInfo &aPairInfo = anUpdatePairs[i];

    CUpdatePairInfo2 aPairInfo2;
    aPairInfo2.IsAnti = false;
    aPairInfo2.ArchiveItemIndex = aPairInfo.ArchiveItemIndex;
    aPairInfo2.DirItemIndex = aPairInfo.DirItemIndex;
    aPairInfo2.ExistInArchive = (aPairInfo.State != NPairState::kOnlyOnDisk);
    aPairInfo2.ExistOnDisk = (aPairInfo.State != NPairState::kOnlyInArchive);
    switch(anActionSet.StateActions[aPairInfo.State])
    {
      case NPairAction::kIgnore:
        /*
        if (aPairInfo.State != NPairState::kOnlyOnDisk)
          IgnoreArchiveItem(m_ArchiveItems[aPairInfo.ArchiveItemIndex]);
        // cout << "deleting";
        */
        break;
      case NPairAction::kCopy:
        {
          if (aPairInfo.State == NPairState::kOnlyOnDisk)
            throw kUpdateActionSetCollision;
          aPairInfo2.OperationIsCompress = false;
          anOperationChain.Add(aPairInfo2);
          break;
        }
      case NPairAction::kCompress:
        {
          if (aPairInfo.State == NPairState::kOnlyInArchive || 
            aPairInfo.State == NPairState::kNotMasked)
            throw kUpdateActionSetCollision;
          aPairInfo2.OperationIsCompress = true;
          anOperationChain.Add(aPairInfo2);
          break;
        }
      case NPairAction::kCompressAsAnti:
        {
          aPairInfo2.IsAnti = true;
          aPairInfo2.OperationIsCompress = true;
          anOperationChain.Add(aPairInfo2);
          break;
        }
    }
  }
}
