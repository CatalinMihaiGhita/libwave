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

#include <uv.h>

#include <memory>
#include <functional>
#include <iostream>

#include "wave_private.h"
#include "async_private.h"

#define $ [=]
#define $finally || [=] () noexcept

namespace wave {

template <typename T>
constexpr bool is_source_v = std::is_base_of<detail::abstract_source, T>::value;

template <typename T
          ,typename U
          ,typename DT = typename std::decay_t<T>
          ,typename DU = typename std::decay_t<U>
          ,typename R = typename detail::lambda<DT>::result_type>
using map_t = typename std::conditional_t<is_source_v<R>, detail::flat_map<DT, DU>, detail::map<DT, DU, R>>;

template <typename T
         ,typename U
         ,typename M = map_t<T,U>>
M operator>>=(T&& t, U&& u)
{
    return M{ std::forward<T>(t), std::forward<U>(u) };
}

template <typename F
         ,typename Exit
         ,typename = typename detail::lambda<std::decay_t<F>>::result_type>
decltype(auto) operator||(F&& f, Exit&& exit)
{
    static_assert(noexcept(exit()), "Exit function must not throw");
    return detail::closure<std::decay_t<F>, std::decay_t<Exit>>{std::forward<F>(f), std::forward<Exit>(exit)};
}

template <typename Handle, template<typename...> class Callback, typename... Args>
struct source : public detail::generic_source<Args...>
{
    typedef source base;

    template <class F>
    void operator>>= (F&& functor) const
    {
        new Callback<std::decay_t<F>, Args...>{ std::forward<F>(functor), handle };
    }

    source(Handle handle)
        : handle(std::move(handle))
    {}

    Handle handle;
};

template <typename... T>
class function : public detail::generic_source<T...>
{
public:
    function()
        : handle{std::make_shared<detail::function_handle<T...>>()}
    {}

    template <class... U>
    const function& operator()(U&&... values) const
    {
        handle->f(handle.get(), std::forward<U>(values)...);
        return *this;
    }

    void close() const
    {
        handle->source_cb.reset();
    }

    template <typename F>
    void operator>>=(F&& f) const
    {
        handle->cb.reset(
            new detail::function_callback<std::decay_t<F>, T...>{
                std::forward<F>(f),
                handle.get()
            });
    }

private:
    std::shared_ptr<detail::function_handle<T...>> handle;
};

struct loop
{

    ~loop()
    {
        (void)uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        uv_loop_close(uv_default_loop());
    }
};

template <typename T>
class ref : public std::shared_ptr<T>
{
public:
    template <typename... Us>
    ref(Us&&... us)
        : std::shared_ptr<T>(std::make_shared<T>(std::forward<Us>(us)...))
    {}

    std::add_lvalue_reference_t<T> operator()() const
    {
        return this->operator *();
    }
};

struct nothing
{
    template <typename... Args>
    void operator()(Args&&...)
    {}
};

void rethrow()
{
    if (auto ex = std::current_exception()) {
        std::rethrow_exception(ex);
    }
}

}



