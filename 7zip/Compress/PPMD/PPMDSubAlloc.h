// PPMDSubAlloc.h
// This code is based on Dmitry Shkarin's PPMdH code

#ifndef __PPMD_SUBALLOC_H
#define __PPMD_SUBALLOC_H

#include "PPMDType.h"

#include "../../../Common/Alloc.h"

const UINT N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4;
const UINT UNIT_SIZE=12, N_INDEXES=N1+N2+N3+N4;

#pragma pack(1)
struct MEM_BLK 
{
  UInt16 Stamp, NU;
  MEM_BLK *Next, *Prev;
  void InsertAt(MEM_BLK* p) 
  {
    Next = (Prev = p)->Next;
    p->Next = Next->Prev = this;
  }
  void Remove() 
  { 
    Prev->Next=Next;
    Next->Prev=Prev;
  }
} _PACK_ATTR;
#pragma pack()


class CSubAllocator
{
  UInt32 SubAllocatorSize;
  Byte Indx2Units[N_INDEXES], Units2Indx[128], GlueCount;
  struct NODE { NODE* Next; } FreeList[N_INDEXES];
public:
  Byte* HeapStart, *pText, *UnitsStart, *LoUnit, *HiUnit;
  CSubAllocator():
    SubAllocatorSize(0),
    GlueCount(0),
    pText(0),
    UnitsStart(0),
    LoUnit(0),
    HiUnit(0)
  {
    memset(Indx2Units, 0, sizeof(Indx2Units));
    memset(FreeList, 0, sizeof(FreeList));
  }
  ~CSubAllocator()
  {
    StopSubAllocator();
  };


  void InsertNode(void* p, int indx) 
  {
    ((NODE*) p)->Next = FreeList[indx].Next;
    FreeList[indx].Next = (NODE*)p;
  }

  void* RemoveNode(int indx) 
  {
    NODE* RetVal = FreeList[indx].Next;
    FreeList[indx].Next = RetVal->Next;
    return RetVal;
  }
  
  UINT U2B(int NU) { return 8 * NU + 4 * NU; }
  
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
  
  UInt32 GetUsedMemory()
  {
    UInt32 i, k, RetVal = SubAllocatorSize - (UInt32)(HiUnit - LoUnit) - (UInt32)(UnitsStart - pText);
    for (k = i = 0; i < N_INDEXES; i++, k = 0) 
    {
      for (NODE* pn = FreeList + i;(pn = pn->Next) != NULL; k++)
        ;
      RetVal -= UNIT_SIZE*Indx2Units[i] * k;
    }
    return (RetVal >> 2);
  }
  
  void StopSubAllocator() 
  {
    if ( SubAllocatorSize ) 
    {
      BigFree(HeapStart);
      SubAllocatorSize = 0;
      HeapStart = 0;
    }
  }

  bool StartSubAllocator(UInt32 size)
  {
    if (SubAllocatorSize == size)              
      return true;
    StopSubAllocator();
    if ((HeapStart = (Byte *)::BigAlloc(size)) == 0)
      return false;
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
    for (i = 0, k=1; i < N1 ; i++, k += 1)        Indx2Units[i]=k;
    for (k++; i < N1 + N2      ;i++, k += 2)      Indx2Units[i]=k;
    for (k++; i < N1 + N2 + N3   ;i++,k += 3)     Indx2Units[i]=k;
    for (k++; i < N1 + N2 + N3 + N4; i++, k += 4) Indx2Units[i]=k;
    for (GlueCount = k = i = 0; k < 128; k++) 
    {
      i += (Indx2Units[i] < k+1);
        Units2Indx[k]=i;
    }
  }
  
  void GlueFreeBlocks()
  {
    MEM_BLK s0, *p, *p1;
    int i, k, sz;
    if (LoUnit != HiUnit)
      *LoUnit=0;
    for (i = 0, s0.Next = s0.Prev = &s0; i < N_INDEXES; i++)
      while ( FreeList[i].Next ) 
      {
        p = (MEM_BLK*) RemoveNode(i);
        p->InsertAt(&s0);
        p->Stamp = 0xFFFF;
        p->NU = Indx2Units[i];
      }
    for (p=s0.Next; p != &s0; p =p->Next)
      while ((p1 = p + p->NU)->Stamp == 0xFFFF && int(p->NU) + p1->NU < 0x10000) 
      {
        p1->Remove();
        p->NU += p1->NU;
      }
    while ((p=s0.Next) != &s0) 
    {
      for (p->Remove(), sz=p->NU; sz > 128; sz -= 128, p += 128)
        InsertNode(p, N_INDEXES - 1);
      if (Indx2Units[i = Units2Indx[sz-1]] != sz) 
      {
        k = sz-Indx2Units[--i];
        InsertNode(p + (sz - k), k - 1);
      }
      InsertNode(p,i);
    }
  }
  void* AllocUnitsRare(int indx)
  {
    if ( !GlueCount ) 
    {
      GlueCount = 255;
      GlueFreeBlocks();
      if (FreeList[indx].Next)
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
    } while (!FreeList[i].Next);
    void* RetVal = RemoveNode(i);
    SplitBlock(RetVal, i, indx);
    return RetVal;
  }
  
  void* AllocUnits(int NU)
  {
    int indx = Units2Indx[NU - 1];
    if (FreeList[indx].Next)
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
    if (FreeList->Next)
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
    if ( FreeList[i1].Next ) 
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
