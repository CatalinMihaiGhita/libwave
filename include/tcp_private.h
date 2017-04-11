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
	struct tcp_client_handle
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
			tcp.data = this;
			connect_handle.data = this;
			connect_handle.cb = default_connect_cb;
			write_handle.data = this;
			write_handle.cb = default_write_cb;
			tcp.alloc_cb = nullptr;
			tcp.read_cb = nullptr;
		}

		void close()
		{
			if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&poll_handle))) {
				uv_close(reinterpret_cast<uv_handle_t*>(&poll_handle),
				[](uv_handle_t* handle) {
					auto p = static_cast<tcp_client_handle*>(handle->data);
					p->close_socket();
				});
			}
		}

		void close_socket()
		{
			connect_cb.reset();
			if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&tcp))) {
				tcp.data = this;
				uv_close(reinterpret_cast<uv_handle_t*>(&tcp), [](uv_handle_t* handle) {
					delete static_cast<tcp_client_handle*>(handle->data);
				});
			}
		}

		void stop_reading()
		{
			uv_read_stop(reinterpret_cast<uv_stream_t*>(&tcp));
			read_cb.reset();
		}

		void cancel_write()
		{
			uv_cancel(reinterpret_cast<uv_req_t*>(&write_handle));
		}

		void shutdown()
		{
			shutdown_handle.data = this;
			uv_shutdown(&shutdown_handle, reinterpret_cast<uv_stream_t*>(&tcp),
				[](uv_shutdown_t* req, int status) {
				auto p = static_cast<tcp_client_handle*>(req->data);
				if (status == 0) {
					p->close();
				}
			});
		}

		static void default_write_cb(uv_write_t *req, int status)
		{
			auto p = static_cast<tcp_client_handle*>(req->data);
			if (status != 0) {
				p->close();
			}
		}

		static void default_connect_cb(uv_connect_t *req, int status)
		{
			auto p = static_cast<tcp_client_handle*>(req->data);
			p->close_socket();
		}

		template <typename String>
		void write(String&& s)
		{
			data = std::forward<String>(s);
			buff = uv_buf_init(const_cast<char*>(data.data()), data.size());
			uv_write(&write_handle, reinterpret_cast<uv_stream_t*>(&tcp), &buff, 1, write_handle.cb);
		}

		void poll_disconnect()
		{
			uv_os_fd_t fd;
			if (uv_fileno(reinterpret_cast<uv_handle_t*>(&tcp), &fd) == 0) {
				uv_poll_init_socket(uv_default_loop(), &poll_handle, reinterpret_cast<uv_os_sock_t>(fd));
				poll_handle.data = this;
				uv_poll_start(&poll_handle, UV_DISCONNECT,
					[](uv_poll_t* handle, int status, int events) {
					auto p = static_cast<tcp_client_handle*>(handle->data);
					if (status == 0) {
						p->close();
					}
				});
			}
		}

		void start_reading()
		{
			uv_read_start(reinterpret_cast<uv_stream_t*>(&tcp), tcp.alloc_cb, tcp.read_cb);
		}

		uv_tcp_t tcp;
		uv_connect_t connect_handle;
		uv_shutdown_t shutdown_handle;
		uv_write_t write_handle;
		uv_poll_t poll_handle;
		sockaddr_in addr;
		uv_buf_t buff;
		std::string data;
		std::unique_ptr<callback> connect_cb;
		std::unique_ptr<callback> read_cb;
		std::unique_ptr<callback> write_cb;
	};

	template <typename F>
	struct tcp_connect : public callback
	{
		tcp_connect(F f, tcp_client_handle* h)
			: functor(std::move(f))
		{
			h->connect_handle.cb = cb;
		}

		static void cb(uv_connect_t* req, int status)
		{
			auto h = static_cast<tcp_client_handle*>(req->data);
			try {
				if (status != 0) {
                                        throw std::exception();
				}
				auto p = static_cast<tcp_connect*>(h->connect_cb.get());
				h->poll_disconnect();
				p->functor();
			}
			catch (...) {
				h->close_socket();
			}
		}

		F functor;
	};

	template <typename F>
	struct tcp_read : public callback
	{
		tcp_read(F f, tcp_client_handle* h)
			: functor(std::move(f))
		{
			h->tcp.alloc_cb = alloc_cb;
			h->tcp.read_cb = cb;
		}

		static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
		{
			auto h = static_cast<tcp_client_handle*>(handle->data);
			auto p = static_cast<tcp_read*>(h->read_cb.get());
			p->data.reserve(suggested_size);
			*buf = uv_buf_init(const_cast<char*>(p->data.data()), suggested_size);
		}

		static void cb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
		{
			auto h = static_cast<tcp_client_handle*>(handle->data);
			try {
				auto p = static_cast<tcp_read*>(h->read_cb.get());
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
	struct tcp_write : public callback
	{
		tcp_write(F f, tcp_client_handle* h)
			: functor(std::move(f))
		{
			h->write_handle.cb = cb;
		}

		static void cb(uv_write_t *req, int status)
		{
			auto h = static_cast<tcp_client_handle*>(req->data);
			try {
				if (status != 0) {
                                        throw std::exception();
				}
				auto p = static_cast<tcp_write*>(h->write_cb.get());
				p->functor();
			}
			catch (...) {
				h->cancel_write();
				h->write_cb.reset();
			}
		}
		F functor;
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
			client->poll_disconnect();
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
			server->tcp.stream.serv.connection_cb = cb;
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
