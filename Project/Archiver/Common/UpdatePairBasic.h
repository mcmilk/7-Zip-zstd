// UpdatePairBasic.h

#pragma once

#ifndef __UPDATEPAIRBASIC_H
#define __UPDATEPAIRBASIC_H

namespace NUpdateArchive{

  namespace NPairState 
  {
    const kNumValues = 6;
    enum EEnum
    {
      kNotMasked = 0,
      kOnlyInArchive,
      kOnlyOnDisk,
      kNewInArchive,
      kOldInArchive,
      kSameFiles,      
    };
  }
  namespace NPairAction
  {
    enum EEnum
    {
      kIgnore = 0,
      kCopy,
      kCompress
    };
  }
  struct CActionSet
  {
    NPairAction::EEnum StateActions[NPairState::kNumValues];
  };
};


#endif


