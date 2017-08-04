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

namespace wave {

using file_read_source = source<std::shared_ptr<detail::file_handle>, detail::read_file, std::string>;
using file_wrote_source = source<std::shared_ptr<detail::file_handle>, detail::write_file>;

class file : public file_read_source
{
public:
    file(std::string file_name)
        : base{std::make_shared<detail::file_handle>(std::move(file_name))}
    {}

    void close() const { handle->close();}
    file_wrote_source wrote() { return handle; }

    template <typename String>
    const file& operator<<(String&& data) const {
        handle->write(std::forward<String>(data));
        return *this;
    }
};

}
