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

struct idle_handle
{
    uv_idle_t idle;
    unsigned times;
    std::unique_ptr<callback> idle_cb;

    idle_handle(unsigned times)
        : times(times)
    {
        uv_idle_init(uv_default_loop(), &idle);
        idle.data = this;
        uv_idle_start(&idle, default_idle_cb);
    }

    static void default_idle_cb(uv_idle_t* handle)
    {
        auto h = static_cast<idle_handle*>(handle->data);
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
        if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&idle))) {
            uv_idle_stop(&idle);
            uv_close(reinterpret_cast<uv_handle_t*>(&idle), [](uv_handle_t* handle) {
                delete static_cast<idle_handle*>(handle->data);
            });
        }
    }
};

template <class F>
struct idling : public callback
{
    idling(F&& f, idle_handle* h)
        : functor(std::move(f))
    {
        h->idle_cb.reset(this);
        h->idle.idle_cb = cb;
    }

    idling(const F& f, idle_handle* h)
        : functor(f)
    {
        h->idle_cb.reset(this);
        h->idle.idle_cb = cb;
    }

    static void cb(uv_idle_t* handle)
    {
        auto h = static_cast<idle_handle*>(handle->data);
        try {
            auto start = static_cast<idling*>(h->idle_cb.get());
            start->functor();
            h->after();
        } catch (...) {
            h->stop();
            h->idle_cb.reset();
        }
    };

    F functor;
};

}
}
