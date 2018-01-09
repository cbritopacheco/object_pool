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

The object is returned to the pool using a custom deleter and the auxiliary type `acquired_object`. Through [RAII](http://en.cppreference.com/w/cpp/language/raii) we manage the pointer to the object and then return it to its lender.

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
- [acquire_wait](#acquire_wait)

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
- [operator bool](#operator-bool)

## acquire

```c++
acquired_type acquire();
```
Acquires object from the pool.

### Parameters
n/a

### Return Value
The acquired object.

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Constant.

### Example

```cpp
auto obj = pool.acquire();
doSomething(*obj);
```

## acquire_wait

```c++
acquired_type acquire_wait(std::chrono::milliseconds time_limit = std::chrono::milliseconds::zero())
```
Acquires an object from the pool.

If no object is available, it waits until `time_limit` milliseconds have passed. If still no objects are available, it returns an empty object.

If `time_limit` is equal to [`std::chrono::milliseconds::zero()`](http://en.cppreference.com/w/cpp/chrono/duration/zero) it will wait indefinitely for an object.

### Parameters

| parameter    | type                             | default value | direction |
|:------------:|----------------------------------|:-------------:|:---------:|
|   `time_limit`   | [`std::chrono::milliseconds	`](http://en.cppreference.com/w/cpp/chrono/duration)   |      [`std::chrono::milliseconds::zero()`](http://en.cppreference.com/w/cpp/chrono/duration/zero)      |   input   |

### Return Value
The acquired object.

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Constant.

## push

```c++
void push(const_reference value);
void push(rv_reference value);
```

Adds a new object into the pool by either copying it or moving it.

If no more space is left, it will reallocate memory for the pool.

### Parameters

| parameter | type                           | default value | direction |
|:---------:|--------------------------------|:-------------:|:---------:|
|   value   | [const_reference](#member-types)   |      n/a      |   input   |
|   value   | [rv_reference](#member-types)   |      n/a      |   input   |


### Return Value
n/a

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Amortized Constant.

## emplace

```c++11
template <class... Args>
void emplace(Args&&... args)
```
Adds a new object into the pool by constructing it.


### Parameters

| parameter | type                           | default value | direction |
|:---------:|--------------------------------|:-------------:|:---------:|
|   args   | parameter list   |      n/a      |   input   |


### Return Value
n/a

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Amortized constant.

## resize

```c++
void resize(size_type count);
void resize(size_type count, const value_type& value);
```

Resizes the container to contain count elements.

If the current size is greater than count, the container is reduced to its first count elements.

If the current size is less than count,

1. additional default-inserted elements are appended
2. additional copies of value are appended

### Parameters

| parameter | type                           | default value | direction |
|:---------:|--------------------------------|:-------------:|:---------:|
|   count   | [size_type](#member-types)   |      n/a      |   input   |
|   value   | [value_type](#member-types) |      n/a      |   input   |


### Return Value
n/a

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Linear in the difference between the current size and count. Additional complexity is possible due to reallocation if the capacity is less than count.

## resize

```c++
void reserve(size_type new_cap);
```

Reserves storage.

Increase the capacity of the vector to a value that's greater or equal to `new_cap`.

If `new_cap` is greater than the current [`capacity()`](#capacity), new storage is otherwise the method does nothing.

### Parameters

| parameter | type                           | default value | direction |
|:---------:|--------------------------------|:-------------:|:---------:|
|   count   | [size_type](#member-types)   |      n/a      |   input   |
|   value   | [value_type](#member-types) |      n/a      |   input   |


### Return Value
n/a

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
At most linear in the [`size()`](#size) of the container.

## size

```c++
size_type size() const;
```

Returns the number of **free** elements in the pool.

### Parameters

n/a


### Return Value
Number of free elements in the pool.

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Constant.

## capacity

```c++
size_type empty() const;
```

Returns the number of elements that can be held in currently allocated storage.

### Parameters

n/a


### Return Value
Number of elements that can be held.

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Constant.


## empty

```c++
bool empty() const;
```

Checks if the container has no **free** elements. i.e. whether `size() == 0`.

### Parameters

n/a

### Return Value
`true` if no free elements are found, `false` otherwise

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Constant.

## operator bool

```c++
operator bool();
```

Checks if there are free elements in the pool.

### Parameters

n/a

### Return Value
`true` if free elements are found, `false` otherwise

### Exceptions
No exceptions are thrown directly by this method.

### Complexity
Constant.
