#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>
#include <list>
#include <cw/list.h>
//#include <cw/list_algorithm.h>
#include "logarithmic_range.h"

using namespace std;
using namespace cw;

#ifdef _MSC_VER
#include "chrono.h"
using hrc = cw::high_resolution_clock;
#else
using hrc = std::chrono::high_resolution_clock;
#endif

static const double scale = 1.0e6;

// Preallocate strategy

struct preallocate_enable {
	template<typename L>
	void operator()( L& v, size_t N ) {
		v.reserve(N);
	}

	// can't reserve a std::list
	template<typename T>
	void operator()( std::list<T>&, size_t ) {}
};

struct preallocate_disable {
	template<typename L>
	void operator()( L& v, size_t N ) {}
};

// Create a container of type L with N elements using the fill strategy F and the preallocate strategy P

template<typename L,typename F,typename P = preallocate_enable>
L create( size_t N ) {
	L v;
	P()( v, N );
	F()( v, N );
	return v;
}

// Front -- create using emplace_front.

struct fill_front {
	template<typename L>
	void operator()( L& v, size_t N ) {
		using T = L::value_type;
		for(size_t i=0;i<N;++i) {
			v.emplace_front( T(i) );
		}
	}
};

// Back -- create using emplace_back.

struct fill_back {
	template<typename L>
	void operator()( L& v, size_t N ) {
		using T = L::value_type;
		for(size_t i=0;i<N;++i) {
			v.emplace_back( T(i) );
		}
	}
};

// Alt -- create by alternating front and back.

struct fill_alt {
	template<typename L>
	void operator()( L& v, size_t N ) {
		using T = L::value_type;
		size_t M = N / 2;
		for(size_t i=0;i<M;++i) {
			v.emplace_back( T(2*i) );
			v.emplace_front( T(2*i+1) );
		}
		size_t remain = N - M*2;
		if( remain > 0 ) {
			v.emplace_back( T(N-1) );
		}
	}
};

// Mid -- create by insertion at midpoint, no searching.

struct fill_mid {
	template<typename L>
	void operator()( L& v, size_t N ) {
		using T = L::value_type;
		auto it = begin(v);
		for(size_t i=0;i<N;++i) {
			it = v.insert( it, T(i) );
			if( i % 2 == 0 ) ++it;
		}
	}
};

// Back Random -- random values inserted at the back.

struct fill_back_random {
	mt19937 mt;
	template<typename L>
	void operator()( L& v, size_t N ) {
		using T = L::value_type;
		uint32_t upper = (uint32_t)min<uint64_t>( numeric_limits<uint32_t>::max(), numeric_limits<T>::max() );
		uniform_int_distribution<uint32_t> dist( 0, upper );
		for(size_t i=0;i<N;++i) {
			auto r = dist(mt);
			v.emplace_back( r );
		}
	}
};

// FB Random -- front/back chosen at random.

struct fill_fb_random {
	mt19937 mt;
	template<typename L>
	void operator()( L& v, size_t N ) {
		using T = L::value_type;
		bernoulli_distribution dist;
		for(size_t i=0;i<N;++i) {
			auto r = dist(mt);
			if( r )
				v.emplace_front( T(i) );
			else
				v.emplace_back( T(i) );
		}
	}
};

// Random Sorted -- insert random values, keeping list sorted.

struct fill_random_sorted {
	mt19937 mt;
	template<typename L>
	void operator()( L& v, size_t N ) {
		using T = L::value_type;
		uint32_t upper = (uint32_t)min<uint64_t>( numeric_limits<uint32_t>::max(), numeric_limits<T>::max() );
		uniform_int_distribution<uint32_t> dist( 0, upper );
		for(size_t i=0;i<N;++i) {
			auto r = dist(mt);
			v.insert( lower_bound( begin(v), end(v), T(r) ), T(r) );
		}
	}
};

