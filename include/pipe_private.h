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
#include "stream_private.h"

namespace wave {
namespace detail {

struct pipe_handle : public stream_handle
{
    pipe_handle()
    {
        init();
    }

    pipe_handle(std::string domain)
    {
        init();
        uv_pipe_connect(&connect_handle, &pipe, domain.c_str(), connect_handle.cb);
    }

    pipe_handle(int fd)
    {
        init();
        uv_pipe_open(&pipe, fd);
    }

    void init()
    {
        uv_pipe_init(uv_default_loop(), &pipe, 1);
        stream_handle::init(reinterpret_cast<uv_stream_t*>(&pipe));
        close_cb = pipe_close_cb;
    }

    static void pipe_close_cb(uv_handle_t* handle)
    {
        delete static_cast<pipe_handle*>(handle->data);
    }

    uv_pipe_t pipe;
};

}
}
