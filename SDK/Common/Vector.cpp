// Common/Vector.cpp

#include "StdAfx.h"

#include "Vector.h"

static const kMaxVectorSize = 0x20000000;

CBaseRecordVector::CBaseRecordVector(size_t anItemSize):
  m_Size(0),
  m_Capacity(0),
  m_Items(NULL),
  m_ItemSize(anItemSize)
{}

CBaseRecordVector::~CBaseRecordVector()
{
  //  Clear();
  delete []((BYTE *)m_Items);
}

void CBaseRecordVector::Grow()
{
  int aDelta;
  if (m_Capacity > 64)
    aDelta = m_Capacity / 2;
  else if (m_Capacity > 8)
    aDelta = 16;
  else
    aDelta = 4;
  Reserve(m_Capacity + aDelta);
}

void CBaseRecordVector::ReserveOnePosition()
{
  if(m_Size == m_Capacity)
    Grow();
}

void CBaseRecordVector::Reserve(int aNewCapacity)
{
  if(aNewCapacity <= m_Capacity)
    return;
  #ifndef _WIN32_WCE
  if(aNewCapacity < m_Size || aNewCapacity > kMaxVectorSize) 
    throw 1052354;
  #endif
  if(aNewCapacity != m_Capacity)
  {
    BYTE *p;
    if (aNewCapacity == 0)
      p = NULL;
    else
    {
      p = new BYTE[aNewCapacity * m_ItemSize];
      int aNumRecordsToMove;
      if(aNewCapacity<m_Capacity)
        aNumRecordsToMove = aNewCapacity;
      else
        aNumRecordsToMove = m_Capacity;
      memmove(p, m_Items, m_ItemSize * aNumRecordsToMove);
    }
    delete []m_Items;
    m_Items = p;
  }
  m_Capacity = aNewCapacity;
}

void CBaseRecordVector::Clear()
{
  DeleteFrom(0);
}

void CBaseRecordVector::TestIndexAndCorrectNum(int anIndex, int &aNum) const
{
  if (anIndex + aNum > m_Size)
    aNum = m_Size - anIndex;
}

void CBaseRecordVector::MoveItems(int aDestinationIndex, int aSourceIndex)
{
  memmove((BYTE *)m_Items + aDestinationIndex * m_ItemSize, 
    (BYTE *)m_Items + aSourceIndex * m_ItemSize, 
    m_ItemSize * (m_Size - aSourceIndex));
}

void CBaseRecordVector::InsertOneItem(int anIndex)
{
  int aNum = 1;
  TestIndexAndCorrectNum(anIndex, aNum);
  ReserveOnePosition();
  MoveItems(anIndex + 1, anIndex);
  m_Size++;
}

void CBaseRecordVector::Delete(int anIndex, int aNum)
{
  TestIndexAndCorrectNum(anIndex, aNum);
  if (aNum > 0)
  {
    MoveItems(anIndex, anIndex + aNum);
    m_Size -= aNum;
  }
}

void CBaseRecordVector::DeleteFrom(int anIndex)
{
  Delete(anIndex, m_Size - anIndex); // m_Size - anIndex may be < 0 
}

void CBaseRecordVector::DeleteBack()
{
  DeleteFrom(m_Size - 1); // m_Size must be > 0
}
