// x86.h

#ifndef __X86_H
#define __X86_H

#include "BranchCoder.h"
extern "C" 
{ 
#include "../../../../C/Compress/Branch/BranchX86.h"
}

struct CBranch86
{
  UInt32 _prevMask;
  void x86Init() { x86_Convert_Init(_prevMask); }
};

MyClassB(BCJ_x86, 0x01, 3, CBranch86 , 
    virtual void SubInit() { x86Init(); })

#endif
