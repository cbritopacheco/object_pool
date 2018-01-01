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
#include <stack>
#include <vector>
#include <stdexcept>
#include <type_traits>

#include <iostream>

namespace carlosb
{
    namespace internal
    {   
        template <class...>
        using void_t = void;

        template <class T, class = void>
        struct is_basic_lockable : public std::false_type
        {};

        template <class T>
        struct is_basic_lockable<T, void_t<
            decltype(std::declval<T>().lock()),
            decltype(std::declval<T>().unlock())
        >> : std::true_type
        {};

        template <class T, class = void>
        struct is_allocator : public std::false_type
        {};

        template <class T>
        struct is_allocator<T, void_t<
            typename T::value_type,
            typename T::size_type,
            typename std::is_same<
                decltype(std::declval<T>().allocate(std::declval<typename T::size_type>())),
                typename T::value_type*>::type,
            decltype(std::declval<T>().deallocate(std::declval<typename T::value_type*>(), std::declval<typename T::size_type>()))
        >> : std::true_type
        {};
    }

    template <
        class T,
        class Allocator     = std::allocator<T>,
        class Mutex         = std::mutex,
        class LockGuard     = std::lock_guard<Mutex>
    >
    class object_pool
    {
        static_assert(internal::is_allocator<Allocator>::value,
                      "Allocator template parameter must satisfy the BasicAllocator requirement.");
        static_assert(internal::is_basic_lockable<Mutex>::value,
                      "Mutex template parameter must satisfy the BasicLockable requirement.");
    public:
        // --- HELPER CLASSES ---------------------
        class impl;
        class deleter;

    public:
        // --- TYPEDEFS ---------------------------
        using value_type        = T;
        using lv_reference      = T&;
        using rv_reference      = T&&;
        using const_reference   = const T&;
        
        using acquired_type     = std::unique_ptr<T, deleter>;
        using stack_type        = std::stack<T*>;

        using size_type         = std::size_t;

        using allocator_type    = Allocator;
        using mutex_type        = Mutex;
        using lock_guard        = LockGuard;

        template <class... Args>
        object_pool(Args&&... args)
            :   m_pool(std::make_shared<impl>(std::forward<Args>(args)...))
        {}

        acquired_type acquire()
        {
            lock_guard lock(m_modifier_mutex);
            return m_pool->acquire();
        }

        void insert(const_reference value)
        {
            lock_guard lock(m_modifier_mutex);
            m_pool->insert(value);
        }

        void insert(rv_reference value)
        {
            lock_guard lock(m_modifier_mutex);
            m_pool->insert(std::move(value));
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            lock_guard lock(m_modifier_mutex);
            m_pool->emplace(std::forward<Args>(args)...);
        }

        void resize(size_type count)
        {
            lock_guard lock(m_modifier_mutex);
            m_pool->resize(count);
        }

        void resize(size_type count, const value_type& value)
        {
            lock_guard lock(m_modifier_mutex);
            m_pool->resize(count, value);
        }

        void reserve(size_type count)
        {
            lock_guard lock(m_modifier_mutex);
            m_pool->reserve(count);
        }

        size_type size() const
        {
            return m_pool->size();
        }

        size_type capacity() const
        {
            return m_pool->capacity();
        }

        bool empty() const
        {
            return m_pool->empty();
        }

    private:
        std::shared_ptr<impl>       m_pool;
        mutex_type                  m_modifier_mutex;     ///< Controls access to the modifiers.
    };

    // --- impl IMPLEMENTATION --------------------------------------
    template <
        class T,
        class Allocator,
        class Mutex,
        class LockGuard
    >
    class object_pool<T, Allocator, Mutex, LockGuard>::impl : public std::enable_shared_from_this<impl>
    {
        // --- Friend Functions ----------------
        friend void deleter::operator()(T* ptr);

    public:
        template <typename = typename std::enable_if<
            std::is_default_constructible<Allocator>::value>
        ::type
        >
        impl()
            : impl(Allocator())
        {}

        explicit impl(const Allocator& alloc)
            :   m_size(0),
                m_capacity(4),
                m_allocator(alloc)
        {
            m_pool = m_allocator.allocate(m_capacity);
        }

        template <typename = typename std::enable_if<
            std::is_default_constructible<T>::value && std::is_copy_constructible<T>::value>
        ::type
        >
        impl(size_type count, const T& value, const Allocator& alloc = Allocator())
            :   m_size(count),
                m_capacity(count),
                m_allocator(alloc)
        {   
            m_pool = m_allocator.allocate(m_size);
            for (size_type i = 0; i < m_size; ++i)
                m_pool[i] = T(value);

            for (size_type i = 0; i < m_size; ++i)
                this->insert(m_pool + i);
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
            m_allocator.deallocate(m_pool, m_capacity);
        }

        void reserve(size_type n)
        {
            m_allocator.deallocate(m_pool, m_capacity);
            m_pool = m_allocator.allocate(n);
            m_capacity = n;
        }

        acquired_type acquire()
        {
            if (m_free.empty())
            {
                throw std::out_of_range("No free objects available.");
            }
            else
            {   
                auto obj = acquired_type(m_free.top(), deleter{impl::shared_from_this()});
                m_free.pop();
                return std::move(obj);
            }  
        }

        void insert(const_reference value)
        {
            if (m_size == m_capacity)
                this->reallocate(m_capacity * 2);
            m_pool[m_size] = value;
            m_free.push(m_pool + m_size);
            ++m_size;
        }

        void insert(rv_reference value)
        {
            if (m_size == m_capacity)
                this->reallocate(m_capacity * 2);
            m_pool[m_size] = std::move(value);
            m_free.push(m_pool + m_size);
            ++m_size;
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            if (m_size == m_capacity)
                this->reallocate(m_capacity * 2);
            m_pool[m_size] = T(std::forward<Args>(args)...);
            m_free.push(m_pool + m_size); 
            ++m_size;
        }

        void resize(size_type count)
        {
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

        size_type size() const
        {
            return m_size;
        }

        size_type capacity() const
        {
            return m_capacity;
        }

        bool empty() const
        {
            return m_size == 0;
        }

    private:
        void insert(T* obj)
        {
            m_free.push(obj);
        }

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

        T*                                          m_pool;
        stack_type                                  m_free;
        allocator_type                              m_allocator;
        size_type                                   m_size;
        size_type                                   m_capacity;
    };

    // --- deleter IMPLEMENTATION -----------------------------------
    template <
        class T,
        class Allocator,
        class Mutex,
        class LockGuard
    >
    class object_pool<T, Allocator, Mutex, LockGuard>::deleter
    {
    public:
        deleter(std::weak_ptr<impl> pool_ptr)
            : m_pool_ptr(pool_ptr)
        {}

        void operator()(T* ptr)
        {
            if (auto pool = m_pool_ptr.lock())
                pool->insert(ptr);
        }

    private:
        std::weak_ptr<impl> m_pool_ptr;
    };
}
#endif