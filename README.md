## Welcome to libWave

A C++ library for asynchronous functional programming.

All you need is the `>>=` operator.

### Start with the loop

```C++
int main()
{
  using namespace wave;
  loop main_loop;
  idle{} >>= { std::cout << "Hello World!"; };
  return 0;
}
```

### Idling

```C++
idle{2}
>>= ${ std::cout << "This message is twice!"; };
```

### Timer

```C++
timer{1000}
>>= ${ std::cout << "One second passed!"; };
```

### Composing

```C++
idle{}
>>= ${ return 1000; }
>>= $(int timeout) { return timer{timeout}; }
>>= ${ std::cout << "Maybe one second passed!"; };
```
### TCP server

```C++
tcp_server server{ 5000 };
std::cout << "Listening..." << std::endl;
server >>= ${
  std::cout << "Client connected" << std::endl;
  auto client = server.accept();
  client << "I send you 50$";
  return client;
} >>= $(std::string data)
  std::cout << "Client sent me: " << data << std::endl;
};
```
### TCP client

```C++
tcp_client client{ "127.0.0.1", 5000 };
client.connected() >>= ${
  std::cout << "Client connected" << std::endl;
  client << "I send you 100$";
  return client;
} >>= $(std::string data)
  std::cout << "Server sent me: " << data << std::endl;
};
```
