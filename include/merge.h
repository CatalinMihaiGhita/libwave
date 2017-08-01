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

#include "wave_private.h"

#include <tuple>

namespace wave {
namespace detail {

template <typename... Us>
struct merge_map : public abstract_source
{
    std::tuple<Us...> sources;

    merge_map(Us&&... us)
        : sources{std::move(us)...}
    {}

    merge_map(const Us&... us)
        : sources{ us... }
    {}

    template <typename F>
    void operator>>=(const F& f)
    {
        map_tuple(f, std::index_sequence_for<Us...>{});
    }

    template <typename F, size_t... I>
    void map_tuple(const F& f, std::index_sequence<I...>)
    {
        int dummy[] = { 0, (std::get<I>(sources).operator>>=(f), 0) ... };
        (void)dummy;
    }
};
}

template <typename... Us>
decltype(auto) merge(Us&&... us)
{
    return detail::merge_map<std::decay_t<Us>...>{std::forward<Us>(us)...};
}

}
