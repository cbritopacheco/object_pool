---
title: index
permalink: /index/
---

[![GitHub license](https://img.shields.io/github/license/carlosb/object_pool.svg)](https://github.com/carlosb/object_pool/blob/master/LICENSE)

# object_pool

An `object_pool` is a some kind of container which provides shared access to a collection of instances of one object of type `T`. One only need to include the header `object_pool.hpp` to be able to use the interface. Even more, the interface is designed *á la* STL for ease of use! Take the following example for demonstration purposes:

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

# Table of Contents
1. [Basic Example](#basic-example)
2. [Thread Safety](#thread-safety)
3. [License](#license)

# Basic Example

## Acquisition of an object
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

This program does the following:

1. Create an object pool containing 5 integers with the default value `10`
2. Acquire an object and check if it is valid
3. Access the object through `operator*()` like this `*obj`

## Failure to acquire an object

In the previous example, we constructed an `object_pool<int>` of 5 integers with the default value `10`. Now, we will construct the same `object_pool<int>` with 5 spaces, but instead we will not pass a default value.

```c++
#include <iostream>
#include "object_pool.hpp"

using namespace std;
using namespace carlosb;

int main()
{
    object_pool<int> pool; 	// empty pool
    pool.reserve(5); 		// reserve 5 spaces, but don't construct any objects

    if (auto obj = pool.acquire())
        cout << "We acquired: " << *obj << "\n";
    else
        cout << "We didn't acquire an object" << "\n";

    return 0;
}
```

Output:

```
We didn't acquire an object
```


# Thread Safety
Through the use of mutexes, **all of the functions in** `object_pool` **are thread safe**. This, however, turns our attention to the **synchronized access of objects** in the pool. That is, what if a thread wants to `acquire()` an object from an empty pool? Will it wait indefinitely? Or will the user have to synchronize manually the acquisitions of objects? To try to answer all these questions and provide flexibility, we chose to implement a method that appeals to most synchronization lingos:

```c++
// Waits until there is a free object in the pool or the time limit is reached.
acquired_type lock_acquire(std::chrono::milliseconds time_limit = std::chrono::nanoseconds::zero());
```

which is different from

```c++
// Attempts to acquire an object instantaneously
acquired_type acquire();
```

The method `lock_acquire()` will wait until there is a free object in the pool. If `time_limit = std::chrono::nanoseconds::zero()` then it will wait indefinitely. In particular, this method must be used with great care since if the condition before is met, it might lead to deadlocks. You will still need to check whether you acquired an object or not, but at least you now have a simple interface to control these parameters.

To see a very basic use of  `lock_acquire()` in action, we present an example in the following section.

### Example

The following program does the following:

1. Creates a pool with only one string
2. Creates two threads: `t1` and `t2`
3. `t2` goes to sleep
4. `t1` acquires the only object
5. `t1` goes to sleep for 5 seconds
6. `t2` tries to acquire the object and it must wait for `t1` to wake up
7. `t1` wakes up
8. `t2` acquires the object

The modification of the string lets us see that the objects are indeed shared across scopes and/or threads.

```c++
#include <thread>
#include "object_pool.hpp"

using namespace std;
using namespace carlosb;

object_pool<string> pool;   // Shared pool of strings
mutex io_mutex;             // Used to control access to std::cout


void worker1()
{
    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 1]: Acquiring objects...\n";
    }

    // acquire object
    auto obj = pool.lock_acquire();

    {
        lock_guard<mutex> lock_cout(io_mutex);
        
        cout
            << "[Worker 1]: I have acquired this from the pool: \'" 
            << *obj
            << "\'\n";
    }

    // modify object
    *obj = "Modified from Worker 1";

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 1]: Sleeping for 5 seconds..." << "\n";
    }

    // sleep for 5 seconds
    this_thread::sleep_for(chrono::seconds(5));

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 1]: Waking up!" << "\n";
    }
}

void worker2()
{
    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 2]: Sleeping for 1 second..." << "\n";
    }

    // sleep for 1 seconds so worker1 gets a hold of the object first
    this_thread::sleep_for(chrono::seconds(1));

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout << "[Worker 2]: Waking up!" << "\n";
        cout << "[Worker 2]: Acquiring objects...\n";
    }

    auto obj = pool.lock_acquire();

    {
        lock_guard<mutex> lock_cout(io_mutex);
        cout
            << "[Worker 2]: I have acquired this from the pool: \'" 
            << *obj
            << "\'\n";
    }
}

int main(int argc, char const *argv[])
{   
    // declare two threads
    std::thread t1(worker1), t2(worker2);

    // push the string into the pool
    pool.push("Hello World!");

    // join the threads
    t1.join();
    t2.join();

    return 0;
}
```

Output:

```
[Worker 1]: Acquiring objects...
[Worker 2]: Sleeping for 1 second...
[Worker 1]: I have acquired this from the pool: 'Hello World!'
[Worker 1]: Sleeping for 5 seconds...
[Worker 2]: Waking up!
[Worker 2]: Acquiring objects...
[Worker 1]: Waking up!
[Worker 2]: I have acquired this from the pool: 'Modified from Worker 1'
```

# License

The software is licensed under the [Apache License 2.0](https://choosealicense.com/licenses/apache-2.0/).
