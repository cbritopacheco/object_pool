---
title: Included Classes
permalink: /class/
layout: default
---

# Included Classes

To see the full documentation of the interface please follow the links:
- [`object_pool`](/object_pool/class/object_pool/)
- [`acquired_object`](/object_pool/class/acquired_object/)

Below is some discussion about their implementations and requirements.

## About `object_pool`

### On the template parameters
There is only one class that you should use frequently, and another that you might use from time to time. Namely these are [`object_pool`](/object_pool/class/object_pool) and [`acquired_object`](/object_pool/class/acquired_object). The former represents the pool of shared instances and the latter the _acquired_ versions of these instances.

The actual declaration of `object_pool` is this:

```c++
template <
    class T,
    class Allocator         = std::allocator<T>,
    class Mutex             = std::mutex
>
class object_pool;
```
which means that you can select which type of allocator you want to use. As long as it satisfies the requirements of [Allocator](http://en.cppreference.com/w/cpp/concept/Allocator). In particular, we need these methods in order to do the allocations:

- `T* allocate(size_type n)`
- `deallocate(T* p, size_type n)`

For the requirements of the parameter `Mutex`, we ask that it satisfies the requirements of [Lockable](http://en.cppreference.com/w/cpp/concept/Lockable). We give the option to choose these types because more often than not, development teams will want to run their own custom allocators and/or mutexes since they may be optimized to their needs. Unfortunately, we need to specify the interface they follow and so, we follow each concept's specification. If you're not worried about which implementation of `Allocator` or `Mutex` is used, then `object_pool` will use the STL implementations.


### On the implementation

#### On adding objects
Internally, the implementation uses contiguos allocated memory to store the managed objects. This allows the pushing of `r-value` and `l-value` references into the pool and the in-place construction, i.e.

```c++
std::string my_string = "Hello World!";

object_pool<string> pool;
pool.push(my_string); // pushes l-value reference, will copy the string
pool.push(std::string("Hello World")); // pushes r-value reference, will move the string
pool.emplace("Hello World") // constructs a string, will forward the arguments for construction
```
This then leaves the complexity up to the constructors of `T`.

#### On the acquisition of objects

For the acquisition of resources, the pool keeps a stack of pointers `std::stack<T*>`, which point to the individual blocks of memory where all the objects are stored. This means that all we need to do, is to return the `top()` of the stack and `pop()` the pointer. Effectively, we get constant complexity for the acquisition of objects. Hurray! However, there is but one problem. We need to keep track of the pointers after they've left the pool. Now, there may be a lot of different methods to achieve this, but by far what I consider the best is [RAII](http://en.cppreference.com/w/cpp/language/raii). By wrapping around a helper class that takes care of the pointer we can achieve memory safety. In this case, our helper class is `acquired_object`. This class takes care of seeing that the pointer returns safely to the instance of `object_pool`. When one makes a call to `acquire`, it will construct an `acquired_object`. When the `acquired_object` is destroyed, the pointer will return to the pool where it can be safely managed and deleted.

#### On the freeing of memory

Now I know what you're thinking after you've read the last part. When does the memory get freed? What if I've got a main thread and a child thread that both use the pool? What if I've the pool goes out of scope in the declaring thread and I've got acquired objects in other threads? **Will all acquired objects get invalidated?** The answer for this last question is a big **NO**. **The memory of all the stored objects will only be freed until the last remaining `acquired_object` goes out of scoped or gets destroyed**. The pool achieves this by storing a `shared_ptr` to itself. When each `acquired_object` gets constructed, it gets passed two parameters:

- a `shared_ptr` to the pool
- a pointer to the object acquired

This means that the `shared_ptr` that the `acquired_object` has, owns the `object_pool` and so it is obvious that [only the last `shared_ptr` will be the one to delete the pool](http://en.cppreference.com/w/cpp/memory/shared_ptr/~shared_ptr).

## About `acquired_object`

The class `acquired_object`
The class `acquired_object` is actually a member of `object_pool`. This means that you need to prepend `object_pool<T, Allocator, Mutex>::` if you wanted to declare an object. For example, if you wanted to declare an empty `acquired_object` and then access it, you would need to do this:

```c++
using namespace std;
using namespace carlosb;

...

object_pool<string>::acquired_object empty_obj; // no object acquired
object_pool<string> pool(10, "Not so empty anymore!"); // pool of 10 default-constructed objects

empty_obj = pool.acquire();
cout << *empty_obj << "\n";
```

Output:

```c++
Not so empty anymore!
```

This is all good, until you try to access an empty object. Say you had the following program, where you declare an empty acquisition:

```c++
using namespace std;
using namespace carlosb;

...

object_pool<string>::acquired_object empty_obj; // no object acquired
cout << *empty_obj << "\n"; // std::logic_error, you can't access an empty uninitialized object
```

However, this example shows that it is pretty clear there will be an error. If you've read the documentation, you'll know that `acquire()` can't always acquire an object but refunding that it will not throw an exception. Instead, it relies on the fact that you check whether the object is valid through the following dialect:

```c++
if (obj)
    doSomething(*obj);
else
    doSomethingElse();
```
