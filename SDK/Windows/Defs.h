// Windows/Defs.h

#pragma once

#ifndef __WINDOWS_DEFS_H
#define __WINDOWS_DEFS_H

inline bool BOOLToBool(BOOL aValue)
  { return (aValue != FALSE); }

inline BOOL BoolToBOOL(bool aValue)
  { return (aValue ? TRUE: FALSE); }

inline VARIANT_BOOL BoolToVARIANT_BOOL(bool aValue)
  { return (aValue ? VARIANT_TRUE: VARIANT_FALSE); }

inline bool VARIANT_BOOLToBool(VARIANT_BOOL aValue)
  { return (aValue != VARIANT_FALSE); }

#define RETURN_IF_NOT_S_OK(x) { HRESULT __aResult_ = (x); if(__aResult_ != S_OK) return __aResult_; }

#endif