template<typename F>
double time( F&& f ) {
	auto t1 = hrc::now();
	std::forward<F>(f)();
	auto t2 = hrc::now();
	using namespace std::chrono;
	return duration_cast<duration<double>>(t2 - t1).count();
}

template<typename L>
double test_accumulate( const L& v, int N = 1 ) {
	return time( [&]{
		for(int i=0;i<N;++i) {
			volatile auto dont_optimize_me = accumulate( begin(v), end(v), uint64_t(0) );
		}
	});
}

template<typename L>
double test_adjacent_difference( const L& v, int N = 1 ) {
	using T = L::value_type;
	vector<T> result(v.size());
	return time( [&]{
		for(int i=0;i<N;++i) {
			adjacent_difference( begin(v), end(v), begin(result) );
		}
	});
}

template<typename L>
double test_traversal( const L& v, int N = 1 ) {
	return time( [&]{
		auto e = end(v);
		for(int i=0;i<N;++i) {
			uint64_t count = 0;
			for( auto it = begin(v); it != e; ++it ) {
				++count;
			}
			volatile uint64_t dont_optimize_me = count;
		}
	});
}

template<typename L>
double test_sort( L& v ) {
	return time( [&]{
		stable_sort( begin(v), end(v) );
	});
}

template<typename L>
double test_list_sort( L& v ) {
	return time( [&]{
		v.sort();
	});
}

template<typename T,int N = 1>
struct data_array {
	T b[N];
	data_array() { memset(b,0,N*sizeof(T)); }
	data_array( T i ) {
		memset(b,0,N*sizeof(T));
		b[0] = i;
	}
	operator T() const {
		return b[0];
	}
};

namespace std {
	template<typename T,int N>
	struct numeric_limits<data_array<T,N>> : numeric_limits<T> {};
}

template<typename L,typename P>
void test_vec( vector<double>& times, size_t N, int repeat ) {
	L v;
	double factor = scale / repeat;
	times.push_back( time([&]{
		v = create<L,fill_back,P>(N);
	}) * scale );
	times.push_back( test_accumulate( v, repeat ) * factor );
	times.push_back( test_adjacent_difference( v, repeat ) * factor );
}

template<typename L,typename P>
void test_list( vector<double>& times, size_t N, int repeat ) {
	L v;
	double factor = scale / repeat;
	times.push_back( time([&]{
		v = create<L,fill_back,P>(N);
	}) * scale );
	times.push_back( test_accumulate( v, repeat ) * factor );
	times.push_back( test_adjacent_difference( v, repeat ) * factor );
	times.push_back( test_traversal( v, repeat ) * factor );

	times.push_back( time([&]{
		v = create<L,fill_mid,P>(N);
	}) * scale );
	times.push_back( test_accumulate( v, repeat ) * factor );
	times.push_back( test_adjacent_difference( v, repeat ) * factor );
	times.push_back( test_traversal( v, repeat ) * factor );

	times.push_back( time([&]{
		v = create<L,fill_fb_random,P>(N);
	}) * scale );
	times.push_back( test_accumulate( v, repeat ) * factor );
	times.push_back( test_adjacent_difference( v, repeat ) * factor );
	times.push_back( test_traversal( v, repeat ) * factor );

}

template<typename L,typename P>
void test_sorting( vector<double>& times, size_t N ) {
	L v;

	v = create<L,fill_front,P>(N);
	times.push_back( test_sort( v ) * scale );
	
	v = create<L,fill_front,P>(N);
	times.push_back( test_list_sort( v ) * scale );

	v = create<L,fill_back,P>(N);
	times.push_back( test_sort( v ) * scale );
	
	v = create<L,fill_back,P>(N);
	times.push_back( test_list_sort( v ) * scale );
	
	v = create<L,fill_alt,P>(N);
	times.push_back( test_sort( v ) * scale );
	
	v = create<L,fill_alt,P>(N);
	times.push_back( test_list_sort( v ) * scale );
	
	v = create<L,fill_mid,P>(N);
	times.push_back( test_sort( v ) * scale );
	
	v = create<L,fill_mid,P>(N);
	times.push_back( test_list_sort( v ) * scale );
}

