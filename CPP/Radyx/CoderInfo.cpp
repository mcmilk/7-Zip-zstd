///////////////////////////////////////////////////////////////////////////////
//
// Class:   CoderInfo
//          Information for defining the encoding used
//
// Copyright 2015 Conor McCarthy
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

#include "CoderInfo.h"

namespace Radyx {

size_t CoderInfo::MethodId::GetIdString(IdString& str) const noexcept
{
	uint_least64_t id = method_id;
	size_t length = 1;
	for (; length < sizeof(id); ++length) {
		if ((id >> (8u * length)) == 0) {
			break;
		}
	}
	for (ptrdiff_t i = length - 1; i >= 0; --i) {
		str[i] = static_cast<uint8_t>(id & 0xFF);
		id >>= 8;
	}
	return length;
}

}