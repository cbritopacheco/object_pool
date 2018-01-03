[![GitHub license](https://img.shields.io/github/license/carlosb/object_pool.svg)](https://github.com/carlosb/object_pool/blob/master/LICENSE)

The full documentation can be found [here](https://carlosb.github.io/object_pool…)https://carlosb.github.io/object_pool….

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
    object_pool<int> pool(5, 10);

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
