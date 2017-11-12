///////////////////////////////////////////////////////////////////////////////
//
// Class: OutputStream
//        Interface for file writing
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

#ifndef RADYX_OUTPUT_STREAM_H
#define RADYX_OUTPUT_STREAM_H

#include "common.h"

class OutputStream
{
public:

	virtual OutputStream& Put(char c) = 0;
	virtual OutputStream& Write(const char* s, size_t n) = 0;
	virtual void DisableExceptions() = 0;
	virtual void RestoreExceptions() = 0;
	virtual bool Fail() const NOEXCEPT = 0;
};

#endif