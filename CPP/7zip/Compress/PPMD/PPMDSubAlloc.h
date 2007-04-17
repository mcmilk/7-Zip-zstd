// PPMDSubAlloc.h
// This code is based on Dmitry Shkarin's PPMdH code

#ifndef __PPMD_SUBALLOC_H
#define __PPMD_SUBALLOC_H

#include "PPMDType.h"

extern "C" 
{ 
#include "../../../../C/Alloc.h"
}

const UINT N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4;
const UINT UNIT_SIZE=12, N_INDEXES=N1+N2+N3+N4;

// Extra 1 * UNIT_SIZE for NULL support
// Extra 2 * UNIT_SIZE for s0 in GlueFreeBlocks()
const UInt32 kExtraSize = (UNIT_SIZE * 3);
const UInt32 kMaxMemBlockSize = 0xFFFFFFFF - kExtraSize;

struct MEM_BLK 
{
  UInt16 Stamp, NU;
  UInt32 Next, Prev;
  void InsertAt(Byte *Base, UInt32 p) 
  {
    Prev = p;
    MEM_BLK *pp = (MEM_BLK *)(Base + p);
    Next = pp->Next;
    pp->Next = ((MEM_BLK *)(Base + Next))->Prev = (UInt32)((Byte *)this - Base);
  }
  void Remove(Byte *Base) 
  { 
    ((MEM_BLK *)(Base + Prev))->Next = Next;
    ((MEM_BLK *)(Base + Next))->Prev = Prev;
  }
};


class CSubAllocator
{
  UInt32 SubAllocatorSize;
  Byte Indx2Units[N_INDEXES], Units2Indx[128], GlueCount;
  UInt32 FreeList[N_INDEXES];

  Byte *Base;
  Byte *HeapStart, *LoUnit, *HiUnit;
public:
  Byte *pText, *UnitsStart;
  CSubAllocator():
    SubAllocatorSize(0),
    GlueCount(0),
    LoUnit(0),
    HiUnit(0),
    pText(0),
    UnitsStart(0)
  {
    memset(Indx2Units, 0, sizeof(Indx2Units));
    memset(FreeList, 0, sizeof(FreeList));
  }
  ~CSubAllocator()
  {
    StopSubAllocator();
  };

  void *GetPtr(UInt32 offset) const { return (offset == 0) ? 0 : (void *)(Base + offset); }
  void *GetPtrNoCheck(UInt32 offset) const { return (void *)(Base + offset); }
  UInt32 GetOffset(void *ptr) const { return (ptr == 0) ? 0 : (UInt32)((Byte *)ptr - Base); }
  UInt32 GetOffsetNoCheck(void *ptr) const { return (UInt32)((Byte *)ptr - Base); }
  MEM_BLK *GetBlk(UInt32 offset) const { return (MEM_BLK *)(Base + offset); }
  UInt32 *GetNode(UInt32 offset) const { return (UInt32 *)(Base + offset); }

  void InsertNode(void* p, int indx) 
  {
    *(UInt32 *)p = FreeList[indx];
    FreeList[indx] = GetOffsetNoCheck(p);
  }

  void* RemoveNode(int indx) 
  {
    UInt32 offset = FreeList[indx];
    UInt32 *p = GetNode(offset);
    FreeList[indx] = *p;
    return (void *)p;
  }
  
  UINT U2B(int NU) const { return (UINT)(NU) * UNIT_SIZE; }
  
  void SplitBlock(void* pv, int oldIndx, int newIndx)
  {
    int i, UDiff = Indx2Units[oldIndx] - Indx2Units[newIndx];
    Byte* p = ((Byte*)pv) + U2B(Indx2Units[newIndx]);
    if (Indx2Units[i = Units2Indx[UDiff-1]] != UDiff) 
    {
      InsertNode(p, --i);
      p += U2B(i = Indx2Units[i]);
      UDiff -= i;
    }
    InsertNode(p, Units2Indx[UDiff - 1]);
  }
  
  UInt32 GetUsedMemory() const
  {
    UInt32 RetVal = SubAllocatorSize - (UInt32)(HiUnit - LoUnit) - (UInt32)(UnitsStart - pText);
    for (UInt32 i = 0; i < N_INDEXES; i++) 
      for (UInt32 pn = FreeList[i]; pn != 0; RetVal -= (UInt32)Indx2Units[i] * UNIT_SIZE)
        pn = *GetNode(pn);
    return (RetVal >> 2);
  }
  
  UInt32 GetSubAllocatorSize() const { return SubAllocatorSize; }

  void StopSubAllocator() 
  {
    if (SubAllocatorSize != 0) 
    {
      BigFree(Base);
      SubAllocatorSize = 0;
      Base = 0;
    }
  }

  bool StartSubAllocator(UInt32 size)
  {
    if (SubAllocatorSize == size)              
      return true;
    StopSubAllocator();
    if (size == 0)
      Base = 0;
    else
    {
      if ((Base = (Byte *)::BigAlloc(size + kExtraSize)) == 0)
        return false;
      HeapStart = Base + UNIT_SIZE; // we need such code to support NULL;
    }
    SubAllocatorSize = size;                     
    return true;
  }

