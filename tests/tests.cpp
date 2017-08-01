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

#include <exception>
#include <thread>

#include <gtest/gtest.h>

#include <wave.h>
#include <idle.h>
#include <timer.h>
#include <async.h>
#include <worker.h>
#include <merge.h>
#include <stream.h>
#include <zip.h>
#include <file.h>
#include <tcp.h>

template<typename T>
struct spy
{
    spy(T expect, T def)
            : data(new std::pair<T, T>(std::move(def),std::move(expect)), destroy)
    {}

    void inform(T t) const
    {
        std::get<0>(*data) = std::move(t);
    }

    static void destroy(std::pair<T, T>* data)
    {
        EXPECT_EQ(std::get<0>(*data), std::get<1>(*data));
        delete data;
    }

    std::shared_ptr<std::pair<T, T>> data;
};


TEST(SourceTests, Callback)
{
    using namespace wave;
    source<int> s;
    s >>= $(int data){
        EXPECT_EQ(data, 2);
    };
    s(2);
}

TEST(SourceTests, Exception)
{
    using namespace wave;
    source<int> s;
    s >>= $(int data) {
        EXPECT_EQ(data, 2);
        throw std::exception();
    } $finally {
        ASSERT_THROW(rethrow(), std::exception);
    };
    s(2);
}

TEST(MergeTests, Callback)
{
    using namespace wave;
    spy<int> merge_spy{ 4, 0 };
    loop loop;
    auto i = std::make_shared<int>(0);
    merge(idle{2}, timer{1,2})
    >>= ${
        (*i)++;
        merge_spy.inform(*i);
    };
}

TEST(ZipTests, Callback)
{
    using namespace wave;
    source<int> s1;
    source<std::string> s2;
    auto z = zip(s1, s2);
    z >>= $(int data1, std::string data2) {
        EXPECT_EQ(data1, 2);
        EXPECT_EQ(data2, "sada");
    };
    s1(2);
    s2(std::string("sada"));
}


TEST(FlatMapTests, Callback)
{
    using namespace wave;
    spy<int> s{ 4, 0 };
    loop loop;
    auto i = std::make_shared<int>(0);
    idle{ 2 }
    >>= ${ return timer{ 1,2 }; }
    >>= ${
        (*i)++;
        s.inform(*i);
    } $finally{
        rethrow();
    };
}

TEST(FileTests, Reader)
{
    using namespace wave;
    spy<bool> open_spy{ true, false };
    spy<std::string> read_spy{ "test", "" };
    loop loop;

    file f{ "text.txt" };
    f >>= ${
        open_spy.inform(true);
        return file_reader(f);
    } $finally{
        rethrow();
    }
    >>= $(std::string data) {
        read_spy.inform(data);
        f.close();
    } $finally{
        rethrow();
    };
}

TEST(IdleTests, Callback)
{
    using namespace wave;
    spy<int> idle_spy{ 4, 0 };
    loop loop;
    auto t = std::make_shared<int>(0);
    idle{4} >>= ${
        (*t)++;
        idle_spy.inform(*t);
    };
}

TEST(TimerTests, Callback)
{
    using namespace wave;
    spy<int> timer_spy{ 4, 4 };
    loop loop;
    auto t = std::make_shared<int>(0);
    timer{ 1, 4 }
    >>= ${ return 1; }
    >>= $(int i){
        (*t) += i;
        timer_spy.inform(*t);
    };
}


TEST(AsyncTests, Affinity)
{
    using namespace wave;
    loop loop;
    auto main_id = std::this_thread::get_id();
    async_source<std::thread::id> s;
    s >>= $(std::thread::id i) {
        EXPECT_NE(i, main_id);
    };

    queue_work(${
        s(std::this_thread::get_id());
    }) >>= ${
        EXPECT_EQ(main_id, std::this_thread::get_id());
        s.close_later();
    };
}

TEST(AsyncTests, Times)
{
    using namespace wave;
    spy<int> async_spy{ 9, 0 };
    loop loop;
    async_source<int> s;
    s >>= $(int i) {
        async_spy.inform(i);
    };

    queue_work(${
        int i = 0;
        while (i < 10) {
            s(i);
            i++;
        }
    }) >>= ${
        s.close_later();
    };
}

TEST(TcpTests, ServerClient)
{
    using namespace wave;
    spy<bool> tcp_listen{ true, false };
    spy<bool> tcp_connect{ true, false };
    spy<std::string> tcp_read{ "acasa", "" };
    spy<bool> tcp_write{ true, false };
    loop loop;
    tcp_server server{ 5000 };
    server >>= ${
        tcp_listen.inform(true);
        auto client = server.accept();
        client >>= $(std::string data) {
            tcp_read.inform(data);
            server.close();
            client.close();
        };
    };

    idle{} >>= ${
        tcp_client client{ "127.0.0.1", 5000 };
        client.connected() >>= ${
            tcp_connect.inform(true);
            client.wrote() >>= ${
                tcp_write.inform(true);
            };
            client << "acasa";
        };
    };
}
