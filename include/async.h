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

#include "async_private.h"

namespace wave {

template <typename... T>
class async_function : public detail::generic_source<T...>
{
public:
    async_function()
        : handle(std::make_shared<detail::async_handle<T...>>())
    {
        handle->self = handle;
    }

    template <typename... U>
    void operator()(U&&... values) const {
        handle->invoke(std::forward<U>(values)...);
    }

    void close() const { handle->close(); }
    void close_later() const { handle->close_later (); }

    template <typename F>
    void operator>>=(F&& f) {
        handle->async_cb.reset(
                    new detail::async_start<std::decay_t<F>, T...>{
                        std::forward<F>(f),
                        handle.get()
                    });
    }

private:
    std::shared_ptr<detail::async_handle<T...>> handle;
};

}
