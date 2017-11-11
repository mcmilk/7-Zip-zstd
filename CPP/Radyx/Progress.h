///////////////////////////////////////////////////////////////////////////////
//
// Class: Progress
//        Dummy progress meter class - does nothing
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

#ifndef RADYX_PROGRESS_H
#define RADYX_PROGRESS_H

namespace Radyx {

class Progress
{
public:
	Progress(uint_least64_t, unsigned) {}
	inline void Show() {}
	inline void Rewind() {}
	void RewindLocked() {}
	inline void Erase() {}
	inline void BuildUpdate(size_t) {}
	inline void EncodeUpdate(size_t) {}
	inline void Adjust(int_least64_t) {}
};

}

#endif // RADYX_PROGRESS_H