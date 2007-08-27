// SplitUtils.h

#ifndef __SPLITUTILS_H
#define __SPLITUTILS_H

#include "Common/MyString.h"
#include "Common/Types.h"
#include "Windows/Control/ComboBox.h"

bool ParseVolumeSizes(const UString &s, CRecordVector<UInt64> &values);
void AddVolumeItems(NWindows::NControl::CComboBox &volumeCombo);

UInt64 GetNumberOfVolumes(UInt64 size, CRecordVector<UInt64> &volSizes);

#endif
