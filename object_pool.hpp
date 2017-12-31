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
    }

    template <
        class T,
        class Allocator         = std::allocator<T>,
        class RecursiveMutex    = std::recursive_mutex,
        class LockGuard         = std::lock_guard<RecursiveMutex>
    >
    class object_pool
    {
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

        using size_type         = std::size_t;

        using allocator_type    = Allocator;
        using mutex_type        = RecursiveMutex;
        using lock_guard        = LockGuard;

        explicit object_pool(size_type count, const T& value, const Allocator& alloc = Allocator())
            : m_pool(std::make_shared<impl>(count, value, alloc))
        {}

        acquired_type acquire()
        {
            return m_pool->acquire();
        }

        void insert(const_reference value)
        {
            m_pool->insert(value);
        }

        void insert(rv_reference value)
        {
            m_pool->insert(std::move(value));
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
        std::shared_ptr<impl> m_pool;
    };

    // --- IMPL IMPLEMENTATIONS -------------------------------------
    template <
        class T,
        class Allocator,
        class Mutex,
        class LockGuard
    >
    class object_pool<T, Allocator, Mutex, LockGuard>::impl : public std::enable_shared_from_this<impl>
    {
        friend void deleter::operator()(T* ptr);
    public:
        template <typename = typename std::enable_if<
            std::is_default_constructible<T>::value && std::is_copy_constructible<T>::value>
        ::type
        >
        explicit impl(size_type count, const T& value, const Allocator& alloc = Allocator())
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

        explicit impl(const Allocator& alloc = Allocator())
            :   m_size(0),
                m_capacity(4),
                m_allocator(alloc)
        {
            m_pool = m_allocator.allocate(m_capacity);
        }

        ~impl()
        {
            m_allocator.deallocate(m_pool, m_capacity);
        }

        void reserve(size_type n)
        {
            lock_guard lock(m_pool_mutex);

            if (m_pool)
                m_allocator.deallocate(m_pool, m_capacity);

            m_pool = m_allocator.allocate(n);
            m_capacity = n;
        }

        acquired_type acquire()
        {
            {
                lock_guard lock(m_stack_mutex);

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
        }

        void insert(const_reference value)
        {
            {
                lock_guard lock_pool(m_pool_mutex);
                if (m_size == m_capacity)
                    this->reallocate(m_capacity * 2);

                m_pool[m_size] = value;
            }

            {
                lock_guard lock_stack(m_stack_mutex);
                m_free.push(m_pool + m_size);
            }

            {
                lock_guard lock_pool(m_pool_mutex);
                ++m_size;
            }
        }

        void insert(rv_reference value)
        {
            {
                lock_guard lock_pool(m_pool_mutex);
                if (m_size == m_capacity)
                    this->reallocate(m_capacity * 2);

                m_pool[m_size] = std::move(value);
            }

            {
                lock_guard lock_stack(m_stack_mutex);
                m_free.push(m_pool + m_size);
            }
            
            {
                lock_guard lock_pool(m_pool_mutex);
                ++m_size;
            }
            
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            {
                lock_guard lock_pool(m_pool_mutex);

                if (m_size == m_capacity)
                    this->reallocate(m_capacity * 2);

                m_pool[m_size] = T(std::forward<Args>(args)...);
            }

            {
                lock_guard lock_stack(m_stack_mutex);
                m_free.push(m_pool + m_size); 
            }

            {
                lock_guard lock_pool(m_pool_mutex);
                ++m_size;
            }   
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
            lock_guard lock_stack(m_stack_mutex);
            m_free.push(obj);
        }

        void reallocate(size_type count)
        {
            T* new_pool = m_allocator.allocate(count);
            {
                lock_guard lock(m_pool_mutex);
                std::memcpy(new_pool, m_pool, m_size * sizeof(T));
                m_allocator.deallocate(m_pool, m_capacity);
                m_pool = new_pool;
                m_capacity = count;
            }
        }

        T*                                          m_pool;
        allocator_type                              m_allocator;
        size_type                                   m_size;
        size_type                                   m_capacity;
        std::stack<T*>                              m_free;
        mutex_type                                  m_pool_mutex;       ///< Controls access to the memory.
        mutex_type                                  m_stack_mutex;      ///< Controls access to the stack of free objects.
    };

    // --- deleter IMPLEMENTATIONS ---------------------------
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