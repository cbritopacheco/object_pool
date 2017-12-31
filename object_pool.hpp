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


namespace carlosb
{

    namespace internal
    {
    }

    template <
        class T,
        class Allocator = std::allocator<T>,
        class Deleter   = std::default_delete<T>,
        class Mutex     = std::mutex,
        class LockGuard = std::lock_guard<Mutex>
    >
    class object_pool
    {
    public:
        // --- HELPER CLASSES ---------------------
        class impl;
        class return_deleter;

    public:
        // --- TYPEDEFS ---------------------------
        using value_type        = T;
        using lv_reference      = T&;
        using rv_reference      = T&&;
        using const_reference   = const T&;
        using acquired_type     = std::unique_ptr<T, return_deleter>;

        using size_type         = std::size_t;

        using allocator_type    = Allocator;
        using deleter_type      = Deleter;
        using mutex_type        = Mutex;
        using lock_guard        = LockGuard;

        template <class... Params>
        object_pool(Params... params)
            : m_pool(std::make_shared<impl>(params...))
        {}

        acquired_type acquire()
        {
            return m_pool->acquire();
        }

        void insert(const T& value)
        {
            m_pool->insert(value);
        }

        void insert(T&& value)
        {
            m_pool->insert(std::move(value));
        }

    private:
        std::shared_ptr<impl> m_pool;
    };

    // --- IMPL IMPLEMENTATIONS -------------------------------------
    template <
        class T,
        class Allocator,
        class Deleter,
        class Mutex,
        class LockGuard
    >
    class object_pool<T, Allocator, Deleter, Mutex, LockGuard>::impl : public std::enable_shared_from_this<impl>
    {
    public:
        // --- CONSTRUCTORS -----------------------
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
            for (int i = 0; i < m_size; ++i)
                m_pool[i] = T(value);

            for (int i = 0; i < m_size; ++i)
                this->insert(m_pool + i);
        }

        impl(const Allocator& alloc = Allocator())
            :   m_size(0),
                m_capacity(4),
                m_allocator(alloc)
        {
            m_pool = m_allocator.allocate(m_capacity);
        }

        void reserve(size_type n)
        {
            if (m_pool)
                m_allocator.deallocate(m_pool, m_capacity);

            m_pool = m_allocator.allocate(n);
            m_capacity = n;
        }

        acquired_type acquire()
        {
            lock_guard lock(m_stack_mutex);

            if (m_free.empty())
            {
                throw std::out_of_range("No free objects available.");
            }
            else
            {   
                auto obj = acquired_type(m_free.top(), return_deleter{impl::shared_from_this()});
                m_free.pop();
                return std::move(obj);
            }
        }

        void insert(const T& value)
        {
            lock_guard lock(m_stack_mutex);
            if (m_size == m_capacity)
            {
                m_capacity *= 2;
                this->resize(m_capacity);
            }
            m_pool[m_size] = value;
            m_free.push(m_pool + m_size);
            ++m_size;
        }

        void insert(T&& value)
        {
            lock_guard lock(m_stack_mutex);
            if (m_size == m_capacity)
            {
                m_capacity *= 2;
                this->resize(m_capacity);
            }
            m_pool[m_size] = std::move(value);
            m_free.push(m_pool + m_size);
            ++m_size;
        }

        void resize(size_type n)
        {
            lock_guard lock(m_pool_mutex);
            T* new_pool = m_allocator.allocate(n);
            std::memcpy(new_pool, m_pool, sizeof(T) * m_size);
            m_allocator.deallocate(m_pool, m_capacity);
            m_pool = new_pool;
        }

        size_type size()
        {
            return m_size;
        }

        size_type capacity()
        {
            return m_capacity;
        }

    public:
        void insert(T* obj)
        {
            lock_guard lock(m_stack_mutex);
            m_free.push(obj);
        }

        // --- PRIVATE MEMBERS --------------------
        T*                                          m_pool;
        allocator_type                              m_allocator;
        size_type                                   m_size;
        size_type                                   m_capacity;
        std::stack<T*>                              m_free;
        mutex_type                                  m_pool_mutex;       ///< Controls access to the memory.
        mutex_type                                  m_stack_mutex;      ///< Controls access to the stack of free objects.
    };

    // --- RETURN_DELETER IMPLEMENTATIONS ---------------------------
    template <
        class T,
        class Allocator,
        class Deleter,
        class Mutex,
        class LockGuard
    >
    class object_pool<T, Allocator, Deleter, Mutex, LockGuard>::return_deleter
    {
    public:
        return_deleter(std::weak_ptr<impl> pool_ptr)
            : m_pool_ptr(pool_ptr)
        {}

        void operator()(T* ptr)
        {
            if (auto pool = m_pool_ptr.lock())
                pool->insert(ptr);
            else
                Deleter{}(ptr);
        }

    private:
        std::weak_ptr<impl> m_pool_ptr;
    };
}

#endif