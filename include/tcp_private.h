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

struct tcp_client_handle : public stream_handle
{
    tcp_client_handle(std::string a, int port)
    {
        uv_ip4_addr(a.c_str(), port, &addr);
        init();
        uv_tcp_connect(&connect_handle, &tcp, reinterpret_cast<const sockaddr*>(&addr), connect_handle.cb);
    }

    tcp_client_handle()
    {
        init();
    }

    void init()
    {
        uv_tcp_init(uv_default_loop(), &tcp);
        stream_handle::init(reinterpret_cast<uv_stream_t*>(&tcp));
        close_cb = tcp_close_cb;
    }

    static void tcp_close_cb(uv_handle_t* handle)
    {
        delete static_cast<tcp_client_handle*>(handle->data);
    }

    uv_tcp_t tcp;
    sockaddr_in addr;
};

struct tcp_server_handle
{
    tcp_server_handle(int port, int maxcon)
    {
        uv_ip4_addr("0.0.0.0", port, &addr);
        uv_tcp_init(uv_default_loop(), &tcp);
        tcp.data = this;
        uv_tcp_bind(&tcp, reinterpret_cast<const struct sockaddr*>(&addr), 0);
        uv_listen(reinterpret_cast<uv_stream_t*>(&tcp), maxcon, default_listen_cb);
    }

    static void default_listen_cb(uv_stream_t *req, int status)
    {
        auto h = static_cast<tcp_server_handle*>(req->data);
        h->close();
    }

    tcp_client_handle* accept()
    {
        auto client = new tcp_client_handle();
        uv_accept(reinterpret_cast<uv_stream_t*>(&tcp), reinterpret_cast<uv_stream_t*>(&client->tcp));
        return client;
    }

    void close()
    {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&tcp))) {
            uv_close(reinterpret_cast<uv_handle_t*>(&tcp),
                     [](uv_handle_t* h) {
                delete static_cast<tcp_server_handle*>(h->data);
            });
        }
    }

    uv_tcp_t tcp;
    struct sockaddr_in addr;
    std::unique_ptr<callback> listen_cb;
};

template <typename F>
struct tcp_listen : public callback
{
    tcp_listen(F f, tcp_server_handle* server)
        : functor(std::move(f))
    {
        server->tcp.connection_cb = cb;
    }

    static void cb(uv_stream_t *req, int status)
    {
        auto h = static_cast<tcp_server_handle*>(req->data);
        try {
            if (status < 0) {
                throw std::exception();
            }
            auto p = static_cast<tcp_listen*>(h->listen_cb.get());
            p->functor();
        }
        catch (...) {
            h->close();
            h->listen_cb.reset();
        }
    }
    F functor;
};

}
}
