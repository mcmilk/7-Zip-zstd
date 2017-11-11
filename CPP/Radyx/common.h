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

#include <cstddef>
#include <cinttypes>
#include <thread>
#include <cassert>

namespace Radyx {

extern volatile bool g_break;

#ifdef USE_64BIT_FAST_INT
typedef uint_fast64_t UintFast32;
#else
typedef uint_least32_t UintFast32;
#endif

}

#endif // RADYX_COMMON_H_