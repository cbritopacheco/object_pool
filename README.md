This project is still under development.

The full documentation can be found [here](https://cbritopacheco.github.io/object_pool/).

# object_pool

[![Header-only](https://img.shields.io/badge/header--only-master-ff69b4.svg)](src)
[![Build Status](https://travis-ci.org/carlosb/object_pool.svg?branch=master)](https://travis-ci.org/carlosb/object_pool)
[![Coverage Status](https://coveralls.io/repos/github/carlosb/object_pool/badge.svg?branch=master)](https://coveralls.io/github/carlosb/object_pool?branch=master)
[![Documentation](https://img.shields.io/badge/documentation-master-brightgreen.svg)](https://cbritopacheco.github.io/object_pool/)
[![GitHub license](https://img.shields.io/github/license/carlosb/object_pool.svg)](https://github.com/carlosb/object_pool/blob/master/LICENSE)

An `object_pool` is a container which provides shared access to a collection of instances of one object of type `T`. One only needs to include the header `object_pool.hpp` to be able to use it. Even more, the interface is designed *á la* STL for ease of use! Take the following example for demonstration purposes:

```c++
#include "object_pool.hpp"

using namespace carlosb;

struct expensive_object
{
	expensive_object()
	{
		/* very expensive construction */
	}
};

int main()
{
	object_pool<expensive_object> pool(10); // pool of 10 objects!

	if (auto obj = pool.acquire())
		doSomething(*obj);
	else
		doSomethingElse();
	
	return 0;
}
// when the object goes out of scope, it will get returned to the pool
// when the pool goes out of scope, it will delete all objects
```

# Basic Example

To illustrate a typical call to an access function we provide the following example:

```c++
#include <iostream>
#include "object_pool.hpp"

using namespace std;
using namespace carlosb;

int main()
{
    object_pool<int> pool;  // empty pool
    pool.push(10); // push 10 into pool

    if (auto obj = pool.acquire())
        cout << "We acquired: " << *obj << "\n";
    else
        cout << "We didn't acquire an object" << "\n";

    return 0;
}
```

Output:

```
We acquired: 10
```

# License

The software is licensed under the [Boost Software License 1.0](https://github.com/carlosb/object_pool/blob/master/LICENSE).
