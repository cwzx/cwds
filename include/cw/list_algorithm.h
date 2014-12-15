#ifndef INCLUDED_CW_LIST_ALGORITHM
#define INCLUDED_CW_LIST_ALGORITHM
#include <algorithm>
#include "list.h"

namespace cw {

// Algorithms can be implemented directly on the vector of values,
// bypassing the linked structure entirely.

template<typename T,typename U,typename T2,typename BinaryOperation>
T2 accumulate( const cw::list<T,U>& v, T2 init, BinaryOperation op ) {
	return std::accumulate( v.values.begin(), v.values.end(), init, op );
}

template<typename T,typename U,typename T2>
T2 accumulate( const cw::list<T,U>& v, T2 init ) {
	return std::accumulate( v.values.begin(), v.values.end(), init );
}

template<typename T,typename U,typename Pred>
bool all_of( const cw::list<T,U>& v, Pred pred ) {
	return std::all_of( v.values.begin(), v.values.end(), pred );
}

template<typename T,typename U,typename Pred>
bool any_of( const cw::list<T,U>& v, Pred pred ) {
	return std::any_of( v.values.begin(), v.values.end(), pred );
}

template<typename T,typename U,typename Pred>
bool none_of( const cw::list<T,U>& v, Pred pred ) {
	return std::none_of( v.values.begin(), v.values.end(), pred );
}

template<typename T,typename U,typename T2>
typename cw::list<T,U>::difference_type count( const cw::list<T,U>& v, const T2& val ) {
	return std::count( v.values.begin(), v.values.end(), val );
}

template<typename T,typename U,typename Pred>
typename cw::list<T,U>::difference_type count_if( const cw::list<T,U>& v, Pred pred ) {
	return std::count_if( v.values.begin(), v.values.end(), pred );
}

template<typename T,typename U,typename T2>
void fill( const cw::list<T,U>& v, const T2& val ) {
	return std::fill( v.values.begin(), v.values.end(), val );
}

template<typename T,typename U,typename T2>
void replace( const cw::list<T,U>& v, const T2& old_val, const T2& new_val ) {
	return std::replace( v.values.begin(), v.values.end(), old_val, new_val );
}

template<typename T,typename U,typename Pred,typename T2>
void replace_if( const cw::list<T,U>& v, Pred pred, const T2& new_val ) {
	return std::replace_if( v.values.begin(), v.values.end(), pred, new_val );
}

}

namespace std {

template<typename T,typename U,typename T2,typename BinaryOperation>
T2 accumulate( cw::list_const_iterator<T,U> first, cw::list_const_iterator<T,U> last, T2 init, BinaryOperation op ) {
	if( first == first.p->begin() && last == last.p->end() )
		return cw::accumulate( *first.p, init, op );
	
	for( ; first != last; ++first )
		init = op( init, *first );
	return init;
}

template<typename T,typename U,typename T2>
T2 accumulate( cw::list_const_iterator<T,U> first, cw::list_const_iterator<T,U> last, T2 init ) {
	if( first == first.p->begin() && last == last.p->end() )
		return cw::accumulate( *first.p, init );

	for( ; first != last; ++first )
		init += *first;
	return init;
}

/*
// This implements swap by rewiring the links, rather than swapping values.
// This will effectively swap any iterators pointing to the two elements,
// which causes problems in algorithms not expecting it.

template<typename T,typename U>
void iter_swap( cw::list_iterator<T,U>& lhs, cw::list_iterator<T,U>& rhs ) {
	lhs.iter_swap(rhs);
}
*/
	
}


#endif
