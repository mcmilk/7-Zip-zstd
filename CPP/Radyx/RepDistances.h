#ifndef RADYX_REP_DISTANCES_H
#define RADYX_REP_DISTANCES_H

namespace Radyx {

template<size_t kNumReps>
struct RepDistances
{
	union {
		uint_fast32_t reps[kNumReps];
		size_t rep_copier[kNumReps / 2];
	};

	uint_fast32_t& operator[](size_t index) NOEXCEPT {
		return reps[index];
	}
	const uint_fast32_t& operator[](size_t index) const NOEXCEPT {
		return reps[index];
	}
	inline void operator=(const RepDistances<kNumReps>& rvalue) NOEXCEPT;
};

template<size_t kNumReps>
void RepDistances<kNumReps>::operator=(const RepDistances<kNumReps>& rvalue) NOEXCEPT
{
	if (sizeof(rep_copier) == sizeof(reps)) {
		rep_copier[0] = rvalue.rep_copier[0];
		rep_copier[1] = rvalue.rep_copier[1];
	}
	else {
		reps[0] = rvalue.reps[0];
		reps[1] = rvalue.reps[1];
		reps[2] = rvalue.reps[2];
		reps[3] = rvalue.reps[3];
	}
}

}

#endif // RADYX_REP_DISTANCES_H