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
#include <initializer_list>

#include "process_private.h"

namespace wave {

class process
{
public:
    process(std::initializer_list<std::string> args)
        : handle{ new detail::process_handle(std::move(args)) } {}

    class finished_source : public detail::generic_source<int64_t, int>
    {
    public:
        finished_source(detail::process_handle* handle) : handle{handle} {}
        template <class F>
        void operator>>= (F&& functor) {
            handle->exit_cb.reset(new detail::process_finished<std::decay_t<F>>{std::forward<F>(functor), handle });
        }
    private:
        detail::process_handle* handle;
    };

    void kill(int signum) const { handle->kill(signum); }

    pipe stdin() const { return handle->stdin_pipe; }
    pipe stdout() const { return handle->stdout_pipe; }
    pipe stderr() const {return handle->stderr_pipe; }

    finished_source finished() const { return finished_source(handle); }

private:
    detail::process_handle* handle;
};

}
