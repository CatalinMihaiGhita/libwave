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

#include <memory>
#include <string>
#include <iostream>

#include <uv.h>

#include "wave.h"
#include "pipe.h"

namespace wave {
namespace detail {
struct process_handle
{
    uv_process_t process;
    uv_process_options_t options{0};
    std::unique_ptr<callback> exit_cb;
    detail::pipe_handle* stdin_pipe;
    detail::pipe_handle* stdout_pipe;
    detail::pipe_handle* stderr_pipe;
    uv_stdio_container_t child_stdio[3];
    std::initializer_list<std::string> list;
    char** raw_args;

    process_handle(std::initializer_list<std::string> args)
        : stdin_pipe(new detail::pipe_handle())
        , stdout_pipe(new detail::pipe_handle())
        , stderr_pipe(new detail::pipe_handle())
        , list(std::move(args))
        , raw_args(new char*[list.size() + 1])
    {
        size_t i = 0;
        for (auto& arg : list) {
            raw_args[i++] = const_cast<char*>(arg.c_str());
        }
        raw_args[i] = nullptr;
        process.data = this;
        options.args = raw_args;
        options.file = list.begin()->c_str();
        options.exit_cb = default_exit_cb;

        child_stdio[0].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
        child_stdio[0].data.stream = stdin_pipe->stream;

        child_stdio[1].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        child_stdio[1].data.stream = stdout_pipe->stream;

        child_stdio[2].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        child_stdio[2].data.stream = stderr_pipe->stream;

        options.stdio = child_stdio;
        options.stdio_count = 3;

        if (uv_spawn(uv_default_loop(), &process, &options)) {
            std::cout << "Error" << std::endl;
            throw std::runtime_error("Process " + *list.begin() + " spawn failed");
        }
    }

    ~process_handle()
    {
        delete[] raw_args;
    }

    static void default_exit_cb(uv_process_t* process, int64_t code, int sig)
    {
        auto p = static_cast<process_handle*>(process->data);
        p->close();
    }

    void close() {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&process))) {
            uv_close(reinterpret_cast<uv_handle_t*>(&process), [](uv_handle_t* handle) {
                delete static_cast<process_handle*>(handle->data);
            });
        }
    }

    void kill(int signum)
    {
        uv_process_kill(&process, signum);
    }
};

template <class F>
struct process_finished : public callback
{
    process_finished(F&& f, process_handle* h)
        : functor(std::move(f))
    {
        h->exit_cb.reset(this);
        h->process.exit_cb = cb;
    }

    process_finished(const F& f, process_handle* h)
        : functor(f)
    {
        h->exit_cb.reset(this);
        h->process.exit_cb = cb;
    }

    static void cb(uv_process_t* handle, int64_t exit_status, int term_signal)
    {
        auto h = static_cast<process_handle*>(handle->data);
        try {
            auto exit = static_cast<process_finished*>(h->exit_cb.get());
            exit->functor(exit_status, term_signal);
        } catch (...) {
            h->exit_cb.reset();
        }
        h->close();
    };

    F functor;
};

}
}