template<typename L,typename P>
void test_random( vector<double>& times, size_t N ) {
	L v;

	times.push_back( time([&]{
		v = create<L,fill_random_sorted,P>(N);
	}) * scale );
}

template<typename T,typename U,typename P>
void benchmark( ofstream& out ) {
	size_t maxN = numeric_limits<U>::max();
	size_t bits = sizeof(U) * 8;
	size_t minN = ( bits > 8 ) ? ( 1 << (bits / 2) ) : 1;
	size_t maxBytes = 1 << 27;
	maxN = min( maxN, maxBytes / ( 2 * sizeof(void*) + sizeof(T) ) );

	size_t M = 6000000;
	size_t maxIts = 1000;

	vector<double> times;
	times.reserve(100);

	for( auto i : log_range( minN, maxN, maxIts, size_t(1) ) ) {
		times.clear();
		cout << i << endl;
		int repeat = int( max( M / i , size_t(1) ) );
		test_vec<vector<T>,P>( times, i, repeat );
		test_list<std::list<T>,P>( times, i, repeat );
		test_list<cw::list<T,U>,P>( times, i, repeat );

		out << i << "," << repeat << ",";
		for( auto t : times )
			out << t << ",";
		int start = 3;
		int nTests = 12;
		for(int j=0;j<nTests;++j)
			out << times[start + j] / times[start + nTests + j] << ",";

		for(int j=0;j<start;++j)
			out << times[start + nTests + j] / times[j] << ",";

		out << endl;
	}

}

template<typename T,typename U,typename P>
void benchmark_sorting( ofstream& out ) {
	size_t maxN = numeric_limits<U>::max();
	size_t bits = sizeof(U) * 8;
	size_t minN = ( bits > 8 ) ? ( 1 << (bits / 2) ) : 1;
	size_t maxBytes = 1 << 17;
	maxN = min( maxN, maxBytes / ( 2 * sizeof(void*) + sizeof(T) ) );

	size_t maxIts = 1000;

	vector<double> times;
	times.reserve(100);

	for( auto i : log_range( minN, maxN, maxIts, size_t(1) ) ) {
		times.clear();
		cout << i << endl;
		test_sorting<std::list<T>,P>( times, i );
		test_sorting<cw::list<T,U>,P>( times, i );
		out << i << ",";
		for( auto t : times )
			out << t << ",";
		int start = 0;
		int nTests = 8;
		for(int j=0;j<nTests;++j)
			out << times[start + j] / times[start + nTests + j] << ",";

		out << endl;
	}

}

template<typename T,typename U,typename P>
void benchmark_random( ofstream& out ) {
	size_t maxN = numeric_limits<U>::max();
	size_t bits = sizeof(U) * 8;
	size_t minN = ( bits > 8 ) ? ( 1 << (bits / 2) ) : 1;
	size_t maxBytes = 1 << 17;
	maxN = min( maxN, maxBytes / ( 2 * sizeof(void*) + (size_t)sqrt( 0.75 * sizeof(T) ) ) );

	size_t maxIts = 1000;

	vector<double> times;
	times.reserve(100);

	for( auto i : log_range( minN, maxN, maxIts, size_t(1) ) ) {
		times.clear();
		cout << i << endl;
		test_random<vector<T>,P>( times, i );
		test_random<std::list<T>,P>( times, i );
		test_random<cw::list<T,U>,P>( times, i );
		out << i << ",";
		for( auto t : times )
			out << t << ",";

		out << times[1] / times[2] << ",";
		out << times[1] / times[0] << ",";
		out << times[2] / times[0] << ",";

		out << endl;
	}

}

