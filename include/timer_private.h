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

namespace wave {
namespace detail {

struct timer_handle
{
    uv_timer_t timer;
    unsigned times;
    std::unique_ptr<callback> timer_cb;
    std::exception_ptr cought_ex;

    timer_handle(unsigned long long timeout, unsigned times = 1)
        : times(times)
    {
        uv_timer_init(uv_default_loop(), &timer);
        timer.data = this;
        uv_timer_start(&timer, default_timer_cb, timeout, timeout);
    }

    static void default_timer_cb(uv_timer_t* handle)
    {
        auto h = static_cast<timer_handle*>(handle->data);
        h->stop();
    }

    void after()
    {
        if (times > 0 && --times == 0) {
            stop();
        }
    }

    void stop()
    {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&timer))) {
            uv_timer_stop(&timer);
            uv_close(reinterpret_cast<uv_handle_t*>(&timer),
                     [](uv_handle_t* handle) {
                auto p = static_cast<timer_handle*>(handle->data);
                delete p;
            });
        }
    }
};

template <class F>
struct ticking : public callback
{
    F functor;
    ticking(F f, timer_handle* h)
        : functor(std::move(f))
    {
        h->timer.timer_cb = cb;
    }

    static void cb(uv_timer_t* handle)
    {
        auto h = static_cast<timer_handle*>(handle->data);
        try {
            auto start = static_cast<ticking*>(h->timer_cb.get());
            start->functor();
            h->after();
        } catch (...) {
            h->stop();
            h->timer_cb.reset();
        }
    }
};

}
}
