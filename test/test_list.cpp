#include <iostream>
#include <algorithm>
#include <numeric>
#include <list>
#include <random>
#include <cw/list.h>

using namespace std;
using namespace cw;

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

// Random -- random values inserted at the back.

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
			v.insert( find_if( begin(v), end(v), [=]( auto x ){ return uint32_t(x) > r; } ), T(r) );
		}
	}
};

template<typename L1,typename L2>
bool compare( const L1& v1, const L2& v2 ) {

	auto it1 = begin(v1);
	auto it2 = begin(v2);

	auto e1 = end(v1);
	auto e2 = end(v2);
	size_t error_count = 0;
	while( it1 != e1 && it2 != e2 ) {
		if( *it1 != *it2 )
			return false;
		++it1; ++it2;
	}
	return true;
}

void test_merge() {

	using T = uint16_t;
	size_t N = 16000;

	auto c1 = create<cw::list<T>,fill_back>( N );
	auto c2 = create<cw::list<T>,fill_random_sorted>( N );
	auto s1 = create<std::list<T>,fill_back>( N );
	auto s2 = create<std::list<T>,fill_random_sorted>( N );

	c1.merge(c2);
	s1.merge(s2);

	if( !compare( c1, s1 ) || !compare( c2, s2 ) )
		cout << "FAIL: merge" << endl;
	else
		cout << "PASS: merge" << endl;
}

void test_splice() {

	using T = uint16_t;
	size_t N = 20000;

	auto c1 = create<cw::list<T>,fill_back>( N );
	auto c2 = create<cw::list<T>,fill_random_sorted>( N );
	auto s1 = create<std::list<T>,fill_back>( N );
	auto s2 = create<std::list<T>,fill_random_sorted>( N );

	c1.splice( begin(c1), c2 );
	s1.splice( begin(s1), s2 );

	if( !compare( c1, s1 ) || !compare( c2, s2 ) )
		cout << "FAIL: splice" << endl;
	else
		cout << "PASS: splice" << endl;
}

void test_sort() {

	using T = uint16_t;
	size_t N = 32000;

	auto c = create<cw::list<T>,fill_back_random>( N );
	auto s = create<std::list<T>,fill_back_random>( N );

	c.sort();
	s.sort();

	if( !compare( c, s ) )
		cout << "FAIL: sort" << endl;
	else
		cout << "PASS: sort" << endl;
}

int main() {
	test_merge();
	test_splice();
	test_sort();
	cout << "Finished "; cin.get();
}



