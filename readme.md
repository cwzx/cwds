Data Structures
===============

A non-standard linked list implementation using contiguous arrays.

The list is fully-featured with the same interface as `std::list` and can be used as a drop-in replacement (up to a few caveats, see below).

The source code is in [`include/cw/list.h`](/include/cw/list.h).

More adventures in data structures may appear here in the future.

Usage
-----

```cpp
#include <iostream>
#include <cw/list.h>

using namespace std;

int main() {
    cw::list<double> values = { 1.0, -3.2, 1.0e-3 };
	values.push_front( 30.0 );
	for( auto x : values )
		cout << x << endl;
}
```

The list takes two template type arguments:

* The value type -- the type of the elements you wish to store in the data structure.
* The index type -- an unsigned integer type large enough to index all the elements.

The choice of index type limits the maximum size of the list.

The default index type is `uint16_t`, giving a max size of 65535.

The following convenience typedefs are provided:

```cpp
template<typename T>
using list8 = list<T,uint8_t>;

template<typename T>
using list16 = list<T,uint16_t>;

template<typename T>
using list32 = list<T,uint32_t>;

template<typename T>
using list64 = list<T,uint64_t>;
```

Interface Differences
---------------------

* There is no custom allocator support (yet).
* `.erase()` invalidates iterators to the erased element and the element stored at the back of the underlying vector.
* `.merge()` does allocation and move.
* `.splice()` does allocation and move.
* `.swap()` invalidates all iterators to both lists.

Benchmark
---------

A benchmark can be found in `benchmark/`.

Project files for Visual Studio 2015 Preview are in `build/`.




