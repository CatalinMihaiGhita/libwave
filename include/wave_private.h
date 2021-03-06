/*
* Copyright(c) 2017 Catalin Mihai Ghita
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

#include <exception>
#include <utility>

namespace wave {
namespace detail {

struct callback
{
    virtual ~callback() {}
};

struct abstract_source
{
};

template <typename... Args>
struct generic_source : public abstract_source
{
    typedef generic_source source_type;
};

template <typename F, typename D>
struct closure : public F
{
    closure(F&& f, D&& d)
        : F(std::move(f))
        , d(std::move(d))
        , valid(true)
    {}

    closure(const F& f, const D& d)
        : F(f)
        , d(d)
        , valid(true)
    {}

    closure(const closure& other) = default;

    closure(closure&& other)
        : F(std::move(other))
        , d(std::move(other.d))
        , valid(true)
    {
        other.valid = false;
    }

    ~closure()
    {
        if (valid) {
            d();
        }
    }

    D d;
    bool valid;
};

template <typename T, typename U, typename R>
struct map
{
    template <class... Args>
    decltype(auto) operator()(Args&&... args)
    {
        return u(t(std::forward<Args>(args)...));
    }
    T t; U u;
};

template <typename T, typename U>
struct map<T, U, void>
{
    template <class... Args>
    decltype(auto) operator()(Args&&... args)
    {
        t(std::forward<Args>(args)...);
        return u();
    }
    T t; U u;
};

template <typename T, typename U>
struct flat_map
{
    template <class... Args>
    void operator()(Args&&... args)
    {
        auto source = t(std::forward<Args>(args)...);
        source >>= u;
    }
    T t; U u;
};

template <typename... T>
struct function_handle
{
    function_handle()
        : f(nullptr)
    {}

    void(*f)(void*, T...);
    std::unique_ptr<callback> cb;
};

template <typename F, typename... T>
struct function_callback : public callback
{
    function_callback(F f, function_handle<T...>* h)
        : functor(std::move(f))
    {
        h->f = cb;
    }

    static void cb(void* data, T... args)
    {
        auto h = static_cast<function_handle<T...>*>(data);
        try {
            auto p = static_cast<function_callback*>(h->cb.get());
            p->functor(std::move(args)...);
        }
        catch (...) {
            h->cb.reset();
        }
    }

    F functor;
};

template <typename T>
struct lambda_decay;

template <typename ClassType, typename ReturnType, typename... Args>
struct lambda_decay<ReturnType(ClassType::*)(Args...) const>
{
    enum { arity = sizeof...(Args) };

    typedef ReturnType result_type;
    typedef std::index_sequence_for<Args...> args_index_sequence;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

template <typename ClassType, typename ReturnType, typename... Args>
struct lambda_decay<ReturnType(ClassType::*)(Args...)>
{
    enum { arity = sizeof...(Args) };

    typedef ReturnType result_type;
    typedef std::index_sequence_for<Args...> args_index_sequence;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

template <typename T, typename O = decltype(&T::operator())>
struct lambda : public lambda_decay<O>
{};

}
}
