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

namespace wave {

	class tcp_client : public detail::generic_source<int>
	{
	public:
		tcp_client(std::string address, int port)
			: handle(new detail::tcp_client_handle(address, port)) {}
		void shutdown() const { handle->shutdown(); }
		void stop_reading() const { handle->stop_reading(); }
		void close() const { handle->close(); }
		template <class F>
		void operator>>= (F functor) { handle->connect_cb.reset(new detail::tcp_connect<F>{ std::move(functor), handle }); }
	private:
		tcp_client(detail::tcp_client_handle* handle)
			: handle(std::move(handle)) {}
		friend class tcp_server;
		friend class tcp_writer;
		friend class tcp_reader;
		detail::tcp_client_handle* handle;
	};

	class tcp_reader : public detail::generic_source<std::string>
	{
	public:
		tcp_reader(const tcp_client& client) : handle(std::move(client.handle)) {
			handle->start_reading();
		}
		template <class F>
		void operator>>= (F functor) { handle->read_cb.reset(new detail::tcp_read<F>{ std::move(functor), handle }); }
	private:
		detail::tcp_client_handle* handle;
	};

	class tcp_writer : public detail::generic_source<>
	{
	public:
		tcp_writer(const tcp_client& client)
			: handle(client.handle)
		{}

		template <typename String>
		const tcp_writer& operator<<(String&& data) const { handle->write(std::forward<String>(data)); return *this; }

		template <class F>
		void operator>>= (F functor) { handle->write_cb.reset(new detail::tcp_write<F>{ std::move(functor), handle }); }
	private:
		detail::tcp_client_handle* handle;
	};

	class tcp_server : public detail::generic_source<int>
	{
	public:
		tcp_server(int port, int max_connections = SOMAXCONN)
			: handle(new detail::tcp_server_handle(port, max_connections)) {}
		tcp_client accept() const { return tcp_client(handle->accept()); }
		void close() const { handle->close(); }
		template <class F>
		void operator>>= (F functor) { handle->listen_cb.reset(new detail::tcp_listen<F>{ std::move(functor), handle }); }
	private:
		detail::tcp_server_handle* handle;
	};
}