#ifndef INCLUDED_CW_LOGARITHMIC_RANGE
#define INCLUDED_CW_LOGARITHMIC_RANGE
#include <cmath>
#include <algorithm>
#include <iterator>

#if _MSC_VER <= 1800
#define noexcept throw()
#endif

namespace cw {

template<typename T>
struct logarithmic_iterator;

template<typename T>
struct logarithmic_types {
	using value_type             = T;
	using size_type              = size_t;
	using difference_type        = std::ptrdiff_t;
	using reference              = T&;
	using pointer                = T*;
	using const_reference        = const T&;
	using const_pointer          = const T*;
	using iterator               = logarithmic_iterator<T>;
	using const_iterator         = logarithmic_iterator<T>;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
};

template<typename T>
struct logarithmic_range : logarithmic_types<T> {
	using this_type = logarithmic_range<T>;
	
	value_type start_value,
	           end_value,
	           min_increment;
	double factor;
	iterator end_it;
	
	logarithmic_range( value_type start,
	                   value_type end,
	                   size_type max_its,
	                   value_type min_incr = value_type() ) :
		factor( compute_factor( start, end, max_its ) ),
		start_value(start),
		end_value(end),
		min_increment(min_incr)
	{
		iterator it = begin();
		while( *it <= end_value )
			++it;
		end_it = it;
	}
	
	static double compute_factor( value_type start, value_type end, size_type max_its ) {
		return pow( double(end)/double(start), 1.0/(max_its-1) );
	}
	
	iterator begin() noexcept {
		return iterator( start_value, factor, min_increment );
	}

	iterator end() noexcept {
		return end_it;
	}

	iterator rbegin() noexcept {
		return reverse_iterator( end() );
	}

	iterator rend() noexcept {
		return reverse_iterator( begin() );
	}

	const_iterator rbegin() const noexcept {
		return const_reverse_iterator( end() );
	}

	const_iterator rend() const noexcept {
		return const_reverse_iterator( begin() );
	}

	const_iterator crbegin() const noexcept {
		return rbegin();
	}

	const_iterator crend() const noexcept {
		return rend();
	}
	
};

template<typename T>
struct logarithmic_iterator : logarithmic_types<T> {
	using this_type = logarithmic_iterator<T>;
	using iterator_category = std::bidirectional_iterator_tag;
	
	value_type value = value_type(),
	           min_increment = value_type();
	double factor = 2.0;
	
	logarithmic_iterator() = default;

	logarithmic_iterator( value_type value,
	                      double factor,
	                      value_type min_incr = value_type() ) :
		value(value),
		min_increment(min_incr),
		factor(factor)
	{}
	
	reference operator*() {
		return value;
	}

	pointer operator->() {
		return &value;
	}
	
	this_type& operator++() {
		value = std::max( value_type( double(value) * factor ), value + min_increment );
		return *this;
	}
	
	this_type& operator--() {
		value = std::min( value_type( double(value) / factor ), value - min_increment );
		return *this;
	}
	
	this_type operator++(int) {
		this_type old = *this;
		++(*this);
		return old;
	}

	this_type operator--(int) {
		this_type old = *this;
		--(*this);
		return old;
	}
	
	bool operator==( const this_type& rhs ) const {
		return value == rhs.value;
	}

	bool operator!=( const this_type& rhs ) const {
		return !( *this == rhs );
	}
	
};

template<typename T>
cw::logarithmic_range<T> log_range( T start, T end, size_t max_iterations, T min_increment = T() ) {
	return cw::logarithmic_range<T>( start, end, max_iterations, min_increment );
}


}

#if _MSC_VER <= 1800
#undef noexcept
#endif

#endif
