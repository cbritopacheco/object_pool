[![GitHub license](https://img.shields.io/badge/license-%20Boost%20Software%20License%201.0-blue.svg)](https://github.com/carlosb/object_pool/blob/master/LICENSE) [![Documentation](https://img.shields.io/badge/documentation-master-brightgreen.svg)](https://carlosb.github.io/object_pool/class/object_pool/)

The full documentation can be found [here](https://carlosb.github.io/object_pool/index).

# object_pool

An `object_pool` is a container which provides shared access to a collection of instances of one object of type `T`. One only need to include the header `object_pool.hpp` to be able to use the interface. Even more, the interface is designed *á la* STL for ease of use! Take the following example for demonstration purposes:

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

The software is licensed under the [Apache License 2.0](https://choosealicense.com/licenses/apache-2.0/).
