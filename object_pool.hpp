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

#include <iostream>

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
        class T,
        class Allocator     = std::allocator<T>,
        class Mutex         = std::mutex
    >
    class object_pool;

    /**
     * Contains types not needed by the user.
     */
    namespace detail
    {   
        template <class...>
        using void_t = void;
    }

    /**
     * Contains meta-functions to determine if an object of type `T` satisfies
     * the requirements of some concept. Currently implemented:
     * - `is_lockable<T>`
     * - `is_allocator<T>`
     */
    namespace type_traits
    {
        template <class T, class = void>
        struct is_lockable : public std::false_type
        {};

        template <class T>
        struct is_lockable<T, detail::void_t<
            decltype(std::declval<T>().lock()),
            decltype(std::declval<T>().unlock()),
            decltype(std::declval<T>().try_lock()),
            std::is_same<decltype(std::declval<T>().try_lock()), bool>
        >> : std::true_type
        {};

        template <class T, class = void>
        struct is_allocator : public std::false_type
        {};

        template <class T>
        struct is_allocator<T, detail::void_t<
            typename T::value_type,
            typename T::size_type,
            decltype(std::declval<T>().allocate(std::declval<typename T::size_type>())),
            typename std::is_same<
                decltype(std::declval<T>().allocate(std::declval<typename T::size_type>())),
                typename T::value_type*
            >::type,
            decltype(std::declval<T>().deallocate(std::declval<typename T::value_type*>(), std::declval<typename T::size_type>()))
        >> : std::true_type
        {};
    }

    template <
        class T,
        class Allocator,
        class Mutex
    >
    class object_pool
    {
        static_assert(type_traits::is_allocator<Allocator>::value,
                      "Allocator template parameter must satisfy the BasicAllocator requirement.");
        static_assert(type_traits::is_lockable<Mutex>::value,
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

        /**
         * @brief      An acquired object is that which has been acquired by an object
         * manager. For this case, the acquired object is acquired through a call to the
         * function `acquire()` on an `object_pool`.
         * 
         * Member Types
         * ------------
         * - `value_type`     Type of acquired object.
         * - `lender_type`    Type of the object which has lent the object.
         * - `deleter_type`   Type of deleter which will return `acquired_object` to the lender.
         */
        class acquired_object;

    public:
        // --- TYPEDEFS ---------------------------
        using value_type        = T;                        ///< Type managed by the pool.
        using lv_reference      = T&;                       ///< l-value reference.
        using rv_reference      = T&&;                      ///< r-value reference.
        using const_reference   = const T&;                 ///< const l-value reference.

        using acquired_type     = acquired_object;          ///< Type of acquired objects.
        using deleter_type      = deleter;                  ///< Custom deleter of pool. It returns objects back to the pool.
        using stack_type        = std::stack<T*>;           ///< Stack of pointers to the managed objects. Only contains unused objects.

        using size_type         = std::size_t;              ///< Size type used.

        using allocator_type    = Allocator;                ///< Type of allocator.
        using mutex_type        = Mutex;                    ///< Type of mutex.
        using lock_guard        = std::lock_guard<Mutex>;   ///< Type of lock guard.

        template <class... Args>
        object_pool(Args&&... args)
            :   m_pool(std::make_shared<impl>(std::forward<Args>(args)...))
        {}

        acquired_type acquire()
        {
            return m_pool->acquire();
        }

        acquired_type acquire_lock()
        {
            return m_pool->acquire_lock();
        }

        void push(const_reference value)
        {
            m_pool->push(value);
        }

        void push(rv_reference value)
        {
            m_pool->push(std::move(value));
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            m_pool->emplace(std::forward<Args>(args)...);
        }

        void resize(size_type count)
        {
            m_pool->resize(count);
        }

        void resize(size_type count, const value_type& value)
        {
            m_pool->resize(count, value);
        }

        void reserve(size_type count)
        {
            m_pool->reserve(count);
        }

        size_type size()
        {
            return m_pool->size();
        }

        size_type capacity() const
        {
            return m_pool->capacity();
        }

        bool empty()
        {
            return m_pool->empty();
        }

        operator bool()
        {
            return m_pool->operator bool();
        }
    private:
        std::shared_ptr<impl>       m_pool;                ///< Pointer to implementation of pool.
    };

    // --- impl IMPLEMENTATION --------------------------------------
    template <
        class T,
        class Allocator,
        class Mutex
    >
    class object_pool<T, Allocator, Mutex>::impl : public std::enable_shared_from_this<impl>
    {
        friend void deleter::operator()(T* ptr);
    public:
        impl()
            : impl(Allocator())
        {
            static_assert(std::is_default_constructible<Allocator>::value,
                          "Allocator must be DefaultConstructible to use this constructor.");
        }

        explicit impl(const Allocator& alloc)
            :   m_size(0),
                m_capacity(4),
                m_allocator(alloc)
        {
            m_pool = m_allocator.allocate(m_capacity);
        }

        impl(size_type count, const T& value, const Allocator& alloc = Allocator())
            :   m_size(count),
                m_capacity(count),
                m_allocator(alloc)
        {   
            static_assert(std::is_copy_constructible<T>::value,
                          "T must be CopyConstructible to use this constructor.");

            m_pool = m_allocator.allocate(m_size);
            for (size_type i = 0; i < m_size; ++i)
                m_pool[i] = T(value);

            for (size_type i = 0; i < m_size; ++i)
                m_free.push(m_pool + i);
        }

        explicit impl(size_type count, const Allocator& alloc = Allocator())
            :   m_size(count),
                m_capacity(count),
                m_allocator(alloc)
        {
            m_pool = m_allocator.allocate(count);
        }

        ~impl()
        {
            lock_guard lock_pool(m_mutex);
            m_allocator.deallocate(m_pool, m_capacity);
        }

        void reserve(size_type count)
        {
            lock_guard lock_pool(m_mutex);
            m_allocator.deallocate(m_pool, m_capacity);
            m_pool = m_allocator.allocate(count);
            m_capacity = count;
        }

        acquired_type acquire()
        {   
            lock_guard lock_pool(m_mutex);
            if (m_free.empty())
                throw std::out_of_range("No free objects in the pool.");
            acquired_type obj(m_free.top(), impl::shared_from_this());
            m_free.pop();
            return std::move(obj);
        }

        acquired_type acquire_lock()
        {
            std::unique_lock<mutex_type> acq_lock(m_acq_mutex);
            m_objects_availabe.wait(acq_lock, [this] (void) { return !this->empty(); });

            acquired_type acq(acquire());

            acq_lock.unlock();
            m_objects_availabe.notify_one();
            
            return acq;
        }

        void push(const_reference value)
        {
            lock_guard lock_pool(m_mutex);
            if (m_size == m_capacity)
                this->reallocate(m_capacity * 2);
            m_pool[m_size] = value;
            m_free.push(m_pool + m_size);
            ++m_size;

            m_objects_availabe.notify_one();
        }

        void push(rv_reference value)
        {
            lock_guard lock_pool(m_mutex);
            if (m_size == m_capacity)
                this->reallocate(m_capacity * 2);
            m_pool[m_size] = std::move(value);
            m_free.push(m_pool + m_size);
            ++m_size;

            m_objects_availabe.notify_one();
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            lock_guard lock_pool(m_mutex);
            if (m_size == m_capacity)
                this->reallocate(m_capacity * 2);
            m_pool[m_size] = T(std::forward<Args>(args)...);
            m_free.push(m_pool + m_size); 
            ++m_size;

            m_objects_availabe.notify_one();
        }

        void resize(size_type count)
        {
            lock_guard lock_pool(m_mutex);
            if (count > m_size)
            {
                if (count > m_capacity)
                    this->reallocate(count);
            }
            else
            {
                for (size_type i = m_size; i < count; ++i)
                    m_pool[i].~T();
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
                    m_pool[i] = value;
            }
            else
            {
                for (size_type i = m_size; i < count; ++i)
                    m_pool[i].~T();
            }
            m_size = count;
        }

        size_type size()
        {
            return m_free.size();
        }

        size_type capacity() const
        {
            return m_capacity;
        }

        bool empty()
        {
            return size() == 0;
        }

        operator bool()
        {
            return !empty();
        }
    private:
        void reallocate(size_type count)
        {
            T* new_pool = m_allocator.allocate(count);
            {
                std::memcpy(new_pool, m_pool, m_size * sizeof(T));
                m_allocator.deallocate(m_pool, m_capacity);
                m_pool = new_pool;
                m_capacity = count;
            }
        }

        void return_object(T* obj)
        {
            lock_guard lock_pool(m_mutex);
            m_free.push(obj);
            m_objects_availabe.notify_one();
        }

        T*                          m_pool;                 ///< Pointer to first object in the pool.
        stack_type                  m_free;                 ///< Stack of free objects.
        allocator_type              m_allocator;            ///< Allocates space for the pool.
        size_type                   m_size;                 ///< Number of objects currently in the pool.
        size_type                   m_capacity;             ///< Number of objects the pool can hold.
        mutex_type                  m_mutex;                ///< Controls access to the modifiers.

        mutex_type                  m_acq_mutex;
        std::condition_variable     m_objects_availabe;
    };

    // --- deleter IMPLEMENTATION -----------------------------------
    template <
        class T,
        class Allocator,
        class Mutex
    >
    class object_pool<T, Allocator, Mutex>::deleter
    {
    public:
        deleter(std::weak_ptr<impl> pool_ptr)
            : m_pool_ptr(pool_ptr)
        {}

        void operator()(T* ptr)
        {
            if (auto pool = m_pool_ptr.lock())
                pool->return_object(ptr);
        }

    private:
        std::weak_ptr<impl> m_pool_ptr;
    };

    template <
        class T,
        class Allocator,
        class Mutex
    >
    class object_pool<T, Allocator, Mutex>::acquired_object
    {
    public:
        using value_type    = T;
        using lender_type   = object_pool<T>;
        using deleter_type  = typename object_pool<T>::deleter_type;

        acquired_object(std::unique_ptr<T, deleter_type> ptr)
            : m_ptr(std::move(ptr))
        {}

        acquired_object(T* ptr, std::shared_ptr<impl> lender_ptr)
            : acquired_object(std::unique_ptr<T, deleter_type>{ptr, deleter_type(lender_ptr)})
        {}

        auto operator*() 
            -> decltype(std::declval<std::unique_ptr<T, deleter_type>>().operator*())
        {
            if (!m_ptr)
                throw std::logic_error("Attempting to access data from an object that hasn't been acquired yet!");
            return *m_ptr;
        }

        operator bool()
        {
            return static_cast<bool>(m_ptr);
        }
    private:
        std::unique_ptr<T, deleter_type> m_ptr;
    };
}
#endif