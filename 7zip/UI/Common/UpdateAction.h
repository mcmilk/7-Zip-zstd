// UpdateAction.h

#pragma once

#ifndef __UPDATE_ACTION_H
#define __UPDATE_ACTION_H

namespace NUpdateArchive {

  namespace NPairState 
  {
    const kNumValues = 7;
    enum EEnum
    {
      kNotMasked = 0,
      kOnlyInArchive,
      kOnlyOnDisk,
      kNewInArchive,
      kOldInArchive,
      kSameFiles,
      kUnknowNewerFiles
    };
  }
  namespace NPairAction
  {
    enum EEnum
    {
      kIgnore = 0,
      kCopy,
      kCompress,
      kCompressAsAnti
    };
  }
  struct CActionSet
  {
    NPairAction::EEnum StateActions[NPairState::kNumValues];
  };
  extern const CActionSet kAddActionSet;
  extern const CActionSet kUpdateActionSet;
  extern const CActionSet kFreshActionSet;
  extern const CActionSet kSynchronizeActionSet;
  extern const CActionSet kDeleteActionSet;
};


#endif


