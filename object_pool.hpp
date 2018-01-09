/*
 *  Copyright 2017 Carlos David Brito Pacheco
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef CARLOSB_OBJECT_POOL_HPP
#define CARLOSB_OBJECT_POOL_HPP

#include <memory>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <cassert>
#include <chrono>


namespace carlosb
{
    /**
     * @brief      Class for object pool.
     *
     * @tparam     T          Type which will be managed by the `object_pool`.
     * @tparam     Allocator  Allocator type. Must satisfy BasicAllocator requirement.
     * @tparam     Mutex      Mutex type. Must satisfy Lockable requirement.
     */
    template <
        class _Tp,
        class _Allocator         = std::allocator<_Tp>,
        class _Mutex             = std::mutex
    >
    class object_pool;

    /**
     * Contains types needed by the implementation.
     */
    namespace detail
    {   
        template <class...>
        using void_t = void;

        struct none_helper
        {};
    }

    typedef int detail::none_helper::*none_t;
    none_t const none = (static_cast<none_t>(0)) ;

    /**
     * Contains meta-functions to determine if an object of type `T` satisfies
     * the requirements of some concept. Currently implemented:
     * - `is_lockable<_Tp>`
     * - `is_allocator<_Tp>`
     */
    namespace type_traits
    {
        template <class _Tp, class = void>
        struct is_lockable : public std::false_type
        {};

        template <class _Tp>
        struct is_lockable<_Tp, detail::void_t<
            decltype(std::declval<_Tp>().lock()),
            decltype(std::declval<_Tp>().unlock()),
            decltype(std::declval<_Tp>().try_lock()),
            std::is_same<decltype(std::declval<_Tp>().try_lock()), bool>
        >> : std::true_type
        {};

        template <class _Tp, class = void>
        struct is_allocator : public std::false_type
        {};

        template <class _Tp>
        struct is_allocator<_Tp, detail::void_t<
            typename _Tp::value_type,
            typename _Tp::size_type,
            decltype(std::declval<_Tp>().allocate(std::declval<typename _Tp::size_type>())),
            typename std::is_same<
                decltype(std::declval<_Tp>().allocate(std::declval<typename _Tp::size_type>())),
                typename _Tp::value_type*
            >::type,
            decltype(std::declval<_Tp>().deallocate(std::declval<typename _Tp::value_type*>(), std::declval<typename _Tp::size_type>()))
        >> : std::true_type
        {};
    }

    template <
        class _Tp,
        class _Allocator,
        class _Mutex
    >
    class object_pool
    {
        static_assert(type_traits::is_allocator<_Allocator>::value,
                      "Allocator template parameter must satisfy the BasicAllocator requirement.");
        static_assert(type_traits::is_lockable<_Mutex>::value,
                      "Mutex template parameter must satisfy the Lockable requirement.");
    private:
        /**
         * @brief      Contains the actual implementation of `object_pool`.
         */
        class impl;

        /**
         * @brief      The `deleter` class returns lent objects to the pool.
         */
        class deleter;
    public:
        class acquired_object;

        using value_type        = _Tp;                        ///< Type managed by the pool.
        using lv_reference      = _Tp&;                       ///< l-value reference.
        using rv_reference      = _Tp&&;                      ///< r-value reference.
        using const_reference   = const _Tp&;                 ///< const l-value reference.

        using acquired_type     = acquired_object;          ///< Type of acquired objects.
        using deleter_type      = deleter;                  ///< Custom deleter of pool. It returns objects back to the pool.
        using stack_type        = std::stack<_Tp*>;           ///< Stack of pointers to the managed objects. Only contains unused objects.

        using size_type         = std::size_t;              ///< Size type used.

        using allocator_type    = _Allocator;                ///< Type of allocator.
        using mutex_type        = _Mutex;                    ///< Type of mutex.
        using lock_guard        = std::lock_guard<_Mutex>;   ///< Type of lock guard.

        /**
         * @brief      Constructs an empty pool.
         */
        object_pool()
            : object_pool(_Allocator())
        {}

        /**
         * @brief      Constructs an empty pool with the given allocator.
         *
         * @param[in]  alloc  The allocator.
         */
        explicit
        object_pool(const _Allocator& alloc)
            :   m_pool(std::make_shared<impl>(alloc))
        {}

        /**
         * @brief      Constructs a pool with `count` copies of `value`.
         *
         * @param[in]  count  Number of elements.
         * @param[in]  value  Value to be copied.
         * @param[in]  alloc  Allocator to be used.
         */
        object_pool(size_type count, const _Tp& value, const _Allocator& alloc = _Allocator())
            :   m_pool(std::make_shared<impl>(count, value, alloc))
        {}

        /**
         * @brief      Constructs the container with count default-inserted instances of T. No copies are made.
         *
         * @param[in]  count  Number of default-inserted elements.
         * @param[in]  alloc  Allocator to be used.
         */
        explicit
        object_pool(size_type count, const _Allocator& alloc = _Allocator())
            :   m_pool(std::make_shared<impl>(count, alloc))
        {}

        object_pool(const object_pool& other)
            :   m_pool(std::make_shared<impl>(*other.m_pool))
        {}
        
        explicit
        object_pool(const object_pool& other, const _Allocator& alloc)
            :   m_pool(std::make_shared<impl>(*other.m_pool, alloc))
        {}

        object_pool(object_pool&& other, const _Allocator& alloc)
            :    m_pool(std::make_shared<impl>(std::move(*other.m_pool), alloc))
        {}

        object_pool(object_pool&& other) noexcept
            :    m_pool(std::make_shared<impl>(std::move(*other.m_pool)))
        {}

        /**
         * @brief      Acquires an object from the pool. 
         * 
         * @return     Acquired object.
         * 
         * Complexity
         * ----------
         * Constant.
         */
        acquired_type acquire()
        {
            return m_pool->acquire();
        }

        /**
         * @brief      Waits until an object from the pool has been acquired or the `time_limit`
         * has ran out.
         *
         * @return     Acquired object.
         * 
         * Complexity
         * ----------
         * Constant.
         * 
         * This method will lock until an object is available.
         * One may achieve this manually by either:
         * - pushing a new object to the pool
         * - emplacing a new object into the pool
         * - freeing other objects
         */
        acquired_type acquire_wait(std::chrono::milliseconds time_limit = std::chrono::milliseconds::zero())
        {
            return m_pool->acquire_wait(time_limit);
        }

        /**
         * @brief      Pushes an object to the pool by copying it.
         *
         * @param[in]  value  Object to be copied into the pool.
         * 
         * Complexity
         * ----------
         * Amortized constant.
         */
        void push(const_reference value)
        {
            m_pool->push(value);
        }

        /**
         * @brief      Pushes an object to the pool by moving it.
         * 
         * @param[in]  value  Object to be moved into the pool.
         * 
         * Complexity
         * ----------
         * Amortized constant.
         */
        void push(rv_reference value)
        {
            m_pool->push(std::move(value));
        }

        /**
         * @brief      Pushes an object to the pool by constructing it.
         *
         * @param[in]  args       Arguments to be passed to the constructor of the object.
         * 
         * Complexity
         * ----------
         * Amortized constant.
         */
        template <class... Args>
        void emplace(Args&&... args)
        {
            m_pool->emplace(std::forward<Args>(args)...);
        }

        /**
         * @brief      Resizes the pool to contain count elements.
         * 
         * @param[in]  count  Number of elements.
         * 
         * If the current size is greater than count, the container is reduced to its first count elements.
         * If the current size is less than count, additional default-constructed elements are appended.
         *
         * Complexity
         * ----------
         * Linear in the difference between the current number of elements and count.
         */
        void resize(size_type count)
        {
            m_pool->resize(count);
        }

        /**
         * @brief      Resizes the pool to contain count elements.
         *
         * If the current size is greater than count, the container is reduced to its first count elements.
         * If the current size is less than count, additional copies of value are appended.
         * 
         * Complexity
         * ----------
         * Linear in the difference between the current number of elements and count.
         * 
         * @param[in]  count  Number of elements.
         * @param[in]  value  Value to be copied.
         */
        void resize(size_type count, const value_type& value)
        {
            m_pool->resize(count, value);
        }

        /**
         * @brief      Reserves storage.
         * 
         * @param[in]  new_cap  New capacity of the pool
         * 
         * Increase the capacity of the vector to a value that's greater or equal to new_cap.
         * 
         * If new_cap is greater than the current capacity(), new storage is allocated, 
         * otherwise the method does nothing.
         * 
         * Complexity
         * ----------
         * At most linear in the managed_count() of the pool.
         */
        void reserve(size_type new_cap)
        {
            m_pool->reserve(new_cap);
        }

        /**
         * @brief      Returns the number of **free** elements in the pool.
         *
         * @return     The number of **free** elements in the pool.
         * 
         * Complexity
         * ----------
         * Constant.
         */
        size_type size() const
        {
            return m_pool->size();
        }

        /**
         * @brief      Returns the number of elements that can be held in currently allocated storage
         * 
         * @return     Capacity of the currently allocated storage.
         * 
         * Complexity
         * ----------
         * Constant.
         */
        size_type capacity() const
        {
            return m_pool->capacity();
        }

        /**
         * @brief      Checks if the container has no **free** elements. i.e. whether size() == 0.
         * 
         * @return     `true` if no free elements are found, `false` otherwise
         * 
         * Complexity
         * ----------
         * Constant.
         */
        bool empty() const
        {
            return m_pool->empty();
        }

        /**
         * @brief      Checks if there are free elements in the pool.
         * 
         * @return     `true` if free elements are found, `false` otherwise
         * 
         * Complexity
         * ----------
         * Constant.
         */
        operator bool()
        {
            return m_pool->operator bool();
        }

        /**
         * @brief      Checks if the pool is still being used, i.e. not all
         * elements have returned to the pool
         *
         * @return     `true` if the pool still has used elements, `false` otherwise
         */
        bool in_use() const
        {
            return m_pool->in_use();
        }
    private:
        std::shared_ptr<impl>       m_pool;                ///< Pointer to implementation of pool.
    };

    // --- impl IMPLEMENTATION --------------------------------------
    template <
        class _Tp,
        class _Allocator,
        class _Mutex
    >
    class object_pool<_Tp, _Allocator, _Mutex>::impl : public std::enable_shared_from_this<impl>
    {
    public:
        using deleter_type = deleter_type;

        explicit
        impl(const _Allocator& alloc)
            :   m_size(0),
                m_capacity(4),
                m_allocator(alloc)
        {
            m_pool = m_allocator.allocate(m_capacity);
        }

        impl(size_type count, const _Tp& value, const _Allocator& alloc = _Allocator())
            :   m_size(count),
                m_capacity(count),
                m_allocator(alloc)
        {   
            static_assert(std::is_copy_constructible<_Tp>::value,
                          "T must be CopyConstructible to use this constructor.");

            m_pool = m_allocator.allocate(m_capacity);
            for (size_type i = 0; i < m_size; ++i)
                ::new((void *) (m_pool + i)) _Tp(value);

            for (size_type i = 0; i < m_size; ++i)
                m_free.push(m_pool + i);
        }

        explicit
        impl(size_type count, const _Allocator& alloc = _Allocator())
            :   m_size(count),
                m_capacity(count),
                m_allocator(alloc)
        {
            m_pool = m_allocator.allocate(m_capacity);
            for (size_type i = 0; i < m_size; ++i)
                ::new((void *) (m_pool + i)) _Tp();

            for (size_type i = 0; i < m_size; ++i)
                m_free.push(m_pool + i);
        }

        explicit
        impl(const impl& other)
            :   m_size(other.m_size),
                m_capacity(other.m_capacity),
                m_allocator(other.m_allocator)
        {
            lock_guard other_lock(other.m_mutex);
            if (other.m_size - other.m_free.size() > 0)
                throw std::invalid_argument("You may only copy construct an object_pool when in_use() == false.");

            m_pool = m_allocator.allocate(m_capacity);
            for (size_type i = 0; i < m_size; ++i)
                ::new((void *) (m_pool + i)) _Tp(other.m_pool[i]);



            for (size_type i = 0; i < m_size; ++i)
                m_free.push(m_pool + i);
        }

        explicit
        impl(const impl& other, const _Allocator& alloc)
            :   m_size(other.m_size),
                m_capacity(other.m_capacity),
                m_allocator(alloc)
        {
            lock_guard other_lock(other.m_mutex);
            if (other.m_size - other.m_free.size() > 0)
                throw std::invalid_argument("You may only copy construct an object_pool when in_use() == false.");

            m_pool = m_allocator.allocate(m_capacity);
            for (size_type i = 0; i < m_size; ++i)
                ::new((void *) (m_pool + i)) _Tp(other.m_pool[i]);

            for (size_type i = 0; i < m_size; ++i)
                m_free.push(m_pool + i);
        }

        explicit
        impl(impl&& other, const _Allocator& alloc)
            :   m_size(other.m_size),
                m_capacity(other.m_capacity),
                m_allocator(alloc)
        {
            if (alloc == other.get_allocator())
            {
                m_pool = other.m_pool;
                m_free = std::move(other.m_free);
                other.m_pool = nullptr;
            }
            else
            {
                m_pool = m_allocator.allocate(m_capacity);
                for (size_type i = 0; i < m_size; ++i)
                    ::new((void *) (m_pool + i)) _Tp(std::move(other.m_pool[i]));
                for (size_type i = 0; i < m_size; ++i)
                    m_free.push(m_pool + i);

                while (!m_free.empty())
                    other.m_free.pop();
                other.m_pool = nullptr;
            }
        }

        explicit
        impl(impl&& other) noexcept
            :   m_size(other.m_size),
                m_capacity(other.m_capacity),
                m_allocator(std::move(other.m_allocator)),
                m_free(std::move(other.m_free)),
                m_pool(other.m_pool)
        {
            other.m_pool = nullptr;
        }

        impl& operator=(const impl& other)
        {
            lock_guard other_lock(other.m_mutex);
            if (other.m_size - other.m_free.size() > 0)
                throw std::invalid_argument("You may only copy assign an object_pool when in_use() == false.");
            
            m_allocator.deallocate(m_pool);
            while (!m_free.empty())
                m_free.pop();
            
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_allocator = other.m_allocator;

            m_pool = m_allocator.allocate(m_capacity);
            for (size_type i = 0; i < m_size; ++i)
                m_pool[i] = other.m_pool[i];

            for (size_type i = 0; i < m_size; ++i)
                m_free.push(m_pool + i);

            return *this;
        }

        impl& operator=(impl&& other)
        {
            lock_guard other_lock(other.m_mutex);

            m_allocator.deallocate(m_pool);
            while (!m_free.empty())
                m_free.pop();
            
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_allocator = other.m_allocator;

            m_pool = m_allocator.allocate(m_capacity);
            for (size_type i = 0; i < m_size; ++i)
                m_pool[i] = std::move(other.m_pool[i]);

            for (size_type i = 0; i < m_size; ++i)
                m_free.push(m_pool + i);

            return *this;
        }

        ~impl()
        {
            // The destructor is  called until the last
            // shared_ptr to *this is destroyed

            lock_guard lock_pool(m_mutex);
            m_allocator.deallocate(m_pool, m_capacity);
        }

        void reserve(size_type new_cap)
        {
            lock_guard lock_pool(m_mutex);
            m_allocator.deallocate(m_pool, m_capacity);
            m_pool = m_allocator.allocate(new_cap);
            m_capacity = new_cap;
        }

        acquired_type acquire()
        {   
            lock_guard lock_pool(m_mutex);
            if (m_free.empty())
                return acquired_object(none);;
            acquired_object obj(m_free.top(), impl::shared_from_this());
            m_free.pop();
            return std::move(obj);
        }

        acquired_type acquire_wait(std::chrono::milliseconds time_limit)
        {   
            // Lock acquisition of resources
            std::unique_lock<mutex_type> acq_lock(m_acq_mutex);
            
            // To wait, or not to wait? That is the question
            if (time_limit == std::chrono::milliseconds::zero())
            {
                // Wait indefinitely
                m_objects_availabe.wait(acq_lock, time_limit, [this] (void) { return !this->empty(); });
            }
            else
            {
                // Wait until time limit is exceeded
                if (!m_objects_availabe.wait_for(acq_lock, time_limit, [this] (void) { return !this->empty(); }))
                    return acquired_object(none);
            }
            
            // Object to return
            acquired_object obj;
            
            // Critical section
            {
                // Lock pool
                lock_guard lock_pool(m_mutex);

                // Acquire object
                obj = acquired_object(m_free.top(), impl::shared_from_this());
                
                // Remove object from the stack
                m_free.pop();
            }

            // Unlock acquisition of resources
            acq_lock.unlock();

            // Notifiy one thread we have unlocked
            m_objects_availabe.notify_one();
            
            return std::move(obj);
        }

        void push(const_reference value)
        {
            {
                lock_guard lock_pool(m_mutex);
                if (m_size == m_capacity)
                    this->reallocate(m_capacity * 2);
                ::new((void *) (m_pool + m_size)) _Tp(value);
                m_free.push(m_pool + m_size);
                ++m_size;
            }
            m_objects_availabe.notify_one();
        }

        void push(rv_reference value)
        {
            {
                lock_guard lock_pool(m_mutex);
                if (m_size == m_capacity)
                    this->reallocate(m_capacity * 2);
                ::new((void *) (m_pool + m_size)) _Tp(std::move(value));
                m_free.push(m_pool + m_size);
                ++m_size;
            }
            m_objects_availabe.notify_one();
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            {
                lock_guard lock_pool(m_mutex);
                if (m_size == m_capacity)
                    this->reallocate(m_capacity * 2);
                ::new((void *) (m_pool + m_size)) _Tp(std::forward<Args>(args)...);
                m_free.push(m_pool + m_size); 
                ++m_size;
            }
            m_objects_availabe.notify_one();
        }

        void resize(size_type count)
        {
            lock_guard lock_pool(m_mutex);
            if (count > m_size)
            {
                if (count > m_capacity)
                    this->reallocate(count);

                for (size_type i = m_size; i < count; ++i)
                    ::new((void *) (m_pool + i)) _Tp();
            }
            else
            {
                for (size_type i = m_size; i < count; ++i)
                    (m_pool + i)->~_Tp();
            }
            m_size = count;
        }

        void resize(size_type count, const value_type& value)
        {
            lock_guard lock_pool(m_mutex);
            if (count > m_size)
            {
                if (count > m_capacity)
                    this->reallocate(count);

                for (size_type i = m_size; i < count; ++i)
                    ::new((void*) (m_pool + i)) _Tp(value);
            }
            else
            {
                for (size_type i = m_size; i < count; ++i)
                    (m_pool + i)->~_Tp();
            }
            m_size = count;
        }

        void return_object(_Tp* obj)
        {
            {
                lock_guard lock_pool(m_mutex);
                m_free.push(obj);
            }
            m_objects_availabe.notify_one();
        }

        allocator_type get_allocator()
        {
            return m_allocator;
        }

        size_type size() const
        {
            lock_guard lock_pool(m_mutex);
            return m_free.size();
        }

        size_type managed_count() const
        {
            lock_guard lock_pool(m_mutex);
            return m_size;
        }

        size_type capacity() const
        {
            lock_guard lock_pool(m_mutex);
            return m_capacity;
        }

        bool in_use() const
        {
            return (managed_count() - size()) > 0;
        }

        bool empty() const
        {
            return size() == 0;
        }

        operator bool()
        {
            return size() > 0;
        }

        void swap(impl& other)
        {
            using std::swap;

            lock_guard this_lock(m_mutex);
            lock_guard other_lock(other.m_mutex);

            swap(m_pool, other.m_pool);
            swap(m_free, other.m_free);
            swap(m_allocator, other.m_allocator);
        }

    private:
        inline void reallocate(size_type count)
        {
            _Tp* new_pool = m_allocator.allocate(count);
            {
                std::memcpy(new_pool, m_pool, m_size * sizeof(_Tp));
                m_allocator.deallocate(m_pool, m_capacity);
                m_pool = new_pool;
                m_capacity = count;
            }
        }

        _Tp*                        m_pool;                 ///< Pointer to first object in the pool.
        stack_type                  m_free;                 ///< Stack of free objects.
        allocator_type              m_allocator;            ///< Allocates space for the pool.
        size_type                   m_size;                 ///< Number of objects currently in the pool.
        size_type                   m_capacity;             ///< Number of objects the pool can hold.
        mutable mutex_type          m_mutex;                ///< Controls access to the modifiers. That is, you must lock this mutex when using any of the above.

        mutex_type                  m_acq_mutex;            ///< Controls acquisition of objects.
        std::condition_variable     m_objects_availabe;     ///< Indicates presence of free objects.
    };

    // --- deleter IMPLEMENTATION -----------------------------------
    template <
        class _Tp,
        class _Allocator,
        class _Mutex
    >
    class object_pool<_Tp, _Allocator, _Mutex>::deleter
    {
    public:
        deleter(std::weak_ptr<impl> pool_ptr)
            : m_pool_ptr(pool_ptr)
        {}

        void operator()(_Tp* ptr)
        {
            if (auto pool = m_pool_ptr.lock())
                pool->return_object(ptr);
        }
    private:
        std::weak_ptr<impl> m_pool_ptr;
    };

    template <
        class _Tp,
        class _Allocator,
        class _Mutex
    >
    class object_pool<_Tp, _Allocator, _Mutex>::acquired_object
    {
    public:
        acquired_object()
            : m_is_initialized(false)
        {}

        acquired_object(none_t)
            : m_is_initialized(false)
        {}

        acquired_object(std::nullptr_t)
            : m_is_initialized(false)
        {}

        explicit
        acquired_object(_Tp* obj, std::shared_ptr<impl> lender)
            :   m_obj(obj),
                m_pool(lender),
                m_is_initialized(true)
        {
            assert(obj);
        }

        acquired_object(acquired_object&& other)
            :   m_obj(other.m_obj),
                m_pool(std::move(other.m_pool)),
                m_is_initialized(other.m_is_initialized)
        {
            other.m_obj = nullptr;
        }

        acquired_object(const acquired_object&) = delete;

        acquired_object& operator=(acquired_object&& other)
        {
            // return object to pool first
            deleter_type{m_pool}(m_obj);

            // acquire ownership of the managed object
            m_obj = other.m_obj;
            other.m_obj = nullptr;

            // manage pointer of lender
            m_pool = std::move(other.m_pool);

            // copy state
            m_is_initialized = other.m_is_initialized;
            other.m_is_initialized = false;

            return *this;
        }

        ~acquired_object()
        {
            assert(m_is_initialized);
            deleter_type{m_pool}(m_obj);
        }

        _Tp& operator*() 
        {
            if (!m_is_initialized)
                throw std::logic_error("acquired_object::operator*(): Initialization is required for data access.");
            else
                return *m_obj;
        }

        bool operator==(const none_t) const
        {
            return !m_is_initialized;
        }

        bool operator!=(const none_t) const
        {
            return !(*this == none);
        }

        explicit
        operator bool() const
        {
            return m_is_initialized;
        }

    private:
        _Tp*                        m_obj;
        std::shared_ptr<impl>       m_pool;
        bool                        m_is_initialized;
    };
}
#endif