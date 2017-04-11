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

#include <queue>

#include "wave_private.h"

namespace wave {
namespace detail {

template <typename T, typename U>
struct decay_zip_map;

template <typename... T, typename... U>
struct decay_zip_map <generic_source<T...>, generic_source<U...>> : public generic_source<T..., U...>
{
    std::queue<std::tuple<T...>> queue1;
    std::queue<std::tuple<U...>> queue2;

    template <typename F>
    void check_pop(F& f)
    {
        if (queue1.size() > 0 && queue2.size() > 0) {
            this->then_expand(f, std::index_sequence_for<T...>{}, std::index_sequence_for<U...>{});
            queue1.pop();
            queue2.pop();
        }
    }

    template <typename F, size_t... I, size_t... J>
    void then_expand(F &f, std::index_sequence<I...>, std::index_sequence<J...>)
    {
        auto& t1 = queue1.front();
        auto& t2 = queue2.front();
        f(std::move(std::get<I>(t1))..., std::move(std::get<J>(t2))...);
    }

    template <typename F>
    constexpr decltype(auto) queue_args1(F& f)
    {
        return [this, f](T... args) {
            queue1.emplace(std::tuple<T...>(std::move(args)...));
            check_pop(f);
        };
    }

    template <typename F>
    constexpr decltype(auto) queue_args2(F& f)
    {
        return [this, f](U... args) {
            queue2.emplace(std::tuple<U...>(std::move(args)...));
            check_pop(f);
        };
    }
};

template <typename T, typename U>
struct zip_map : public decay_zip_map <typename T::source_type, typename U::source_type>
{
    zip_map(T t, U u)
        : t(std::move(t))
        , u(std::move(u))
    {
    }

    template <typename F>
    void operator>>=(F f)
    {
        t >>= this->queue_args1(f);
        u >>= this->queue_args2(f);
    }

    T t;
    U u;
};

}

template <typename T, typename U>
decltype(auto) zip(T&& t, U&& u)
{
    return detail::zip_map<std::decay_t<T>,std::decay_t<U>> {
            std::forward<T>(t),
            std::forward<U>(u)
    };
}

}
