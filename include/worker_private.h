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

#include <uv.h>

#include "wave_private.h"

namespace wave {
	namespace detail {
		template<typename Task>
		struct worker_handle
		{
			uv_work_t work;
			Task task;
			std::unique_ptr<callback> after_cb;

			worker_handle(Task task)
				: task(std::move(task))
			{
				work.data = this;
				uv_queue_work(uv_default_loop(), &work, default_work_cb, default_after_work_cb);
			}

			static void default_work_cb(uv_work_t* handle)
			{
				auto p = static_cast<worker_handle*>(handle->data);
				p->task();
			}

			static void default_after_work_cb(uv_work_t* handle, int status)
			{
				delete static_cast<worker_handle*>(handle->data);
			}

			void cancel()
			{
				uv_cancel(reinterpret_cast<uv_req_t*>(&work));
			}
		};

		template <typename Task, typename F >
		struct work_start : public callback
		{
			work_start(F f, worker_handle<Task>* h)
				: functor(std::move(f))
			{
				h->work.after_work_cb = cb;
			}

			static void cb(uv_work_t* handle, int status)
			{
				auto h = static_cast<worker_handle<Task>*>(handle->data);
				try {
					if (status != 0) {
						throw std::exception("status != 0");
					}
					auto p = static_cast<work_start*>(h->after_cb.get());
					p->functor();
					delete h;
				}
				catch (...) {
					delete h;
				}
			}
			F functor;
		};
	}
}
