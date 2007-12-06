// UpdateProduce.cpp

#include "StdAfx.h"

#include "UpdateProduce.h"

using namespace NUpdateArchive;

static const char *kUpdateActionSetCollision =
    "Internal collision in update action set";

void UpdateProduce(
    const CObjectVector<CUpdatePair> &updatePairs,
    const NUpdateArchive::CActionSet &actionSet,
    CObjectVector<CUpdatePair2> &operationChain)
{
  for(int i = 0; i < updatePairs.Size(); i++)
  {
    // CUpdateArchiveRange aRange;
    const CUpdatePair &pair = updatePairs[i];

    CUpdatePair2 pair2;
    pair2.IsAnti = false;
    pair2.ArchiveItemIndex = pair.ArchiveItemIndex;
    pair2.DirItemIndex = pair.DirItemIndex;
    pair2.ExistInArchive = (pair.State != NPairState::kOnlyOnDisk);
    pair2.ExistOnDisk = (pair.State != NPairState::kOnlyInArchive && pair.State != NPairState::kNotMasked);
    switch(actionSet.StateActions[pair.State])
    {
      case NPairAction::kIgnore:
        /*
        if (pair.State != NPairState::kOnlyOnDisk)
          IgnoreArchiveItem(m_ArchiveItems[pair.ArchiveItemIndex]);
        // cout << "deleting";
        */
        break;
      case NPairAction::kCopy:
        {
          if (pair.State == NPairState::kOnlyOnDisk)
            throw kUpdateActionSetCollision;
          pair2.NewData = pair2.NewProperties = false;
          operationChain.Add(pair2);
          break;
        }
      case NPairAction::kCompress:
        {
          if (pair.State == NPairState::kOnlyInArchive || 
            pair.State == NPairState::kNotMasked)
            throw kUpdateActionSetCollision;
          pair2.NewData = pair2.NewProperties = true;
          operationChain.Add(pair2);
          break;
        }
      case NPairAction::kCompressAsAnti:
        {
          pair2.IsAnti = true;
          pair2.NewData = pair2.NewProperties = true;
          operationChain.Add(pair2);
          break;
        }
    }
  }
}
