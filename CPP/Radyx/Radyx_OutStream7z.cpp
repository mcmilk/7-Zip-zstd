///////////////////////////////////////////////////////////////////////////////
//
// Class: OutStream7z
//        Output stream wrapper for 7-zip streams in the radyx encoder
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

#include "../Common/Common.h"
#include "../7zip/Common/StreamUtils.h"

#include "OutStream7z.h"

OutputStream& OutStream7z::Write(const char* s, size_t n)
{
	HRESULT err = WriteStream(stream, s, n);
	out_processed += n;

	failbit |= (err != S_OK);

	return *this;
}
