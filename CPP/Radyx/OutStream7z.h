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

#ifndef RADYX_OUT_STREAM_7Z_H
#define RADYX_OUT_STREAM_7Z_H

#include "../7zip/IStream.h"
#include "OutputStream.h"

class OutStream7z : public OutputStream
{
public:
	OutStream7z(ISequentialOutStream* stream_)
		: stream(stream_),
		out_processed(0),
		failbit(false)
	{}

	OutputStream& Put(char c) {
		Write(&c, 1);
		return *this;
	}
	OutputStream& Write(const char* s, size_t n);
	void DisableExceptions() {}
	void RestoreExceptions() {}
	bool Fail() const noexcept {
		return failbit;
	}
	const UInt64* GetProcessed() const {
		return &out_processed;
	}

private:
	ISequentialOutStream* stream;
	UInt64 out_processed;
	bool failbit;
};

#endif