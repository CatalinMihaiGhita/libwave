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
#include <iostream>

#include <memory>
#include <string>

#include <uv.h>

#include "memory.h"

namespace wave {
namespace detail {
struct stream_handle
{
    stream_handle()
        : stream{nullptr}
    {}

    void init(uv_stream_t* s)
    {
        stream = s;
        stream->data = this;
        write_handle.data = this;
        write_handle.cb = default_write_cb;
        stream->alloc_cb = default_alloc_cb;
        stream->read_cb = nullptr;
        connect_handle.data = this;
        connect_handle.cb = default_connect_cb;
    }

    static void default_alloc_cb(uv_handle_t *, size_t , uv_buf_t *)
    {
        std::cout << "Warning! Reading tcp without callback." << std::endl;
    }

    void stop_reading()
    {
        uv_read_stop(reinterpret_cast<uv_stream_t*>(stream));
        read_cb.reset();
    }

    void cancel_write()
    {
        uv_cancel(reinterpret_cast<uv_req_t*>(&write_handle));
    }

    void shutdown()
    {
        shutdown_handle.data = this;
        uv_shutdown(&shutdown_handle, stream, [](uv_shutdown_t* req, int status) {
            auto p = static_cast<stream_handle*>(req->data);
            if (status == 0) {
                p->close();
            }
        });
    }

    static void default_write_cb(uv_write_t *req, int status)
    {
        auto p = static_cast<stream_handle*>(req->data);
        if (status != 0) {
            p->close();
        }
    }

    static void default_connect_cb(uv_connect_t *req, int status)
    {
        auto p = static_cast<stream_handle*>(req->data);
        p->close();
    }

    static void default_close_cb(uv_handle_t* handle)
    {
        delete static_cast<stream_handle*>(handle->data);
    }

    template <typename String>
    void write(String&& s)
    {
        data = std::forward<String>(s);
        buff = uv_buf_init(const_cast<char*>(data.data()), data.size());
        uv_write(&write_handle, stream, &buff, 1, write_handle.cb);
    }

    void start_reading()
    {
        uv_read_start(stream, stream->alloc_cb, stream->read_cb);
    }

    void close()
    {
        connect_cb.reset();
        if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(stream))) {
            uv_close(reinterpret_cast<uv_handle_t*>(stream), close_cb);
        }
    }

    uv_stream_t* stream;
    uv_shutdown_t shutdown_handle;
    uv_connect_t connect_handle;
    uv_write_t write_handle;
    uv_buf_t buff;
    std::string data;
    std::unique_ptr<callback> connect_cb;
    std::unique_ptr<callback> read_cb;
    std::unique_ptr<callback> write_cb;
    uv_close_cb close_cb;
};

template <typename F, typename S>
struct stream_read : public callback
{
    stream_read(F f, stream_handle* h)
        : functor(std::move(f))
    {
        h->read_cb.reset(this);
        h->stream->alloc_cb = alloc_cb;
        h->stream->read_cb = cb;
        h->start_reading();
    }

    static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
    {
        auto h = static_cast<stream_handle*>(handle->data);
        auto p = static_cast<stream_read*>(h->read_cb.get());
        p->data.reserve(suggested_size);
        *buf = uv_buf_init(const_cast<char*>(p->data.data()), suggested_size);
    }

    static void cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
    {
        auto h = static_cast<stream_handle*>(handle->data);
        try {
            auto p = static_cast<stream_read*>(h->read_cb.get());
            if (nread < 0) {
                throw std::exception();
            }
            p->functor(std::string(buf->base, nread));
        }
        catch (...) {
            h->stop_reading();
        }
    }

    F functor;
    std::vector<char> data;
};

template <typename F>
struct stream_write : public callback
{
    stream_write(F f, stream_handle* h)
        : functor(std::move(f))
    {
        h->write_cb.reset(this);
        h->write_handle.cb = cb;
    }

    static void cb(uv_write_t *req, int status)
    {
        auto h = static_cast<stream_handle*>(req->data);
        try {
            if (status != 0) {
                throw std::exception();
            }
            auto p = static_cast<stream_write*>(h->write_cb.get());
            p->functor();
        }
        catch (...) {
            h->cancel_write();
            h->write_cb.reset();
        }
    }
    F functor;
};


template <typename F>
struct stream_connect : public callback
{
    stream_connect(F f, stream_handle* h)
        : functor(std::move(f))
    {
        h->connect_cb.reset(this);
        h->connect_handle.cb = cb;
    }

    static void cb(uv_connect_t* req, int status)
    {
        auto h = static_cast<stream_handle*>(req->data);
        try {
            if (status != 0) {
                throw std::exception();
            }
            auto p = static_cast<stream_connect*>(h->connect_cb.get());
            p->functor();
        }
        catch (...) {
            h->close();
        }
    }

    F functor;
};


}
}
