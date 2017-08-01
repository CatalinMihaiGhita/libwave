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

#include "stream_private.h"
#include "wave.h"

namespace wave {

class end_stream
{
};

class stream : public detail::generic_source<std::string>
{
public:
    template <class F>
    void operator>>= (F functor) const {
        handle->read_cb.reset(new detail::stream_read<F>{ std::move(functor), handle });
        handle->start_reading();
    }

    void operator<<(end_stream&&) const {
        handle->close();
    }

    template <typename String>
    const stream& operator<<(String&& data) const {
        handle->write(std::forward<String>(data));
        return *this;
    }

    class wrote_source : public detail::generic_source<>
    {
    public:
        template <class F>
        void operator>>= (F functor) const {
            handle->write_cb.reset(new detail::stream_write<F>{ std::move(functor), handle });
        }
    private:
        friend class stream;
        wrote_source(detail::stream_handle* handle)
            : handle(handle){}
        detail::stream_handle* handle;
    };

    class connected_source : public detail::generic_source<>
    {
    public:
        template <class F>
        void operator>>= (F functor) const {
            handle->connect_cb.reset(new detail::stream_connect<F>{ std::move(functor), handle });
        }
    private:
        friend class stream;
        connected_source(detail::stream_handle* handle)
            : handle(handle) {}
        detail::stream_handle* handle;
    };

    wrote_source wrote() const { return handle; }
    connected_source connected() const { return handle; }

    void shutdown() const { handle->shutdown(); }
    void stop_reading() const { handle->stop_reading(); }
    void close() const { handle->close(); }

protected:
    stream(detail::stream_handle* handle)
        : handle(handle)
    {}

private:
    detail::stream_handle* handle;
};

}
