///////////////////////////////////////////////////////////////////////////////
//
// Struct: ErrorCode  
//         Represent errors and the OS error code, if any
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

#ifndef RADYX_ERROR_CODE_H
#define RADYX_ERROR_CODE_H

#include "winlean.h"

namespace Radyx {

struct ErrorCode
{
	enum ErrorType
	{
		kGood,
		kMemory,
		kWrite,
		kRunTime,
		kUnknown
	};

	ErrorCode() : type(kGood), os_code(0) {}
	inline void LoadOsErrorCode();

	ErrorType type;
	int os_code;
};

void ErrorCode::LoadOsErrorCode()
{
#ifdef _WIN32
	os_code = GetLastError();
#else
	os_code = errno;
#endif
}

}

#endif