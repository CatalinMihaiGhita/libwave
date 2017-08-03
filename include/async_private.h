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

#include <queue>
#include <mutex>
#include <memory>
#include <atomic>

#include <uv.h>

#include "wave.h"

namespace wave {
namespace detail {
	template <typename... T>
	struct async_handle
	{
		async_handle()
			: close_later_flag(false)
		{
			async.data = this;
			uv_async_init(uv_default_loop(), &async, default_async_cb);
		}

		void close()
		{
			uv_close(reinterpret_cast<uv_handle_t*>(&async),
			[](uv_handle_t* handle) {
				auto p = static_cast<async_handle*>(handle->data);
				p->async_cb.reset();
				p->self.reset();
			});
		}

		static void default_async_cb(uv_async_t* handle)
		{
			auto p = static_cast<async_handle*>(handle->data);
			p->close();
		}

		void close_later()
		{
			std::unique_lock<std::mutex> lk(queuemutex);
			if (argsqueue.size() > 0) {
				close_later_flag = true;
			} else {
				close();
			}
		}

		template <typename... Args>
		void invoke(Args&&... args)
		{
			{
				std::unique_lock<std::mutex> lk(queuemutex);
				argsqueue.emplace(std::make_unique<std::tuple<T...>>(std::forward<Args>(args)...));
			}
			uv_async_send(&async);
		}

		uv_async_t async;
		bool close_later_flag;
		std::mutex queuemutex;
		std::queue<std::unique_ptr<std::tuple<T...>>> argsqueue;
		std::unique_ptr<callback> async_cb;
		std::shared_ptr<async_handle> self;
	};

	template <>
	struct async_handle<>
	{
		async_handle()
			: count(0)
			, close_cb(nullptr)
			, close_later_flag(false)
		{
			async.data = this;
			uv_async_init(uv_default_loop(), &async, default_async_cb);
		}

		static void default_async_cb(uv_async_t* handle)
		{
			auto p = static_cast<async_handle*>(handle->data);
			p->close();
		}

		void close()
		{
			uv_close(reinterpret_cast<uv_handle_t*>(&async),
			[](uv_handle_t* handle) {
				auto p = static_cast<async_handle*>(handle->data);
				p->async_cb.reset();
				p->self.reset();
			});
		}

		void close_later()
		{
			if (count) {
				close_later_flag = true;
			} else {
				close();
			}
		}

		void invoke()
		{
			++count;
			uv_async_send(&async);
		}

		uv_async_t async;
		uv_close_cb close_cb;
		bool close_later_flag;
		std::atomic<int> count;
		std::unique_ptr<callback> async_cb;
		std::shared_ptr<async_handle> self;
	};

	template <typename F, typename... T>
	struct async_start : public callback
	{
		async_start(F f, async_handle<T...>* h)
			: functor(std::move(f))
		{
			h->async.async_cb = cb;
		}

		static void cb(uv_async_t* handle)
		{
			auto h = static_cast<async_handle<T...>*>(handle->data);
			auto p = static_cast<async_start*>(h->async_cb.get());
			p->call_tuple(h, std::index_sequence_for<T...>{});
		};

		template <size_t... I>
		void call_tuple(async_handle<T...>* handle, std::index_sequence<I...>)
		{
			bool notify_again = false;
			std::unique_ptr<std::tuple<T...>> args;
			{
				std::unique_lock<std::mutex> lk(handle->queuemutex);
				args = std::move(handle->argsqueue.front());
				handle->argsqueue.pop();
				notify_again = handle->argsqueue.size() > 0;
			}
			if (notify_again) {
				uv_async_send(&handle->async);
			} else if (handle->close_later_flag) {
				handle->close();
			}
			try {
				functor(std::move(std::get<I>(*args))...);
			} catch (...) {
				handle->close();
				handle->async_cb.reset();
			}
		}

		F functor;
	};

	template <typename F>
	struct async_start<F> : public callback
	{
		async_start(F f, async_handle<>* h)
			: functor(std::move(f))
		{
			h->async.async_cb = cb;
		}

		static void cb(uv_async_t* handle)
		{
			auto h = static_cast<async_handle<>*>(handle->data);
			auto p = static_cast<async_start*>(h->async_cb.get());
			p->call(h);
		}

		void call(async_handle<>* handle)
		{
			if (--handle->count > 0) {
				uv_async_send(&handle->async);
			} else if (handle->close_later_flag) {
				handle->close();
			}
			try {
				functor();
			} catch (...) {
				handle->close();
				handle->async_cb.reset();
			}
		}
		F functor;
	};
}
}
