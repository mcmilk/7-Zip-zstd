///////////////////////////////////////////////////////////////////////////////
//
// Class: MatchTable
//        Create a table of string matches
//
// Copyright 1998-2000, 2015 Conor McCarthy
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

#include "common.h"
#include "MatchTable.h"

namespace Radyx {

const std::array<UintFast32, 4> PackedMatchTable::kMatchBufferSize =
{ 768 * 1024, 300 * 1024, 180 * 1024, 128 * 1024 };

}