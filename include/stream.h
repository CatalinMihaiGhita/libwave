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

using stream_read_source = source<detail::stream_handle*, detail::stream_read, std::string>;
using stream_wrote_source = source<detail::stream_handle*, detail::stream_write>;
using stream_connected_source = source<detail::stream_handle*, detail::stream_write>;

class stream : public stream_read_source
{
public:
    void operator<<(end_stream&&) const {
        handle->close();
    }

    template <typename String>
    const stream& operator<<(String&& data) const {
        handle->write(std::forward<String>(data));
        return *this;
    }

    stream_wrote_source wrote() const { return handle; }
    stream_connected_source connected() const { return handle; }

    void shutdown() const { handle->shutdown(); }
    void stop_reading() const { handle->stop_reading(); }
    void close() const { handle->close(); }

protected:
    stream(detail::stream_handle* handle)
        : base(handle)
    {}
};

}
