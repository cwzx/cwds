#ifndef INCLUDED_CW_LIST
#define INCLUDED_CW_LIST
#include <cstdint>
#include <vector>
#include <utility>
#include <exception>
#include <limits>
#include <iterator>

#if _MSC_VER <= 1800
#define noexcept throw()
#endif

namespace cw {

template<typename T,typename U>
struct list;

template<typename T,typename U>
struct list_iterator;

template<typename T,typename U>
struct list_const_iterator;

template<typename T,typename U>
struct list_types_base {
	using value_type             = T;
	using index_type             = U;
	using size_type              = size_t;
	using difference_type        = std::ptrdiff_t;
};

template<typename T,typename U,bool is_const>
struct list_iterator_types;

template<typename T,typename U>
struct list_iterator_types<T,U,false> : list_types_base<T,U> {
	using reference              = T&;
	using pointer                = T*;
	using iterator               = list_iterator<T,U>;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using iterator_category      = std::bidirectional_iterator_tag;
	using list_type              = list<T,U>;
};

template<typename T,typename U>
struct list_iterator_types<T,U,true> : list_types_base<T,U> {
	using reference              = const T&;
	using pointer                = const T*;
	using iterator               = list_const_iterator<T,U>;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using iterator_category      = std::bidirectional_iterator_tag;
	using list_type              = const list<T,U>;
};

template<typename T,typename U,bool is_const>
struct list_iterator_base : list_iterator_types<T,U,is_const> {
	using base_type = list_iterator_base<T,U,is_const>;

	using typename list_iterator_types<T,U,is_const>::value_type;
	using typename list_iterator_types<T,U,is_const>::index_type;
	using typename list_iterator_types<T,U,is_const>::reference;
	using typename list_iterator_types<T,U,is_const>::pointer;
	using typename list_iterator_types<T,U,is_const>::iterator;
	using typename list_iterator_types<T,U,is_const>::list_type;

	list_iterator_base() = default;

	list_iterator_base( list_type* p, index_type index ) : p(p), index(index) {}

	reference operator*() const {
		return p->values[index];
	}

	pointer operator->() const {
		return &p->values[index];
	}

	base_type& operator++() {
		if( index == p->terminator ) {
			index = p->head;
		} else {
			index = p->nodes[index].next;
		}
		return *this;
	}

	base_type& operator--() {
		if( index == p->terminator ) {
			index = p->tail;
		} else {
			index = p->nodes[index].prev;
		}
		return *this;
	}

	base_type operator++(int) {
		auto old = *this;
		++(*this);
		return old;
	}

	base_type operator--(int) {
		auto old = *this;
		--(*this);
		return old;
	}

	bool operator==( const base_type& rhs ) const {
		return (p == rhs.p) && (index == rhs.index);
	}

	bool operator!=( const base_type& rhs ) const {
		return !( *this == rhs );
	}

	friend list<value_type,index_type>;

	list_type* p;
	index_type index;
};

template<typename T,typename U>
struct list_const_iterator : list_iterator_base<T,U,true> {

	using base_type::list_iterator_base;

	list_const_iterator() = default;

	list_const_iterator( const base_type& it ) : list_iterator_base(it) {}

	list_const_iterator( const list_iterator<T,U>& it ) : list_iterator_base(it.p,it.index) {}

	friend list_iterator<T,U>;
};

template<typename T,typename U>
struct list_iterator : list_iterator_base<T,U,false> {

	using base_type::list_iterator_base;

	list_iterator() = default;

	list_iterator( const base_type& it ) : list_iterator_base(it) {}

	explicit list_iterator( const list_const_iterator<T,U>& it ) : list_iterator_base( const_cast<list_type*>(it.p), it.index ) {}

	void iter_swap( iterator& rhs ) {
		if( p != rhs.p )
			return;
		p->swap_nodes( index, rhs.index );
		std::swap( index, rhs.index );
	}

