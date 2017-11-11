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

#ifndef RADYX_CODER_INFO_H
#define RADYX_CODER_INFO_H

#include <array>
#include "common.h"

namespace Radyx {
	
struct CoderInfo
{
	struct MethodId
	{
		typedef std::array<uint8_t, 15> IdString;

		uint_least64_t method_id;
		MethodId(uint_least64_t method_id_) noexcept
			: method_id(method_id_) {}
		size_t GetIdString(IdString& str) const noexcept;
	};

	std::basic_string<uint8_t> props;
	MethodId method_id;
	unsigned num_in_streams;
	unsigned num_out_streams;

	CoderInfo()
		: method_id(0),
		num_in_streams(0),
		num_out_streams(0) {}
	inline CoderInfo(const uint8_t* props_,
		unsigned props_count_,
		uint_least64_t method_id_,
		unsigned num_in_streams_,
		unsigned num_out_streams_);
	bool IsComplex() const noexcept {
		return num_in_streams != 1 || num_out_streams != 1;
	}
	uint8_t GetHeaderFlags() const noexcept {
		return (IsComplex() ? 0x10 : 0) | ((props.length() != 0) ? 0x20 : 0);
	}
};

CoderInfo::CoderInfo(const uint8_t* props_,
	unsigned props_count_,
	uint_least64_t method_id_,
	unsigned num_in_streams_,
	unsigned num_out_streams_)
	: method_id(method_id_),
	num_in_streams(num_in_streams_),
	num_out_streams(num_out_streams_)
{
	props.assign(props_, props_count_);
}

}

#endif