///////////////////////////////////////////////////////////////////////////////
//
// Commonly used inclusions and definitions.
//
// Copyright 2017 Conor McCarthy
//
// This file is part of Radyx.
//
// Radyx is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Radyx is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Radyx. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RADYX_COMMON_H_
#define RADYX_COMMON_H_

#include "../../C/7zTypes.h"

#include <cstddef>
#include <thread>
#include <cassert>

namespace Radyx {

/**
 * define NOEXCEPT if supported
 * - should work with clang, gcc and msvc
 */
#if defined(_MSC_VER) && defined(_NOEXCEPT)
#pragma warning(disable : 4512)
# define NOEXCEPT _NOEXCEPT
#elif defined(__clang__) && __has_feature(cxx_noexcept) || \
  defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46 || \
# define NOEXCEPT noexcept
#else
# define NOEXCEPT
#endif

extern volatile bool g_break;
typedef UInt32 UintFast32;

}

#endif // RADYX_COMMON_H_