	friend list_const_iterator<T,U>;
};

template<typename T,typename U = uint16_t>
struct list : list_types_base<T,U> {
	using reference              = T&;
	using const_reference        = const T&;
	using pointer                = T*;
	using const_pointer          = const T*;
	using iterator               = list_iterator<T,U>;
	using const_iterator         = list_const_iterator<T,U>;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using list_type              = list<T,U>;

	struct node {
		index_type prev, next;
	};

	static const index_type terminator = index_type(-1); //std::numeric_limits<index_type>::max();

	std::vector<value_type> values;
	std::vector<node> nodes;
	
	index_type head = terminator,
	           tail = terminator;
	
	list() = default;

	list( const std::initializer_list<value_type>& rhs ) {
		assign( rhs );
	}

	explicit list( const std::vector<value_type>& rhs ) {
		assign( rhs );
	}

	explicit list( std::vector<value_type>&& rhs ) {
		assign( std::move( rhs ) );
	}
	
	explicit list( size_type N ) {
		resize(N);
	}

	// Assignment

	list_type& operator=( const std::vector<value_type>& rhs ) {
		size_type N = rhs.size();
		if( N > max_size() ) {
			throw std::exception("cw::list assignment -- vector too big for index_type");
		}
		values = rhs;
		set_default_nodes( N );
		return *this;
	}

	list_type& operator=( std::vector<value_type>&& rhs ) {
		size_type N = rhs.size();
		if( N > max_size() ) {
			throw std::exception("cw::list assignment -- vector too big for index_type");
		}
		values = std::move(rhs);
		set_default_nodes( N );
		return *this;
	}

	list_type& operator=( const std::initializer_list<value_type>& rhs ) {
		size_type N = rhs.size();
		if( N > max_size() ) {
			throw std::exception("cw::list assignment -- initializer_list too big for index_type");
		}
		values = rhs;
		set_default_nodes( N );
		return *this;
	}

	void assign( size_type count, const T& value ) {
		values.assign( count, value );
		set_default_nodes( count );
	}

	template<typename InputIt>
	void assign( InputIt first, InputIt last ) {
		values.assign( first, last );
		set_default_nodes( std::distance(first,last) );
	}

	void assign( const std::initializer_list<T>& rhs ) {
		values.assign( rhs );
		set_default_nodes( rhs.size() );
	}

	// Element Access

	value_type& front() { return values[head]; }

	value_type& back() { return values[tail]; }

	value_type* data() noexcept { return values.data(); }

	const value_type* data() const noexcept { return values.data(); }

	// Capacity

	bool empty() const noexcept { return size() == 0; }

	size_type size() const noexcept { return nodes.size(); }
	
	size_type max_size() const noexcept { return std::numeric_limits<index_type>::max(); }
	
	void reserve( size_type N ) {
		values.reserve(N);
		nodes.reserve(N);
	}

	size_type capacity() const noexcept { return values.capacity(); }

	void shrink_to_fit() {
		values.shrink_to_fit();
		nodes.shrink_to_fit();
	}

	// Modifiers

	void clear() noexcept {
		values.clear();
		nodes.clear();
		head = tail = terminator;
	}

	iterator insert( const_iterator pos, const value_type& x ) {
		values.push_back(x);
		return insert_index_node( pos.index );
	}

	iterator insert( const_iterator pos, value_type&& x ) {
		values.push_back( std::move(x) );
		return insert_index_node( pos.index );
	}

	template<typename... Ts>
	iterator emplace( const_iterator pos, Ts&&... xs ) {
		values.emplace_back( std::forward<Ts>(xs)... );
		insert_index_node( pos.index );
	}

	iterator erase( const_iterator pos ) {
		return erase_index(pos.index);
	}

	iterator erase( const_iterator first, const_iterator last ) {
		while( first != last ) {
			first = erase_index( first.index );
		}
		return iterator(first);
	}

	void push_front( const value_type& x ) {
		values.push_back(x);
		push_front_node();
	}

	void push_back( const value_type& x ) {
		values.push_back(x);
		push_back_node();
	}

	void push_front( value_type&& x ) {
		values.push_back( std::move(x) );
		push_front_node();
	}

	void push_back( value_type&& x ) {
		values.push_back( std::move(x) );
		push_back_node();
	}