  void InitSubAllocator()
  {
    int i, k;
    memset(FreeList, 0, sizeof(FreeList));
    HiUnit = (pText = HeapStart) + SubAllocatorSize;
    UINT Diff = UNIT_SIZE * (SubAllocatorSize / 8 / UNIT_SIZE * 7);
    LoUnit = UnitsStart = HiUnit - Diff;
    for (i = 0, k=1; i < N1 ; i++, k += 1)        Indx2Units[i] = (Byte)k;
    for (k++; i < N1 + N2      ;i++, k += 2)      Indx2Units[i] = (Byte)k;
    for (k++; i < N1 + N2 + N3   ;i++,k += 3)     Indx2Units[i] = (Byte)k;
    for (k++; i < N1 + N2 + N3 + N4; i++, k += 4) Indx2Units[i] = (Byte)k;
    GlueCount = 0;
    for (k = i = 0; k < 128; k++) 
    {
      i += (Indx2Units[i] < k+1);
        Units2Indx[k] = (Byte)i;
    }
  }
  
  void GlueFreeBlocks()
  {
    UInt32 s0 = (UInt32)(HeapStart + SubAllocatorSize - Base);

    // We need add exta MEM_BLK with Stamp=0
    GetBlk(s0)->Stamp = 0;
    s0 += UNIT_SIZE;
    MEM_BLK *ps0 = GetBlk(s0);

    UInt32 p;
    int i;
    if (LoUnit != HiUnit)
      *LoUnit=0;
    ps0->Next = ps0->Prev = s0;

    for (i = 0; i < N_INDEXES; i++)
      while (FreeList[i] != 0) 
      {
        MEM_BLK *pp = (MEM_BLK *)RemoveNode(i);
        pp->InsertAt(Base, s0);
        pp->Stamp = 0xFFFF;
        pp->NU = Indx2Units[i];
      }
    for (p = ps0->Next; p != s0; p = GetBlk(p)->Next)
    {
      for (;;)
      {
        MEM_BLK *pp = GetBlk(p);
        MEM_BLK *pp1 = GetBlk(p + pp->NU * UNIT_SIZE);
        if (pp1->Stamp != 0xFFFF || int(pp->NU) + pp1->NU >= 0x10000)
          break;
        pp1->Remove(Base);
        pp->NU = (UInt16)(pp->NU + pp1->NU);
      }
    }
    while ((p = ps0->Next) != s0) 
    {
      MEM_BLK *pp = GetBlk(p);
      pp->Remove(Base);
      int sz;
      for (sz = pp->NU; sz > 128; sz -= 128, p += 128 * UNIT_SIZE)
        InsertNode(Base + p, N_INDEXES - 1);
      if (Indx2Units[i = Units2Indx[sz-1]] != sz) 
      {
        int k = sz - Indx2Units[--i];
        InsertNode(Base + p + (sz - k) * UNIT_SIZE, k - 1);
      }
      InsertNode(Base + p, i);
    }
  }
  void* AllocUnitsRare(int indx)
  {
    if ( !GlueCount ) 
    {
      GlueCount = 255;
      GlueFreeBlocks();
      if (FreeList[indx] != 0)
        return RemoveNode(indx);
    }
    int i = indx;
    do 
    {
      if (++i == N_INDEXES) 
      {
        GlueCount--;                    
        i = U2B(Indx2Units[indx]);
        return (UnitsStart - pText > i) ? (UnitsStart -= i) : (NULL);
      }
    } while (FreeList[i] == 0);
    void* RetVal = RemoveNode(i);
    SplitBlock(RetVal, i, indx);
    return RetVal;
  }
  
  void* AllocUnits(int NU)
  {
    int indx = Units2Indx[NU - 1];
    if (FreeList[indx] != 0)
      return RemoveNode(indx);
    void* RetVal = LoUnit;
    LoUnit += U2B(Indx2Units[indx]);
    if (LoUnit <= HiUnit)
      return RetVal;
    LoUnit -= U2B(Indx2Units[indx]);
    return AllocUnitsRare(indx);
  }
  
  void* AllocContext()
  {
    if (HiUnit != LoUnit)
      return (HiUnit -= UNIT_SIZE);
    if (FreeList[0] != 0)
      return RemoveNode(0);
    return AllocUnitsRare(0);
  }
  
  void* ExpandUnits(void* oldPtr, int oldNU)
  {
    int i0=Units2Indx[oldNU - 1], i1=Units2Indx[oldNU - 1 + 1];
    if (i0 == i1)
      return oldPtr;
    void* ptr = AllocUnits(oldNU + 1);
    if (ptr) 
    {
      memcpy(ptr, oldPtr, U2B(oldNU));      
      InsertNode(oldPtr, i0);
    }
    return ptr;
  }
  
  void* ShrinkUnits(void* oldPtr, int oldNU, int newNU)
  {
    int i0 = Units2Indx[oldNU - 1], i1 = Units2Indx[newNU - 1];
    if (i0 == i1)
      return oldPtr;
    if (FreeList[i1] != 0) 
    {
      void* ptr = RemoveNode(i1);
      memcpy(ptr, oldPtr, U2B(newNU));
      InsertNode(oldPtr,i0);              
      return ptr;
    } 
    else 
    {
      SplitBlock(oldPtr, i0, i1);
      return oldPtr;
    }
  }
  
  void FreeUnits(void* ptr, int oldNU)
  {
    InsertNode(ptr, Units2Indx[oldNU - 1]);
  }
};

#endif