void print_header( ofstream& out ) {
	out << "size,"
	       "repeat,"

		   "create_back vector,"
		   "accumulate,"
		   "adjacent_difference,"

		   "create_back stdlist,"
		   "accumulate,"
		   "adjacent_difference,"
		   "traversal,"
		   "create_mid stdlist,"
		   "accumulate,"
		   "adjacent_difference,"
		   "traversal,"
		   "create_fb stdlist,"
		   "accumulate,"
		   "adjacent_difference,"
		   "traversal,"

		   "create_back cwlist,"
		   "accumulate,"
		   "adjacent_difference,"
		   "traversal,"
		   "create_mid cwlist,"
		   "accumulate,"
		   "adjacent_difference,"
		   "traversal,"
		   "create_fb cwlist,"
		   "accumulate,"
		   "adjacent_difference,"
		   "traversal,"

		   "create_back ratio,"
		   "accumulate ratio,"
		   "adjacent_difference ratio,"
		   "traversal ratio,"
		   "create_mid ratio,"
		   "accumulate ratio,"
		   "adjacent_difference ratio,"
		   "traversal ratio,"
		   "create_fb ratio,"
		   "accumulate ratio,"
		   "adjacent_difference ratio,"
		   "traversal ratio,"

		   "create_back vec ratio,"
		   "accumulate vec ratio,"
		   "adjacent_difference vec ratio,"
	<< endl;
}

int main1() {

	using P = preallocate_enable;
	{
		ofstream out("output/results1.csv");
		print_header(out);

		using T = uint8_t;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results2.csv");
		print_header(out);

		using T = uint16_t;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results4.csv");
		print_header(out);

		using T = uint32_t;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results8.csv");
		print_header(out);

		using T = uint64_t;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results16.csv");
		print_header(out);

		using T = data_array<uint64_t,2>;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results32.csv");
		print_header(out);

		using T = data_array<uint64_t,4>;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results64.csv");
		print_header(out);

		using T = data_array<uint64_t,8>;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results128.csv");
		print_header(out);

		using T = data_array<uint64_t,16>;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	{
		ofstream out("output/results1k.csv");
		print_header(out);

		using T = data_array<uint64_t,128>;

		benchmark<T,uint8_t,P>( out );
		benchmark<T,uint16_t,P>( out );
		benchmark<T,uint32_t,P>( out );
	}

	return 0;
}

int main2() {

	using P = preallocate_enable;
	{
		ofstream out("output/is1.csv");

		using T = uint8_t;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	{
		ofstream out("output/is2.csv");

		using T = uint16_t;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	{
		ofstream out("output/is4.csv");

		using T = uint32_t;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	{
		ofstream out("output/is8.csv");

		using T = uint64_t;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	{
		ofstream out("output/is16.csv");

		using T = data_array<uint64_t,2>;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	{
		ofstream out("output/is32.csv");

		using T = data_array<uint64_t,4>;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	{
		ofstream out("output/is64.csv");

		using T = data_array<uint64_t,8>;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	{
		ofstream out("output/is128.csv");

		using T = data_array<uint64_t,16>;

		benchmark_sorting<T,uint8_t,P>( out );
		benchmark_sorting<T,uint16_t,P>( out );
	}

	return 0;
}

int main3() {

	using P = preallocate_enable;
	{
		ofstream out("output/random1.csv");

		using T = uint8_t;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random2.csv");

		using T = uint16_t;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random4.csv");

		using T = uint32_t;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random8.csv");

		using T = uint64_t;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random16.csv");

		using T = data_array<uint64_t,2>;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random32.csv");

		using T = data_array<uint64_t,4>;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random64.csv");

		using T = data_array<uint64_t,8>;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random128.csv");

		using T = data_array<uint64_t,16>;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random256.csv");

		using T = data_array<uint64_t,32>;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random512.csv");

		using T = data_array<uint64_t,64>;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}
	{
		ofstream out("output/random1k.csv");

		using T = data_array<uint64_t,128>;

		benchmark_random<T,uint8_t,P>( out );
		benchmark_random<T,uint16_t,P>( out );
		benchmark_random<T,uint32_t,P>( out );
	}

	return 0;
}

int main() {
	main1();
	//main2();
	main3();
}
