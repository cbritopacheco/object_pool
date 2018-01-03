---
layout: blank
title: object_pool
permalink: /object_pool/
---

# carlosb::object_pool
Defined in header [`object_pool.hpp`](../object_pool.hpp)

```c++
template<
        class T,
        class Allocator     = std::allocator<T>,
        class Mutex         = std::mutex
> class object_pool;
```

`carlosb::object_pool` is a container which manages several shared instances of objects of type `T`.

The object is returned to the pool using a custom deleter and the auxiliary type `acquired_object`. Through [RAII](http://en.cppreference.com/w/cpp/language/raii) we manage the pointer to the object and then return it to its lender.

# Member types

| Member type      | Definition                |
|:-----------------|:--------------------------|
| value_type       | T                         |
| lv_reference     | T&                        |
| rv_reference     | T&&                       |
| const_reference  | const value_type&         |
| acquired_type    | acquired_object<T>        |
| deleter_type     | object_pool::deleter      |
| stack_type       | std::stack<T*>            |
| size_type        | std::size_t               |
| allocator_type   | Allocator                 |
| mutex_type       | Mutex                     |
| lock_guard       | std::lock_guard           |



# Template Parameters

# Member Functions

## Modifiers
- [acquire](#acquire)
- lock_acquire
- try_acquire
- push
- emplace
- resize
- reserve

## Capacity
- size
- capacity
- empty

## Observers
- operator bool


## acquire

```c++
acquired_type acquire()
```

### Parameters
n/a

### Return Value
Acquired object

### Exceptions