	template<typename... Ts>
	void emplace_front( Ts&&... xs ) {
		values.emplace_back( std::forward<Ts>(xs)... );
		size_type num_values = values.size();
		size_type num_nodes = nodes.size();
		while( num_nodes++ < num_values ) {
			push_front_node();
		}
	}

	template<typename... Ts>
	void emplace_back( Ts&&... xs ) {
		values.emplace_back( std::forward<Ts>(xs)... );
		size_type num_values = values.size();
		size_type num_nodes = nodes.size();
		while( num_nodes++ < num_values ) {
			push_back_node();
		}
	}

	void pop_front() {
		erase_index(head);
	}

	void pop_back() {
		erase_index(tail);
	}

	void resize( size_type N ) {
		if( N > max_size() ) {
			throw std::exception("cw::list::resize() -- size too big for index_type");
		}
		size_type current_size = size();
		if( N > current_size ) {
			values.resize( N );
			nodes.resize( N );
			resize_nodes( N );
		} else {
			while( current_size > N ) {
				pop_back();
				--current_size;
			}
		}
	}

	void resize( size_type N, const value_type& x ) {
		if( N > max_size() ) {
			throw std::exception("cw::list::resize() -- size too big for index_type");
		}
		size_type current_size = size();
		if( N > current_size ) {
			values.resize( N, x );
			nodes.resize( N );
			resize_nodes( N );
		} else {
			while( current_size > N ) {
				pop_back();
				--current_size;
			}
		}
	}

	void swap( list& rhs ) {
		values.swap( rhs );
		nodes.swap( rhs );
		std::swap( head, rhs.head );
		std::swap( tail, rhs.tail );
	}

	// Iterators

	iterator begin() noexcept {
		return iterator( this, head );
	}

	iterator end() noexcept {
		return iterator( this, terminator );
	}

	const_iterator begin() const noexcept {
		return const_iterator( this, head );
	}

	const_iterator end() const noexcept {
		return const_iterator( this, terminator );
	}

	const_iterator cbegin() const noexcept {
		return begin();
	}

