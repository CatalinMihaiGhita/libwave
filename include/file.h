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

#include "wave.h"

#include "file_private.h"

/// TODO: refactor

namespace wave {
	class file : public detail::generic_source<>
	{
	public:
		file(std::string file_name)
			: handle{std::make_shared<detail::file_handle>(std::move(file_name))}{}
		void close() const { handle->close();}
		template <class F>
		void operator>>= (F functor) {
			handle->open_cb.reset(new detail::open_file<F>{ std::move(functor), handle });
		}

	private:
		friend class file_reader;
		friend class file_writer;
		std::shared_ptr<detail::file_handle> handle;
	};

	class file_reader : public detail::generic_source<std::string>
	{
	public:
		file_reader(file f)
			: handle{ std::move(f.handle) } {}
		template <class F>
		void operator>>= (F functor) {
			handle->read_cb.reset(new detail::read_file<F>{ std::move(functor), handle });
		}
	private:
		std::shared_ptr<detail::file_handle> handle;
	};

	class file_writer : public detail::generic_source<>
	{
	public:
		file_writer(file f)
			: handle{ std::move(f.handle) } {}
		template <class F>
		void operator>>= (F functor) {
			handle->write_cb.reset(new detail::write_file<F>{ std::move(functor), handle });
		}
	private:
		std::shared_ptr<detail::file_handle> handle;
	};
}