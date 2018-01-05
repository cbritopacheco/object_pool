---
title: object_pool
permalink: /class/object_pool/
layout: default
---

# `carlosb::object_pool`
Defined in header [`object_pool.hpp`](/object_pool/header/object_pool)

```cpp
template<
    class T,
    class Allocator     = std::allocator<T>,
    class Mutex         = std::mutex
> class object_pool;
```

`carlosb::object_pool` is a container which manages several shared instances of objects of type `T`.

The object is returned to the pool using a custom deleter and the auxiliary type `object_pool<T>::acquired_object`. Through [RAII](http://en.cppreference.com/w/cpp/language/raii) we manage the pointer to the object and then return it to its lender.

# Template Parameters

| Parameter   | Description                              |
|------------:|------------------------------------------|
|   `T`       | The type of the elements that are stored
| `Allocator` | An allocator that is used to acquire/release memory and to construct/destroy the elements in that memory. The type must meet the requirements of [Allocator](http://en.cppreference.com/w/cpp/concept/Allocator). The behavior is undefined if `Allocator::value_type` is not the same as `T`.          |

# Member types

| Member type        | Definition                |
|:-------------------|:--------------------------|
| `value_type`       | `T                       `|
| `lv_reference`     | `T&                      `|
| `rv_reference`     | `T&&                     `|
| `const_reference`  | `const value_type&       `|
| `acquired_type`    | `object_pool::acquired_object      `|
| `size_type`        | `std::size_t             `|
| `allocator_type`   | `Allocator               `|
| `mutex_type`       | `Mutex                   `|
| `lock_guard`       | `std::lock_guard         `|


# Member Functions

- [(constructor)](#constructor)
- [(destructor)](#destructor)
- [(operator=)](#operator=)
- [get_allocator()](#get_allocator)

## Element Access

- [acquire](#acquire)
- [lock_acquire](#lock_acquire)
- [try_acquire](#try_acquire)

## Modifiers

- [push](#push)
- [emplace](#push)
- [resize](#resize)
- [reserve](#reserve)

## Capacity
- [size](#size)
- [capacity](#capacity)
- [empty](#empty)

## Observers
- [in_use](#in_use)
- [operator bool](#operator-bool)


## acquire

```c++
acquired_type acquire();
```

### Parameters
n/a

### Return Value
The acquired object. You can access it by dereferencing it like this:

```cpp
auto obj = pool.acquire();
doSomething(*obj);
```

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Constant.
