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

#include <string>
#include <vector>
#include <memory>

#include <uv.h>

namespace wave {
	namespace detail {
		struct file_handle
		{
			file_handle(std::string file_name) :
				file_name(std::move(file_name))
			{
				close_req.data = nullptr;
			}

			void close()
			{
				if (close_req.data != this) {
					close_req.data = this;
					uv_fs_close(uv_default_loop(), &close_req, open_req.result,
					[](uv_fs_t* handle) {
						auto p = static_cast<file_handle*>(handle->data);
						std::unique_ptr<callback> open_cb(std::move(p->open_cb));
						std::unique_ptr<callback> read_cb(std::move(p->read_cb));
						std::unique_ptr<callback> write_cb(std::move(p->write_cb));
					});
					uv_fs_req_cleanup(&open_req);
				}
			}

			uv_fs_t open_req;
			uv_fs_t close_req;
			std::string file_name;
			std::string data;
			std::unique_ptr<callback> open_cb;
			std::unique_ptr<callback> read_cb;
			std::unique_ptr<callback> write_cb;
		};

		template <typename F>
		struct open_file : public callback
		{
			open_file(F f, const std::shared_ptr<file_handle>& file)
				: functor(std::move(f))
				, handle(file)
			{
				handle->open_req.data = this;
				uv_fs_open(uv_default_loop(), &handle->open_req, handle->file_name.c_str(), GENERIC_READ, 0,
					[](uv_fs_t *req) {
					auto p = static_cast<open_file*>(req->data);
					if (req->result >= 0) {
						p->functor();
						p->handle->open_cb.reset();
					} else {
						p->handle->close();
					}
				});
			}

			F functor;
			std::shared_ptr<file_handle> handle;
		};

		template <typename F>
		struct read_file : public callback
		{
			read_file(F f, std::shared_ptr<file_handle>& file)
				: functor(std::move(f))
				, handle(file)
				, memory(8 * 1024)
			{
				read_req.data = this;
				buff = uv_buf_init(memory.data(), memory.size());
				read();
			}

			void read()
			{
				uv_fs_read(uv_default_loop(), &read_req, handle->open_req.result, &buff, 1, -1,
					[](uv_fs_t *req) {
					auto p = static_cast<read_file*>(req->data);
					if (req->result > 0) {
						p->functor(std::string(p->buff.base, req->result));
						uv_fs_req_cleanup(req);
						p->read();
					} else {
						p->handle->read_cb.reset();
					}
				});
			}

			std::vector<char> memory;
			uv_fs_t read_req;
			uv_buf_t buff;
			std::shared_ptr<file_handle> handle;
			F functor;
		};

		template <typename F>
		struct write_file : public callback
		{
			write_file(F f, const std::shared_ptr<file_handle>& file)
				: functor(std::move(f))
				, handle(file)
			{
				buff = uv_buf_init(const_cast<char*>(handle->data.data()), handle->data.size());
				uv_fs_write(uv_default_loop(), &write, handle->open_req.result, &buff, 1, -1,
					[](uv_fs_t *req) {
					auto p = static_cast<write_file*>(req->data);
					if (req->result > 0) {
						p->functor();
					}
					uv_fs_req_cleanup(req);
					p->handle->write_cb.reset();
				});
			}

			uv_fs_t write;
			uv_buf_t buff;
			std::shared_ptr<file_handle> handle;
			F functor;
		};
	}
}