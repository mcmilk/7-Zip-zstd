///////////////////////////////////////////////////////////////////////////////
//
// Class: OptionalSetting
//        Store a setting that can be modified by the user
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

#ifndef RADYX_OPTIONAL_SETTING_H
#define RADYX_OPTIONAL_SETTING_H

namespace Radyx {

template<class T>
class OptionalSetting
{
public:
	OptionalSetting(T value_) : value(value_), is_set(false) {}
	void operator=(const OptionalSetting<T>& right);
	void operator=(T value_) {
		Set(value_);
	}
	inline void Set(T value_);

	inline T Get() const {
		return value;
	}
	bool IsSet() const {
		return is_set;
	}
	operator T() const {
		return value;
	}

private:
	T value;
	bool is_set;
};

template<class T>
void OptionalSetting<T>::operator=(const OptionalSetting<T>& right)
{
	value = right.value;
	is_set = right.is_set;
}

template<class T>
void OptionalSetting<T>::Set(T value_)
{
	value = value_;
	is_set = true;
}

}

#endif