	const_iterator cend() const noexcept {
		return end();
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

	// Operations

	template<typename Comp>
	void merge( list_type& rhs, Comp comp ) {		
		merge( std::move(rhs), comp );

		// rhs is now empty
		rhs.clear();
	}

	template<typename Comp>
	void merge( list_type&& rhs, Comp comp ) {
		size_type left_size = size();
		size_type right_size = rhs.size();
		size_type sum_size = left_size + right_size;
		if( sum_size > max_size() ) {
			throw std::exception("cw::list merge -- too big for index_type");
		}

		values.reserve( sum_size );
		std::move( std::begin(rhs.values), std::end(rhs.values), std::back_inserter(values) );

		nodes.resize( sum_size );
		memcpy( &nodes[left_size], &rhs.nodes[0], right_size * sizeof(node) );

		// offset the new indexes
		for(size_type i=left_size;i<sum_size;++i) {
			nodes[ i ].prev += index_type(left_size);
			nodes[ i ].next += index_type(left_size);
		}

		// connect the head
		index_type right_head = rhs.head + index_type(left_size);
		nodes[ tail ].next = right_head;
		nodes[ right_head ].prev = tail;

		// connect the tail
		index_type right_tail = rhs.tail + index_type(left_size);
		tail = right_tail;
		nodes[ right_tail ].next = terminator;

		// merge the two parts
		merge_index( head, right_head, tail, comp );
	}

	void merge( list_type& rhs ) {
		merge( rhs, less<>() );
	}

	void merge( list_type&& rhs ) {
		merge( std::move(rhs), less<>() );
	}

	void remove( const T& value ) {
		index_type N = index_type(values.size());
		for(index_type i=0;i<N;++i) {
			if( values[i] == value ) {
				erase_index(i);
				--i; --N;
			}
		}
	}

	template<typename Pred>
	void remove_if( Pred pred ) {
		index_type N = index_type(values.size());
		for(index_type i=0;i<N;++i) {
			if( pred(values[i]) ) {
				erase_index(i);
				--i; --N;
			}
		}
	}

	void reverse() noexcept {
		for( auto&& node : nodes ) {
			std::swap( node.prev, node.next );
		}
		std::swap( head, tail );
	}

	void splice( const_iterator pos, list_type& rhs ) {
		splice( pos, std::move(rhs) );
		rhs.clear();
	}

	void splice( const_iterator pos, list_type&& rhs ) {
		size_type left_size = size();
		size_type right_size = rhs.size();
		size_type sum_size = left_size + right_size;
		if( sum_size > max_size() ) {
			throw std::exception("cw::list splice -- too big for index_type");
		}

		values.reserve( sum_size );
		std::move( std::begin(rhs.values), std::end(rhs.values), std::back_inserter(values) );

		nodes.resize( sum_size );
		memcpy( &nodes[left_size], &rhs.nodes[0], right_size * sizeof(node) );

		index_type right_head = rhs.head + index_type(left_size);
		index_type right_tail = rhs.tail + index_type(left_size);

		splice_index( pos.index, index_type(left_size), index_type(sum_size), right_head, right_tail );
	}

	void splice( const_iterator pos, list_type& rhs, const_iterator it ) {
		splice( pos, std::move(rhs), it );
		rhs.erase( it );
	}

	void splice( const_iterator pos, list_type&& rhs, const_iterator it ) {
		size_type left_size = size();
		if( left_size + 1 > max_size() ) {
			throw std::exception("cw::list splice -- too big for index_type");
		}

		values.emplace_back( std::move(*it) );
		
		index_type prev_index = prev_index(pos.index);
		nodes.push_back( { prev_index, pos.index } );

		if( prev_pos == terminator ) {
			head = left_size;
		} else {
			nodes[ prev_pos ].next = left_size;
		}

		if( pos.index == terminator ) {
			tail = left_size;
		} else {
			nodes[ pos.index ].prev = left_size;
		}
	}

	void splice( const_iterator pos, list_type& rhs, const_iterator first, const_iterator last) {
		splice( pos, std::move(rhs), first, last );
		rhs.erase( first, last );
	}

	void splice( const_iterator pos, list_type&&, const_iterator first, const_iterator last ) {
		size_type left_size = size();
		size_type right_size = std::distance( first, last );
		size_type sum_size = left_size + right_size;
		if( sum_size > max_size() ) {
			throw std::exception("cw::list splice -- too big for index_type");
		}

		reserve( sum_size );
		std::move( first, last, std::back_inserter(*this) );
	}

	// Delete repeated values
	template<typename Comp>
	void unique( Comp comp ) {
		index_type N = index_type(values.size());
		for(index_type i=0;i<N;++i) {
			for(index_type j=i+1;j<N;++j) {
				if( comp( values[i], values[j] ) ) {
					erase_index(j);
					--j; --N;
				}
			}
		}
	}

	void unique() {
		unique( equal_to<>() );
	}

	template<typename Comp>
	void sort( Comp comp ) {
		size_type N = nodes.size();
		if( N < 2 ) return;
		//merge_sort(head,tail,N,comp);
		insertion_sort(head,tail,comp);
		//selection_sort(head,tail,comp);
	}

	void sort() {
		sort( less<>() );
	}

protected:

	// Assignment

	void set_default_nodes( size_type N ) {
		nodes.resize( N );
		if( N == 0 ) return;
		nodes[0].prev = terminator;
		nodes[0].next = 1;
		head = 0;
		tail = index_type(N-1);
		if( N == 1 ) return;
		for(size_type i=1;i<N;++i) {
			nodes[i].prev = index_type(i-1);
			nodes[i].next = index_type(i+1);
		}
		nodes[N-1].next = terminator;
	}

	// Iteration

	index_type prev_index( index_type i ) const {
		if( i == terminator ) return tail;
		return nodes[i].prev;
	}

	index_type next_index( index_type i ) const {
		if( i == terminator ) return head;
		return nodes[i].next;
	}

	index_type prev_index( index_type index, index_type n ) const {
		for(index_type i=0;i<n;++i)
			index = prev_index(index);
		return index;
	}

	index_type next_index( index_type index, index_type n ) const {
		for(index_type i=0;i<n;++i)
			index = next_index(index);
		return index;
	}

	index_type advance_index( index_type index, difference_type n ) const {
		if( n > 0 )
			next_index( index, index_type(n) );
		if( n < 0 )
			prev_index( index, index_type(-n) );
		return index;
	}

	// Element Access

	index_type get_value_index( const T& val ) const {
		return (&val - &values[0]) / sizeof(value_type);
	}

	node& get_value_node( const T& val ) {
		return nodes[ get_value_index(val) ];
	}

	index_type get_pos_index( index_type n ) const {
		index_type half = index_type(nodes.size() / 2);
		if( n < half ) {
			return next_index( head, n );
		} else {
			return prev_index( tail, index_type(nodes.size()-1) - n );
		}
	}

	// Modifiers

	iterator insert_index_node( index_type index ) {
		index_type N = index_type(nodes.size());
		if( index == terminator ) {
			nodes.push_back( {tail, terminator} );
			if( tail == terminator ) {
				head = N;
			} else {
				nodes[ tail ].next = N;
			}
			tail = N;
		} else {
			index_type prev_index = nodes[ index ].prev;
			nodes.push_back( { prev_index, index } );
			nodes[ index ].prev = N;
			if( prev_index == terminator ) {
				head = N;
			} else {
				nodes[ prev_index ].next = N;
			}
		}
		return iterator( this, N );
	}

	iterator erase_index( index_type index ) {

		index_type prev_index = nodes[ index ].prev;
		index_type next_index = nodes[ index ].next;

		if( prev_index == terminator ) {
			head = next_index;
		} else {
			nodes[ prev_index ].next = next_index;
		}
		
		if( next_index == terminator ) {
			tail = prev_index;
		} else {
			nodes[ next_index ].prev = prev_index;
		}

		index_type last_index = index_type(values.size() - 1);

		// move the last element to the erased index
		if( index < last_index ) {
			index_type last_prev = nodes[ last_index ].prev;
			index_type last_next = nodes[ last_index ].next;

			values[index] = std::move( values.back() );
			nodes[index] = std::move( nodes.back() );
		
			if( last_prev == terminator ) {
				head = index;
			} else {
				nodes[ last_prev ].next = index;
			}

			if( last_next == terminator ) {
				tail = index;
			} else {
				nodes[ last_next ].prev = index;
			}
		}
		values.pop_back();
		nodes.pop_back();
		return iterator( this, next_index );
	}

	void push_front_node() {
		index_type N = index_type(nodes.size());
		nodes.push_back( { terminator, head } );
		if( head == terminator ) {
			tail = N;
		} else {
			nodes[head].prev = N;
		}
		head = N;
	}

	void push_back_node() {
		index_type N = index_type(nodes.size());
		nodes.push_back( { tail, terminator } );
		if( tail == terminator ) {
			head = N;
		} else {
			nodes[tail].next = N;
		}
		tail = N;
	}

	void resize_nodes( size_type N ) {
		
		size_type current_size = size();
		if( current_size <= N ) return;
		
		// modify previous tail and add first new node
		if( tail == terminator ) {
			head = 0;
			nodes[0].prev = terminator;
			nodes[0].next = 1;
		} else {
			nodes[tail].next = index_type(current_size);
			nodes[current_size].prev = tail;
			if( N > current_size + 1 ) {
				nodes[current_size].next = index_type(current_size + 1);
			} else {
				nodes[current_size].next = terminator;
			}
		}

		// add more new nodes
		for( size_type i = current_size + 1; i < N-1; ++i ) {
			nodes[i].prev = index_type(i - 1);
			nodes[i].next = index_type(i + 1);
		}

		// add the last node and update the tail
		if( N > current_size + 1 ) {
			tail = index_type(N - 1);
			nodes[tail].prev = tail - 1;
			nodes[tail].next = terminator;
		}
	}

	void swap_nodes( index_type left, index_type right ) {

		// nothing to do
		if( left == right ) return;

		// can't swap with the terminator
		if( left == terminator || right == terminator ) return;

		index_type left_prev = nodes[ left ].prev;
		index_type left_next = nodes[ left ].next;
		index_type right_prev = nodes[ right ].prev;
		index_type right_next = nodes[ right ].next;

		// check for adjacency left -> right
		if( right_prev == left ) {
			nodes[ left ].prev = left_next;
			nodes[ left ].next = right_next;
			nodes[ right ].prev = left_prev;
			nodes[ right ].next = right_prev;

			if( left_prev == terminator ) {
				head = right;
			} else {
				nodes[ left_prev ].next = right;
			}

			if( right_next == terminator ) {
				tail = left;
			} else {
				nodes[ right_next ].prev = left;
			}
			return;
		}

		// check for adjacency right -> left
		if( right_next == left ) {
			nodes[ left ].prev = right_prev;
			nodes[ left ].next = left_prev;
			nodes[ right ].prev = right_next;
			nodes[ right ].next = left_next;

			if( left_next == terminator ) {
				tail = right;
			} else {
				nodes[ left_next ].prev = right;
			}

			if( right_prev == terminator ) {
				head = left;
			} else {
				nodes[ right_prev ].next = left;
			}
			return;
		}

		// non-adjacent

		nodes[ left ].prev = right_prev;
		nodes[ left ].next = right_next;
		nodes[ right ].prev = left_prev;
		nodes[ right ].next = left_next;

		if( left_prev == terminator ) {
			head = right;
		} else {
			nodes[ left_prev ].next = right;
		}
		
		if( left_next == terminator ) {
			tail = right;
		} else {
			nodes[ left_next ].prev = right;
		}

		if( right_prev == terminator ) {
			head = left;
		} else {
			nodes[ right_prev ].next = left;
		}

		if( right_next == terminator ) {
			tail = left;
		} else {
			nodes[ right_next ].prev = left;
		}
	}

	// Operations

	index_type count( index_type first, index_type last ) {
		if( first == head && last == tail )
			return index_type(nodes.size());
		if( first == terminator || last == terminator )
			return 0;
		index_type N = 1;
		index_type index = first;
		while( index != last ) {
			if( index == terminator )
				return 0;
			index = next_index(index);
			++N;
		}
		return N;
	}

	// produces sorted [first,last] from sorted [first,mid) and sorted [mid,last]
	template<typename Comp>
	void merge_index( index_type first, index_type mid, index_type last, Comp comp ) {
		index_type last_next = next_index(last);
		for( index_type i = first; i != mid; i = next_index(i) ) {
			for( index_type j = mid; j != last_next; j = next_index(j) ) {
				if( comp( values[j], values[i] ) ) {
					swap_nodes( i, j );
					if( mid == j )
						mid = i;
					std::swap( i, j );
				} else {
					break;
				}
			}
		}
		for( index_type i = mid; i != last_next; i = next_index(i) ) {
			for( index_type j = next_index(i); j != last_next; j = next_index(j) ) {
				if( comp( values[j], values[i] ) ) {
					swap_nodes( i, j );
					std::swap( i, j );
				} else {
					break;
				}
			}
		}
	}

	// splice [right_head,right_tail] into [head,right_head) at index
	void splice_index( index_type index, index_type left_size, index_type sum_size, index_type right_head, index_type right_tail ) {
		index_type prev_pos = prev_index(index);
		
		// offset the new indexes
		for(index_type i=left_size;i<sum_size;++i) {
			nodes[ i ].prev += left_size;
			nodes[ i ].next += left_size;
		}
		
		// connect the head
		if( prev_pos == terminator ) {
			head = right_head;
		} else {
			nodes[ prev_pos ].next = right_head;
		}
		nodes[ right_head ].prev = prev_pos;

		// connect the tail
		if( index == terminator ) {
			tail = right_tail;
		} else {
			nodes[ index ].prev = right_tail;
		}
		nodes[ right_tail ].next = index;
	}

	// insertion sort -- O(N^2) compares/swaps, adaptive, [first,last]
	template<typename Comp>
	void insertion_sort( index_type first, index_type last, Comp comp ) {
		index_type first_prev = prev_index(first);
		for( index_type i = first; i != last; i = next_index(i) ) {
			for( index_type j = i; j != first_prev; j = prev_index(j) ) {
				index_type j_next = next_index(j);
				if( !comp( values[j_next], values[j] ) ) break;
				swap_nodes( j, j_next );
				if( i == j ) i = j_next;
				else if( i == j_next ) i = j;
				if( last == j_next ) last = j;
				j = j_next;
			}
		}
	}

	// selection sort -- O(N^2) compares, O(N) swaps, non-adaptive, [first,last]
	template<typename Comp>
	void selection_sort( index_type first, index_type last, Comp comp ) {
		index_type last_next = next_index(last);
		for( index_type i = first; i != last_next; i = next_index(i) ) {
			index_type min_index = i;
			for( index_type j = next_index(i); j != last_next; j = next_index(j) ) {
				if( comp( values[j], values[min_index] ) ) {
					min_index = j;
				}
			}
			if( min_index != i ) {
				swap_nodes( i, min_index );
				i = min_index;
			}
		}
	}

	// merge sort, [first,last] -- fix me
	template<typename Comp>
	void merge_sort( index_type first, index_type last, size_type N, Comp comp ) {
		if( N < 2 ) return;

		if( N * sizeof(node) <= 64 ) {
			insertion_sort(first,last,comp);
			return;
		}

		index_type first_pos = index_type( count( head, first ) - 1 );
		index_type half_size = index_type((N-1) / 2);
		index_type mid = next_index( first, half_size );
		index_type mid_next = next_index( mid );

		merge_sort( first, mid, half_size+1, comp );
		merge_sort( mid_next, last, N-(half_size+1), comp );

		// sorting invalidates indices. recompute them.
		first = get_pos_index( first_pos );
		mid_next = next_index( first, half_size + 1 );
		last = next_index( mid_next, index_type(N - 2) - half_size );
		
		merge_index( first, mid_next, last, comp );
	}

	/*
	// shell sort
	template<typename Comp>
	void shell_sort( index_type first, index_type last, Comp comp ) {
		size_type N = count(first,last);
		size_type h = 1;
		while( h < N )
			h = 3*h + 1;
		while( h > 0 ) {
			h = h / 3;
			index_type h_index = get_index(index_type(h));
			index_type h_next = next_index(h_index);
			for( index_type k = first; k != h_next; k = next_index(k) )
				insertion_sort( k, index_type(h), last, comp )
		}

	}
	*/
};

template<typename T>
using list8 = list<T,uint8_t>;

template<typename T>
using list16 = list<T,uint16_t>;

template<typename T>
using list32 = list<T,uint32_t>;

template<typename T>
using list64 = list<T,uint64_t>;

// Operators

template<typename T, typename U>
bool operator==( const cw::list<T,U>& lhs, const cw::list<T,U>& rhs ) {
	return lhs.size() == rhs.size() && std::equal( lhs.begin(), lhs.end(), rhs.begin() );
}

template<typename T, typename U>
bool operator!=( const cw::list<T,U>& lhs, const cw::list<T,U>& rhs ) {
	return !(lhs == rhs);
}

template<typename T, typename U>
bool operator<( const cw::list<T,U>& lhs, const cw::list<T,U>& rhs ) {
	return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
}

template<typename T, typename U>
bool operator>( const cw::list<T,U>& lhs, const cw::list<T,U>& rhs ) {
	return rhs < lhs;
}

template<typename T, typename U>
bool operator<=( const cw::list<T,U>& lhs, const cw::list<T,U>& rhs ) {
	return !(rhs < lhs);
}

template<typename T, typename U>
bool operator>=( const cw::list<T,U>& lhs, const cw::list<T,U>& rhs ) {
	return !(lhs < rhs);
}


}

namespace std {

template<typename T, typename U>
void swap( cw::list<T,U>& lhs, cw::list<T,U>& rhs ) {
	lhs.swap(rhs);
}

}

#if _MSC_VER <= 1800
#undef noexcept
#endif

#endif
