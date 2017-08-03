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

#include "tcp_private.h"
#include "wave.h"
#include "stream.h"

namespace wave {

using tcp_server_connected_source = source<detail::tcp_server_handle*, detail::tcp_listen>;

class tcp_client : public stream
{
public:
    tcp_client(std::string address, int port)
        : stream(new detail::tcp_client_handle(std::move(address), port))
    {}

private:
    tcp_client(detail::tcp_client_handle* handle)
        : stream(handle)
    {}

    friend class tcp_server;
};

class tcp_server : public tcp_server_connected_source
{
public:
    tcp_server(int port, int max_connections = SOMAXCONN)
        : base(new detail::tcp_server_handle(port, max_connections))
    {}

    tcp_client accept() const { return tcp_client(handle->accept()); }
    void close() const { handle->close(); }
};

}
