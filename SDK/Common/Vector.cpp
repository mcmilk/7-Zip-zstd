// Common/Vector.cpp

#include "StdAfx.h"

#include "Vector.h"

static const kMaxVectorSize = 0x20000000;

CBaseRecordVector::CBaseRecordVector(size_t itemSize):
  _size(0),
  _capacity(0),
  _items(NULL),
  _itemSize(itemSize)
{}

CBaseRecordVector::~CBaseRecordVector()
{
  //  Clear();
  delete []((BYTE *)_items);
}

void CBaseRecordVector::Grow()
{
  int delta;
  if (_capacity > 64)
    delta = _capacity / 2;
  else if (_capacity > 8)
    delta = 16;
  else
    delta = 4;
  Reserve(_capacity + delta);
}

void CBaseRecordVector::ReserveOnePosition()
{
  if(_size == _capacity)
    Grow();
}

void CBaseRecordVector::Reserve(int newCapacity)
{
  if(newCapacity <= _capacity)
    return;
  #ifndef _WIN32_WCE
  if(newCapacity < _size || newCapacity > kMaxVectorSize) 
    throw 1052354;
  #endif
  if(newCapacity != _capacity)
  {
    BYTE *p;
    if (newCapacity == 0)
      p = NULL;
    else
    {
      p = new BYTE[newCapacity * _itemSize];
      int numRecordsToMove;
      if(newCapacity<_capacity)
        numRecordsToMove = newCapacity;
      else
        numRecordsToMove = _capacity;
      memmove(p, _items, _itemSize * numRecordsToMove);
    }
    delete []_items;
    _items = p;
  }
  _capacity = newCapacity;
}

void CBaseRecordVector::Clear()
{
  DeleteFrom(0);
}

void CBaseRecordVector::TestIndexAndCorrectNum(int index, int &num) const
{
  if (index + num > _size)
    num = _size - index;
}

void CBaseRecordVector::MoveItems(int destIndex, int srcIndex)
{
  memmove((BYTE *)_items + destIndex * _itemSize, 
    (BYTE *)_items + srcIndex * _itemSize, 
    _itemSize * (_size - srcIndex));
}

void CBaseRecordVector::InsertOneItem(int index)
{
  int num = 1;
  TestIndexAndCorrectNum(index, num);
  ReserveOnePosition();
  MoveItems(index + 1, index);
  _size++;
}

void CBaseRecordVector::Delete(int index, int num)
{
  TestIndexAndCorrectNum(index, num);
  if (num > 0)
  {
    MoveItems(index, index + num);
    _size -= num;
  }
}

void CBaseRecordVector::DeleteFrom(int index)
{
  Delete(index, _size - index); // _size - index may be < 0 
}

void CBaseRecordVector::DeleteBack()
{
  DeleteFrom(_size - 1); // _size must be > 0
}
