#ifndef INCLUDED_CW_CHRONO
#define INCLUDED_CW_CHRONO
#include <chrono>
#include <Windows.h>

#pragma comment( lib, "Winmm" )

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace cw {

	struct perf_freq {
		LONGLONG value;
		perf_freq() { ::QueryPerformanceFrequency( (LARGE_INTEGER*)&value ); }
		static LONGLONG get() {
			static const perf_freq freq;
			return freq.value;
		}
	};

	struct high_resolution_clock {
		using rep = LONGLONG;
		using period = std::ratio<1>;
		using duration = std::chrono::duration<rep>;
		using time_point = std::chrono::time_point<high_resolution_clock>;
		static const bool is_steady = false;

		static time_point now() {
			LONGLONG time;
			::QueryPerformanceCounter( (LARGE_INTEGER*)&time );
			return time_point(duration(time));
		}
	};

}

namespace std {
	namespace chrono {
		template<>
		inline duration<double> duration_cast<duration<double>>( const cw::high_resolution_clock::duration& dur ) {
			return duration<double>( double(dur.count()) / double(cw::perf_freq::get()) );
		}
	}
}

#endif
